#include "Bullet.h"

#include <algorithm>		// Use std::remove_if

#include "Donya/Sound.h"

#include "Bullets/Buster.h"
#include "Common.h"			// Use IsShowCollision()
#include "FilePath.h"
#include "ModelHelper.h"
#include "Music.h"
#include "Parameter.h"

namespace Bullet
{
	namespace
	{
		constexpr size_t kindCount = scast<size_t>( Kind::KindCount );
		constexpr const char *modelFolderName = "Bullet/";
		constexpr std::array<const char *, kindCount> modelNames
		{
			"Buster",
		};

		static std::array<std::shared_ptr<ModelHelper::SkinningSet>, kindCount> modelPtrs{ nullptr };

		bool LoadModels()
		{
			const std::string folderName = modelFolderName;
			bool succeeded = true;
			std::string filePath{};
			for ( size_t i = 0; i < kindCount; ++i )
			{
				if ( modelPtrs[i] ) { continue; }
				// else

				filePath = MakeModelPath( folderName + modelNames[i] );

				if ( !Donya::IsExistFile( filePath ) )
				{
					const std::string msg = "Error: File is not found: " + filePath + "\n";
					Donya::OutputDebugStr( msg.c_str() );
					succeeded = false;
					continue;
				}
				// else

				modelPtrs[i] = std::make_shared<ModelHelper::SkinningSet>();
				const bool result = ModelHelper::Load( filePath, modelPtrs[i].get() );
				if ( !result )
				{
					modelPtrs[i].reset(); // Make not loaded state

					const std::string msg = "Failed: Loading failed: " + filePath;
					Donya::OutputDebugStr( msg.c_str() );
					succeeded = false;
					continue;
				}
				// else
			}

			return succeeded;
		}
		constexpr bool IsOutOfRange( Kind kind )
		{
			return ( kindCount <= scast<size_t>( kind ) );
		}
		std::shared_ptr<ModelHelper::SkinningSet> GetModelPtrOrNullptr( Kind kind )
		{
			if ( IsOutOfRange( kind ) ) { return nullptr; }
			// else

			const auto &ptr = modelPtrs[scast<size_t>( kind )];
			if ( !ptr )
			{
				_ASSERT_EXPR( 0, L"Error: The Bullet's model is not initialized!" );
				return nullptr;
			}
			// else

			return ptr;
		}
		constexpr const char *GetModelName( Kind kind )
		{
			if ( IsOutOfRange( kind ) ) { return nullptr; }
			// else

			return modelNames[scast<size_t>( kind )];
		}
	}

	namespace Parameter
	{
		void Load()
		{
			Impl::LoadBuster();
		}

	#if USE_IMGUI
		void Update( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			Impl::UpdateBuster( u8"Buster" );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI
	}

	bool LoadResource()
	{
		Parameter::Load();
		return LoadModels();
	}

#if USE_IMGUI
	void FireDesc::ShowImGuiNode( const std::string &nodeCaption, bool isRelativePos )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat( u8"初速[m/s]", &initialSpeed, 0.1f );
		initialSpeed = std::max( 0.0f, initialSpeed );

		ImGui::SliderFloat3( u8"方向", &direction.x, -1.0f, 1.0f );
		if ( ImGui::Button( u8"方向を正規化" ) )
		{
			direction.Normalize();
		}

		const char *caption = ( isRelativePos ) ? u8"生成位置（ローカル・相対）" : u8"ワールド座標";
		ImGui::DragFloat3( caption, &position.x, 0.1f );

		ImGui::TreePop();
	}
#endif // USE_IMGUI

	void Base::Init( const FireDesc &parameter )
	{
		model.pResource = GetModelPtrOrNullptr( GetKind() );
		if ( model.pResource )
		{
			model.pose.AssignSkeletal( model.pResource->skeletal );
			model.animator.ResetTimer();
		}

		InitBody( parameter );

		velocity	= parameter.direction * parameter.initialSpeed;
		UpdateOrientation( parameter.direction );
		wantRemove	= false;
	}
	void Base::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreen )
	{
		body.UpdateIgnoreList( elapsedTime );

		if ( wasCollided )
		{
			CollidedProcess();
		}

		if ( OnOutSide( wsScreen ) )
		{
			wantRemove = true;
		}
	}
	void Base::PhysicUpdate( float elapsedTime )
	{
		const auto movement = velocity * elapsedTime;
		Solid::Move( movement, {}, {} );
	}
	void Base::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer			) { return; }
		if ( !model.pResource	) { return; }
		// else

		const Donya::Vector3   &drawPos = body.pos; // I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
		const Donya::Vector4x4 W		= MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );

		Donya::Model::Constants::PerModel::Common modelConstant{};
		modelConstant.drawColor			= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		modelConstant.worldMatrix		= W;;
		pRenderer->UpdateConstant( modelConstant );
		pRenderer->ActivateConstantModel();

		pRenderer->Render( model.pResource->model, model.pose );

		pRenderer->DeactivateConstantModel();
	}
	void Base::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

	#if DEBUG_MODE
		Donya::Model::Cube::Constant constant;
		constant.matWorld			= MakeWorldMatrix( body.size * 2.0f, /* enableRotation = */ false, body.WorldPosition() );
		constant.matViewProj		= VP;
		constant.drawColor			= Donya::Vector4{ 1.0f, 0.6f, 0.0f, 0.6f };
		constant.lightDirection		= -Donya::Vector3::Up();

		pRenderer->ProcessDrawingCube( constant );
	#endif // DEBUG_MODE
	}
	bool Base::ShouldRemove() const
	{
		return wantRemove;
	}
	bool Base::OnOutSide( const Donya::Collision::Box3F &wsScreen ) const
	{
		return ( !Donya::Collision::IsHit( body, wsScreen ) );
	}
	void Base::CollidedToObject() const
	{
		wasCollided = true;
		// Donya::Sound::Play( Music::Bullet_Hit );
	}
	void Base::CollidedProcess()
	{
		wasCollided = false;
		wantRemove  = true;
		body.exist  = false;
	}
	void Base::InitBody( const FireDesc &parameter )
	{
		AssignBodyParameter( parameter.position );
		body.id			= Donya::Collision::GetUniqueID();
		body.ownerID	= parameter.owner;
		body.ignoreList.clear();
	}
	void Base::UpdateOrientation( const Donya::Vector3 &direction )
	{
		orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), direction );
	}
	void Base::UpdateMotionIfCan( float elapsedTime, int motionIndex )
	{
		if ( !model.IsAssignableIndex( motionIndex ) ) { return; }
		// else

		model.UpdateMotion( elapsedTime, motionIndex );
	}
	Donya::Vector4x4 Base::MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const
	{
		Donya::Vector4x4 W{};
		W._11 = scale.x;
		W._22 = scale.y;
		W._33 = scale.z;
		if ( enableRotation ) { W *= orientation.MakeRotationMatrix(); }
		W._41 = translation.x;
		W._42 = translation.y;
		W._43 = translation.z;
		return W;
	}
