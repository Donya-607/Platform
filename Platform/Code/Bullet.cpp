#include "Bullet.h"

#include <algorithm>			// Use std::remove_if

#include "Donya/Sound.h"

#include "BulletParam.h"
#include "Bullets/Buster.h"
#include "Bullets/SkullBullet.h"
#include "Bullets/SuperBall.h"
#include "Common.h"				// Use IsShowCollision()
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
			"SkullBuster",
			"SkullShield",
			"SuperBall",
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
			Impl::LoadGeneral();
			Impl::LoadSkullBuster();
			Impl::LoadSkullShield();
			Impl::LoadSuperBall();
		}

	#if USE_IMGUI
		void Update( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			Impl::UpdateGeneral		( u8"General" );
			Impl::UpdateBuster		( u8"Buster" );
			Impl::UpdateSkullBuster	( u8"SkullBuster" );
			Impl::UpdateSkullShield	( u8"SkullShield" );
			Impl::UpdateSuperBall	( u8"SuperBall" );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI

		static ParamOperator<GeneralParam> generalParam{ "General", "Bullet/" };
		const GeneralParam &GetGeneral()
		{
			return generalParam.Get();
		}
		namespace Impl
		{
			void LoadGeneral()
			{
				generalParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateGeneral( const std::string &nodeCaption )
			{
				generalParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	bool LoadResource()
	{
		Parameter::Load();
		return LoadModels();
	}

#if USE_IMGUI
	constexpr const char *GetKindName( Kind kind )
	{
		switch ( kind )
		{
		case Kind::Buster:		return "Buster";
		case Kind::SkullBuster:	return "SkullBuster";
		case Kind::SkullShield:	return "SkullShield";
		case Kind::SuperBall:	return "SuperBall";
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
		return "ERROR_KIND";
	}
	void ShowKindNode( const std::string &nodeCaption, Kind *p )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		for ( size_t i = 0; i < kindCount; ++i )
		{
			const Kind current = scast<Kind>( i );
			if ( ImGui::RadioButton( GetKindName( current ), *p == current ) )
			{
				*p = current;
			}
		}

		ImGui::TreePop();
	}
	void FireDesc::ShowImGuiNode( const std::string &nodeCaption, bool isRelativePos )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ShowKindNode( u8"生成する種類", &kind );

		ImGui::DragFloat( u8"初速[m/s]", &initialSpeed, 0.1f );
		initialSpeed = std::max( 0.0f, initialSpeed );

		ImGui::SliderFloat3( u8"方向", &direction.x, -1.0f, 1.0f );
		if ( ImGui::Button( u8"方向を正規化" ) )
		{
			direction.Normalize();
		}

		const char *caption = ( isRelativePos ) ? u8"生成位置（ローカル・相対）" : u8"ワールド座標";
		ImGui::DragFloat3( caption, &position.x, 0.1f );

		bool addDamage = ( !pAdditionalDamage ) ? false : true;
		ImGui::Checkbox( u8"ダメージを追加するか", &addDamage );
		if ( addDamage )
		{
			if ( !pAdditionalDamage )
			{
				pAdditionalDamage = std::make_shared<Definition::Damage>();
			}

			ImGui::Text( u8"生成する弾の持つダメージ・属性等に合算します" );
			pAdditionalDamage->ShowImGuiNode( u8"加算ダメージ設定" );
		}
		else
		{
			pAdditionalDamage.reset();
			ImGui::TextDisabled( u8"加算ダメージ設定" );
		}

		ImGui::TreePop();
	}

	void GeneralParam::ShowImGuiNode()
	{
		ImGui::DragFloat( u8"反射後の速度",		&reflectSpeed,  0.01f );
		ImGui::DragFloat( u8"反射角度(degree)",	&reflectDegree, 1.0f  );
		reflectSpeed  = std::max( 0.1f, reflectSpeed  );
	}
	void BasicParam::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		damage.ShowImGuiNode( u8"基本ダメージ設定" );
		ImGui::DragFloat3( u8"当たり判定・オフセット",				&hitBoxOffset.x,	0.01f );
		ImGui::DragFloat3( u8"当たり判定・AABBサイズ（半分を指定）",	&hitBoxSize.x,		0.01f );
		ImGui::DragFloat ( u8"当たり判定・Sphere半径",				&hitBoxSize.x,		0.01f );
		ImGui::Text( u8"（Sphereの場合はＸ成分が半径として使用されます）" );
		hitBoxSize.x = std::max( 0.0f, hitBoxSize.x );
		hitBoxSize.y = std::max( 0.0f, hitBoxSize.y );
		hitBoxSize.z = std::max( 0.0f, hitBoxSize.z );

		ImGui::TreePop();
	}
#endif // USE_IMGUI

	void Base::Init( const FireDesc &parameter )
	{
		model.Initialize( GetModelPtrOrNullptr( GetKind() ) );
		
		InitBody( parameter );

		velocity	= parameter.direction * parameter.initialSpeed;
		UpdateOrientation( parameter.direction );
		damage		= GetDamageParameter();
		if ( parameter.pAdditionalDamage )
		{
			damage.Combine( *parameter.pAdditionalDamage );
		}
		wantRemove	= false;
	}
	void Base::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreen )
	{
		body.UpdateIgnoreList( elapsedTime );

		if ( wasCollided )
		{
			CollidedProcess();
		}
		if ( wasProtected != ProtectedInfo::None && wasProtected != ProtectedInfo::Processed )
		{
			ProtectedProcess();
		}

		if ( OnOutSide( wsScreen ) )
		{
			wantRemove = true;
		}
	}
	void Base::PhysicUpdate( float elapsedTime, const Map &terrain )
	{
		const auto movement = velocity * elapsedTime;
		Solid::Move( movement, {}, {} );

		// Move() moves only the "body", so apply here.
		hitSphere.pos = body.pos;
	}
	void Base::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer			) { return; }
		if ( !model.pResource	) { return; }
		// else

		const bool useAABB = ( !hitSphere.exist );
		// I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
		const Donya::Vector3   &drawPos	= ( useAABB ) ? body.pos : hitSphere.pos;
		const Donya::Vector4x4 W		= MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );

		Donya::Model::Constants::PerModel::Common modelConstant{};
		modelConstant.drawColor			= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		modelConstant.worldMatrix		= W;
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
		constexpr Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 0.6f };
		constexpr Donya::Vector3 lightDir = -Donya::Vector3::Up();

		const bool useAABB		= ( !hitSphere.exist	);
		const bool useSphere	= ( !body.exist			);
		if ( !useAABB && !useSphere ) { return; }
		// else

		const Donya::Vector3 size	=
									( useAABB	) ? body.size * 2.0f :
									( useSphere	) ? hitSphere.radius * 2.0f :
									0.0f;
		const Donya::Vector3 wsPos	=
									( useAABB	) ? body.WorldPosition() :
									( useSphere	) ? hitSphere.WorldPosition() :
									0.0f;
		const Donya::Vector4x4 W	= MakeWorldMatrix( size, /* enableRotation = */ false, wsPos );

		if ( useAABB )
		{
			Donya::Model::Cube::Constant constant;
			constant.matWorld			= W;
			constant.matViewProj		= VP;
			constant.drawColor			= color;
			constant.lightDirection		= lightDir;
			pRenderer->ProcessDrawingCube( constant );
		}
		else
		{
			Donya::Model::Sphere::Constant constant;
			constant.matWorld			= W;
			constant.matViewProj		= VP;
			constant.drawColor			= color;
			constant.lightDirection		= lightDir;
			pRenderer->ProcessDrawingSphere( constant );
		}
	#endif // DEBUG_MODE
	}
	bool Base::Destructible() const
	{
		return false; // a derived class may returns true
	}
	bool Base::ShouldRemove() const
	{
		return wantRemove;
	}
	bool Base::OnOutSide( const Donya::Collision::Box3F &wsScreen ) const
	{
		// I am not consider an unused hit box(size or radius is zero) in here,
		// so I make to true an invalid one certainly.
		const bool outAABB		= ( body.size.IsZero()			|| !Donya::Collision::IsHit( body,		wsScreen, /* considerExistFlag = */ false ) );
		const bool outSphere	= ( IsZero( hitSphere.radius )	|| !Donya::Collision::IsHit( hitSphere,	wsScreen, /* considerExistFlag = */ false ) );
		return outAABB & outSphere; // Which one is true certainly, so I should combine it.
	}
	void Base::CollidedToObject( bool otherIsBroken ) const
	{
		const bool pierceable = Definition::Damage::Contain( Definition::Damage::Type::Pierce, damage.type );
		if ( otherIsBroken && pierceable ) { return; }
		// else

		wasCollided = true;
		// Donya::Sound::Play( Music::Bullet_Hit );
	}
	void Base::ProtectedBy( const Donya::Collision::Box3F &otherBody ) const
	{
		const auto myCenter		= ( hitSphere.exist ) ? hitSphere.WorldPosition( orientation ) : body.WorldPosition( orientation );
		const auto otherMax		= otherBody.Max( orientation );
		const auto otherMin		= otherBody.Min( orientation );
		const float distLeft	= otherMin.x - myCenter.x;
		const float distRight	= otherMax.x - myCenter.x;
		ProtectedByImpl( distLeft, distRight );
	}
	void Base::ProtectedBy( const Donya::Collision::Sphere3F &otherBody ) const
	{
		const auto myCenter		= ( hitSphere.exist ) ? hitSphere.WorldPosition() : body.WorldPosition();
		const auto otherCenter	= otherBody.WorldPosition();
		const auto otherRight	= otherCenter.x + otherBody.radius;
		const auto otherLeft	= otherCenter.x - otherBody.radius;
		const float distLeft	= otherLeft  - myCenter.x;
		const float distRight	= otherRight - myCenter.x;
		ProtectedByImpl( distLeft, distRight );
	}
	void Base::ProtectedByImpl( float distLeft, float distRight ) const
	{
		if ( wasProtected == ProtectedInfo::Processed ) { return; }
		// else

		bool fromRightSide{};
		if ( 0.0f <= distLeft  ) { fromRightSide = true;  }
		else
		if ( distRight <= 0.0f ) { fromRightSide = false; }
		else
		{
			/*
			|	<-distLeft->	|	<-distRight->	|
			Min				myCenter				Max
			*/
			fromRightSide = ( fabsf( distLeft ) <= fabsf( distRight ) );
		}

		wasProtected	= ( fromRightSide )
						? ProtectedInfo::ByRightSide
						: ProtectedInfo::ByLeftSide;
	}
	Donya::Collision::Sphere3F Base::GetHitSphere() const
	{
		Donya::Collision::Sphere3F tmp = hitSphere;
		tmp.offset = orientation.RotateVector( tmp.offset );
		return tmp;
	}
	Definition::Damage Base::GetDamage() const
	{
		return damage;
	}
	void Base::SetWorldPosition( const Donya::Vector3 &wsPos )
	{
		hitSphere.pos = body.pos = wsPos;
	}
	void Base::SetVelocity( const Donya::Vector3 &newVelocity )
	{
		velocity = newVelocity;
	}
	void Base::CollidedProcess()
	{
		wasCollided		= false;
		wantRemove		= true;
		body.exist		= false;
		hitSphere.exist	= false;
	}
	void Base::ProtectedProcess()
	{
		wasCollided		= false;
		body.exist		= false;
		hitSphere.exist	= false;

		const auto &data		= Parameter::GetGeneral();
		const bool fromRight	= wasProtected == ProtectedInfo::ByRightSide;
		const float directionRadian	= ( fromRight )
									? ToRadian( 180.0f - data.reflectDegree )
									: ToRadian( data.reflectDegree );
		const float cos = cosf( directionRadian );
		const float sin = sinf( directionRadian );

		velocity.x = cos * data.reflectSpeed;
		velocity.y = sin * data.reflectSpeed;
		velocity.z = 0.0f;
		// The orientation is not change

		wasProtected	= ProtectedInfo::Processed;
	}
	void Base::InitBody( const FireDesc &parameter )
	{
		// Default hit box is used as AABB
		body.exist			= true;
		hitSphere.radius	= 0.0f;
		hitSphere.exist		= false;
		AssignBodyParameter( parameter.position );
		body.id				= Donya::Collision::GetUniqueID();
		body.ownerID		= parameter.owner;
		body.ignoreList.clear();
		hitSphere.id		= body.id;
		hitSphere.ownerID	= body.ownerID;
		hitSphere.ignoreList.clear();
	}
	void Base::UpdateOrientation( const Donya::Vector3 &direction )
	{
		if ( direction.IsZero() ) { return; }
		// else
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
	void Admin::PhysicUpdate( float elapsedTime, const Map &terrain )
	{
		// TODO: Should detect a remove sign and erase that in here?

		for ( auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->PhysicUpdate( elapsedTime, terrain );

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
			case Kind::Buster:		tmp = std::make_shared<Buster>();		break;
			case Kind::SkullBuster:	tmp = std::make_shared<SkullBuster>();	break;
			case Kind::SkullShield:	tmp = std::make_shared<SkullShield>();	break;
			case Kind::SuperBall:	tmp = std::make_shared<SuperBall>();	break;
			default: _ASSERT_EXPR( 0, L"Error: Unexpected bullet kind!" );	return;
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
