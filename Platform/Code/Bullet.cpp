#include "Bullet.h"

#include <algorithm>	// Use std::remove_if

#include "BulletParam.h"
#include "Common.h"		// Use IsShowCollision()
#include "FilePath.h"
#include "ModelHelper.h"
#include "Parameter.h"

namespace
{
	constexpr const char *modelName = "Bullet/Buster"; // Model of Buster
	static std::unique_ptr<ModelHelper::StaticSet> pModel{}; // Model of Buster

	bool LoadModel()
	{
		// Already has loaded.
		if ( pModel ) { return true; }
		// else

		const auto filePath = MakeModelPath( modelName );

		if ( !Donya::IsExistFile( filePath ) )
		{
			Donya::OutputDebugStr( "Error : The Bullet::Buster's model file does not exist." );
			return false;
		}
		// else

		pModel = std::make_unique<ModelHelper::StaticSet>();
		const bool result = ModelHelper::Load( filePath, pModel.get() );
		if ( !result )
		{
			pModel.reset();
			return false;
		}
		// else
		return true;
	}
	const ModelHelper::StaticSet *GetModelPtrOrNullptr()
	{
		if ( !pModel )
		{
			_ASSERT_EXPR( 0, L"Error: The Bullet::Buster's model is not initialized!" );
			return nullptr;
		}
		// else

		return pModel.get();
	}
}
namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<BusterParam> busterParam{ "Buster" };

		const BusterParam &GetBuster()
		{
			return busterParam.Get();
		}
	#if USE_IMGUI
		void Update( const std::string &nodeCaption )
		{
			busterParam.ShowImGuiNode( nodeCaption );
		}
	#endif // USE_IMGUI
	}

	bool LoadResource()
	{
		Parameter::busterParam.LoadParameter();

		return ::LoadModel();
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

	int  Buster::livingCount = 0;
	int  Buster::GetLivingCount()
	{
		return livingCount;
	}
	void Buster::Init( const FireDesc &parameter )
	{
		livingCount++;

		const auto &data = Parameter::GetBuster();

		body.pos	= parameter.position;
		body.offset	= data.hitBoxOffset;
		body.size	= data.hitBoxSize;

		velocity	= parameter.direction * parameter.initialSpeed;
		orientation	= Donya::Quaternion::LookAt( Donya::Vector3::Front(), parameter.direction );
		wantRemove	= false;
	}
	void Buster::Uninit()
	{
		livingCount--;
	}
	void Buster::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreen )
	{
		if ( OnOutSide( wsScreen ) )
		{
			wantRemove = true;
		}
	}
	void Buster::PhysicUpdate( float elapsedTime )
	{
		const auto movement = velocity * elapsedTime;
		Solid::Move( movement, {}, {} );
	}
	void Buster::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer ) { return; }
		// else

		const ModelHelper::StaticSet *pModelSet = GetModelPtrOrNullptr();
		if ( !pModelSet ) { return; }
		// else

		const auto model = *pModelSet;

		const Donya::Vector3   &drawPos = body.pos; // I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
		const Donya::Vector4x4 W		= MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );

		Donya::Model::Constants::PerModel::Common modelConstant{};
		modelConstant.drawColor			= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		modelConstant.worldMatrix		= W;;
		pRenderer->UpdateConstant( modelConstant );
		pRenderer->ActivateConstantModel();

		pRenderer->Render( model.model, model.pose );

		pRenderer->DeactivateConstantModel();
	}
	void Buster::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

	#if DEBUG_MODE
		Donya::Model::Cube::Constant constant;
		constant.matWorld			= MakeWorldMatrix( body.size * 2.0f, /* enableRotation = */ false, body.WorldPosition() );
		constant.matViewProj		= VP;
		constant.drawColor			= Donya::Vector4{ 1.0f, 1.0f, 0.0f, 0.6f };
		constant.lightDirection		= -Donya::Vector3::Up();

		pRenderer->ProcessDrawingCube( constant );
	#endif // DEBUG_MODE
	}
	bool Buster::ShouldRemove() const
	{
		return wantRemove;
	}
	bool Buster::OnOutSide( const Donya::Collision::Box3F &wsScreen ) const
	{
		return ( !Donya::Collision::IsHit( body, wsScreen ) );
	}
	Donya::Vector4x4 Buster::MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const
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
	void Buster::ShowImGuiNode( const std::string &nodeCaption )
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
	void BusterParam::ShowImGuiNode()
	{
		ImGui::DragFloat3( u8"Buster-当たり判定・オフセット（半分を指定）",	&hitBoxOffset.x,	0.01f );
		ImGui::DragFloat3( u8"Buster-当たり判定・サイズ（半分を指定）",	&hitBoxSize.x,		0.01f );
		hitBoxSize.x = std::max( 0.0f, hitBoxSize.x );
		hitBoxSize.y = std::max( 0.0f, hitBoxSize.y );
		hitBoxSize.z = std::max( 0.0f, hitBoxSize.z );
	}
#endif // USE_IMGUI


	void Admin::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreen )
	{
		GenerateRequestedFires();
		generateRequests.clear();

		for ( auto &it : bullets )
		{
			it.Update( elapsedTime, wsScreen );

			if ( it.ShouldRemove() )
			{
				it.Uninit(); // It will be removed at RemoveInstancesIfNeeds(), so we should finalize here.
			}
		}

		RemoveInstancesIfNeeds();
	}
	void Admin::PhysicUpdate( float elapsedTime )
	{
		// TODO: Should detect a remove sign and erase that in here?

		for ( auto &it : bullets )
		{
			it.PhysicUpdate( elapsedTime );

			if ( it.ShouldRemove() )
			{
				it.Uninit();
			}
		}

		RemoveInstancesIfNeeds();
	}
	void Admin::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer ) { return; }
		// else
		for ( const auto &it : bullets )
		{
			it.Draw( pRenderer );
		}
	}
	void Admin::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !pRenderer ) { return; }
		// else
		for ( const auto &it : bullets )
		{
			it.DrawHitBox( pRenderer, VP );
		}
	}
	void Admin::ClearInstances()
	{
		for ( auto &it : bullets )
		{
			it.Uninit();
		}
		bullets.clear();
	}
	void Admin::RequestFire( const FireDesc &parameter )
	{
		generateRequests.emplace_back( parameter );
	}
	void Admin::GenerateRequestedFires()
	{
		for ( const auto &it : generateRequests )
		{
			Buster tmp{};
			tmp.Init( it );
			bullets.emplace_back( std::move( tmp ) );
		}
	}
	void Admin::RemoveInstancesIfNeeds()
	{
		auto result = std::remove_if
		(
			bullets.begin(), bullets.end(),
			[]( Buster &element )
			{
				return element.ShouldRemove();
			}
		);
		bullets.erase( result, bullets.end() );
	}
#if USE_IMGUI
	void Admin::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::TreeNode( u8"実体操作" ) )
		{
			std::string caption;
			const size_t count = bullets.size();
			for ( size_t i = 0; i < count; ++i )
			{
				caption =  "[";
				if ( i < 100 ) { caption += "_"; } // Align
				if ( i < 10  ) { caption += "_"; } // Align
				caption += std::to_string( i );
				caption += "]";
				bullets[i].ShowImGuiNode( caption );
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI
}