#if USE_IMGUI
	void Base::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::Button( ( nodeCaption + u8"を削除" ).c_str() ) )
		{
			wantRemove = true;
		}

		ImGui::Helper::ShowAABBNode ( u8"体", &body );
		ImGui::DragFloat3( u8"速度", &velocity.x, 0.1f );
		ImGui::Helper::ShowFrontNode( "", &orientation );

		ImGui::TreePop();
	}
#endif // USE_IMGUI


	void Admin::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreen )
	{
		GenerateRequestedFires();
		generateRequests.clear();

		for ( auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->Update( elapsedTime, wsScreen );

			if ( pIt->ShouldRemove() )
			{
				pIt->Uninit(); // It will be removed at RemoveInstancesIfNeeds(), so we should finalize here.
			}
		}

		RemoveInstancesIfNeeds();
	}
	void Admin::PhysicUpdate( float elapsedTime )
	{
		// TODO: Should detect a remove sign and erase that in here?

		for ( auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->PhysicUpdate( elapsedTime );

			if ( pIt->ShouldRemove() )
			{
				pIt->Uninit();
			}
		}

		RemoveInstancesIfNeeds();
	}
	void Admin::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer ) { return; }
		// else
		for ( const auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->Draw( pRenderer );
		}
	}
	void Admin::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !pRenderer ) { return; }
		// else
		for ( const auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->DrawHitBox( pRenderer, VP );
		}
	}
	void Admin::ClearInstances()
	{
		for ( auto &pIt : bulletPtrs )
		{
			if ( pIt ) { pIt->Uninit(); }
		}
		bulletPtrs.clear();
	}
	void Admin::RequestFire( const FireDesc &parameter )
	{
		generateRequests.emplace_back( parameter );
	}
	size_t Admin::GetInstanceCount() const
	{
		return bulletPtrs.size();
	}
	bool Admin::IsOutOfRange( size_t instanceIndex ) const
	{
		return ( GetInstanceCount() <= instanceIndex ) ? true : false;
	}
	std::shared_ptr<const Base> Admin::GetInstanceOrNullptr( size_t instanceIndex ) const
	{
		if ( IsOutOfRange( instanceIndex ) ) { return nullptr; }
		// else
		return bulletPtrs[instanceIndex];
	}
	void Admin::GenerateRequestedFires()
	{
		auto Append = [&]( const FireDesc &parameter )
		{
			std::shared_ptr<Base> tmp = nullptr;

			switch ( parameter.kind )
			{
			case Kind::Buster: tmp = std::make_shared<Buster>(); break;
			default: _ASSERT_EXPR( 0, L"Error: Unexpected bullet kind!" ); return;
			}

			if ( !tmp ) { return; }
			// else

			tmp->Init( parameter );
			bulletPtrs.emplace_back( std::move( tmp ) );
		};

		for ( const auto &it : generateRequests )
		{
			Append( it );
		}
	}
	void Admin::RemoveInstancesIfNeeds()
	{
		auto result = std::remove_if
		(
			bulletPtrs.begin(), bulletPtrs.end(),
			[]( std::shared_ptr<Base> &pElement )
			{
				return ( pElement ) ? pElement->ShouldRemove() : true;
			}
		);
		bulletPtrs.erase( result, bulletPtrs.end() );
	}
#if USE_IMGUI
	void Admin::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::TreeNode( u8"実体操作" ) )
		{
			std::string caption;
			const size_t count = bulletPtrs.size();
			for ( size_t i = 0; i < count; ++i )
			{
				if ( !bulletPtrs[i] ) { continue; }
				// else

				caption =  "[";
				if ( i < 100 ) { caption += "_"; } // Align
				if ( i < 10  ) { caption += "_"; } // Align
				caption += std::to_string( i );
				caption += "]";
				bulletPtrs[i]->ShowImGuiNode( caption );
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI
}
