#include "SkullBullet.h"

#include "../Parameter.h"

namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<SkullBusterParam> skullBusterParam{ "SkullBuster" };
		static ParamOperator<SkullShieldParam> skullShieldParam{ "SkullShield" };

		const SkullBusterParam &GetSkullBuster()
		{
			return skullBusterParam.Get();
		}
		const SkullShieldParam &GetSkullShield()
		{
			return skullShieldParam.Get();
		}

		namespace Impl
		{
			void LoadSkullBuster()
			{
				skullBusterParam.LoadParameter();
			}
			void LoadSkullShield()
			{
				skullShieldParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateSkullBuster( const std::string &nodeCaption )
			{
				skullBusterParam.ShowImGuiNode( nodeCaption );
			}
			void UpdateSkullShield( const std::string &nodeCaption )
			{
				skullShieldParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}


	void SkullBuster::Uninit() {} // No op
	Kind SkullBuster::GetKind() const
	{
		return Kind::SkullBuster;
	}
	Definition::Damage SkullBuster::GetDamageParameter() const
	{
		return Parameter::GetSkullBuster().damage;
	}
	void SkullBuster::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetSkullBuster();
		body.pos	= wsPos;
		body.offset	= data.hitBoxOffset;
		body.size	= data.hitBoxSize;
	}
#if USE_IMGUI
	void SkullBusterParam::ShowImGuiNode()
	{
		ImGui::DragFloat3( u8"当たり判定・オフセット",			&hitBoxOffset.x,	0.01f );
		ImGui::DragFloat3( u8"当たり判定・サイズ（半分を指定）",	&hitBoxSize.x,		0.01f );
		hitBoxSize.x = std::max( 0.0f, hitBoxSize.x );
		hitBoxSize.y = std::max( 0.0f, hitBoxSize.y );
		hitBoxSize.z = std::max( 0.0f, hitBoxSize.z );
		damage.ShowImGuiNode( u8"基本ダメージ設定" );
	}
#endif // USE_IMGUI


	void SkullShield::Init( const FireDesc &parameter )
	{
		Base::Init( parameter );

		orientation = Donya::Quaternion::Identity();
	}
	void SkullShield::Uninit() {} // No op
	void SkullShield::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox )
	{
		Base::Update( elapsedTime, wsScreenHitBox );

		currentDegree += Parameter::GetSkullShield().rotateDegree * elapsedTime;
	}
	void SkullShield::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer			) { return; }
		if ( !model.pResource	) { return; }
		// else

		Donya::Model::Constants::PerModel::Common modelConstant{};
		modelConstant.drawColor = Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		
		const auto &data = Parameter::GetSkullShield();

		const float				baseRadian	= ToRadian( currentDegree );
		const float				rotRadian	= ToRadian( 360.0f / std::max( 1, data.partCount ) );
		const Donya::Vector3	axis		= orientation.LocalFront();
		const Donya::Vector3	offset		= orientation.LocalUp() * data.drawPartOffset;
		const Donya::Vector3	wsPos		= hitSphere.pos; // I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.

		Donya::Vector3			drawPos;
		Donya::Quaternion		rotation;
		Donya::Quaternion		rotated;
		for ( int i = 0; i < data.partCount; ++i )
		{
			rotation = Donya::Quaternion::Make( axis, ( rotRadian * i ) + baseRadian );
			rotated  = orientation.Rotated( rotation );
			drawPos  = wsPos + rotated.RotateVector( offset );

			modelConstant.worldMatrix = MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );;
			pRenderer->UpdateConstant( modelConstant );
			pRenderer->ActivateConstantModel();

			pRenderer->Render( model.pResource->model, model.pose );

			pRenderer->DeactivateConstantModel();
		}
	}
	Kind SkullShield::GetKind() const
	{
		return Kind::SkullShield;
	}
	Definition::Damage SkullShield::GetDamageParameter() const
	{
		return Parameter::GetSkullShield().damage;
	}
	void SkullShield::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data	= Parameter::GetSkullShield();
		hitSphere.pos		= wsPos;
		hitSphere.offset	= data.hitBoxOffset;
		hitSphere.radius	= data.hitBoxRadius;

		// Only enable sphere hit box
		hitSphere.exist	= true;
		body.size		= 0.0f;
		body.exist		= false;
	}
#if USE_IMGUI
	void SkullShieldParam::ShowImGuiNode()
	{
		ImGui::DragFloat ( u8"回転角度(degree)",			&rotateDegree,		1.0f );
		ImGui::DragFloat ( u8"パーツ位置の半径",			&drawPartOffset,	0.01f );
		ImGui::DragFloat3( u8"当たり判定・オフセット",	&hitBoxOffset.x,	0.01f );
		ImGui::DragFloat ( u8"当たり判定・半径",			&hitBoxRadius,		0.01f );
		hitBoxRadius = std::max( 0.0f, hitBoxRadius );
		damage.ShowImGuiNode( u8"基本ダメージ設定" );
	}
#endif // USE_IMGUI
}
