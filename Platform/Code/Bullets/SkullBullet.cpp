#include "SkullBullet.h"

#include "../Donya/RenderingStates.h"
#include "../Donya/Sound.h"

#include "../Common.h"
#include "../Effect/EffectAdmin.h"
#include "../Effect/EffectKind.h"
#include "../Music.h"
#include "../Parameter.h"

namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<SkullBusterParam> skullBusterParam{ "SkullBuster", "Bullet/" };
		static ParamOperator<SkullShieldParam> skullShieldParam{ "SkullShield", "Bullet/" };

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
	void SkullBuster::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox )
	{
		Base::Update( elapsedTime, wsScreenHitBox );

		const auto &data = Parameter::GetSkullBuster();
		UpdateMotionIfCan( elapsedTime * data.basic.animePlaySpeed, 0 );
	}
	Kind SkullBuster::GetKind() const
	{
		return Kind::SkullBuster;
	}
	void SkullBuster::GenerateCollidedEffect() const
	{
		// No op
	}
	void SkullBuster::PlayCollidedSE() const
	{
		Donya::Sound::Play( Music::Bullet_HitBuster );
	}
	Definition::Damage SkullBuster::GetDamageParameter() const
	{
		return Parameter::GetSkullBuster().basic.damage;
	}
	void SkullBuster::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetSkullBuster().basic;
		body.pos	= wsPos;
		body.offset	= orientation.RotateVector( data.hitBoxOffset );
		body.size	= data.hitBoxSize;
	}
#if USE_IMGUI
	void SkullBusterParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"îƒópê›íË" );
	}
#endif // USE_IMGUI


	void SkullShield::Init( const FireDesc &parameter )
	{
		Base::Init( parameter );

		orientation		= Donya::Quaternion::Identity();

		currentDegree	= 0.0f;
		rotateSign		= ( parameter.direction.x < 0.0f ) ? +1.0f : -1.0f;
	}
	void SkullShield::Uninit() {} // No op
	void SkullShield::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox )
	{
		Base::Update( elapsedTime, wsScreenHitBox );

		const auto &data = Parameter::GetSkullShield();
		currentDegree += data.rotateDegree * rotateSign * elapsedTime;

		UpdateMotionIfCan( elapsedTime * data.basic.animePlaySpeed, 0 );
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
	void SkullShield::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

	#if DEBUG_MODE
		constexpr Donya::Vector4 hitColor{ 0.4f, 0.8f, 0.9f, 0.6f };
		constexpr Donya::Vector4 subColor{ 0.3f, 0.3f, 0.1f, 0.6f };
		constexpr Donya::Vector3 lightDir = -Donya::Vector3::Up();

		Donya::Model::Sphere::Constant constant;
		constant.matViewProj		= VP;
		constant.lightDirection		= lightDir;

		const auto wsPos = hitSphere.WorldPosition();
		Donya::Vector4x4 baseWorld = Donya::Vector4x4::MakeTranslation( wsPos.x, wsPos.y, wsPos.z );

		pRenderer->ActivateShaderSphere();
		pRenderer->ActivateRasterizerSphere();
		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::NoTest_Write );
		
		auto Draw = [&]( float radius, const Donya::Vector4 &color )
		{
			baseWorld._11 = radius * 2.0f;
			baseWorld._22 = radius * 2.0f;
			baseWorld._33 = radius * 2.0f;
			constant.matWorld	= baseWorld;
			constant.drawColor	= color;
			pRenderer->UpdateConstant( constant );
			pRenderer->ActivateConstantSphere();

			pRenderer->DrawSphere();

			pRenderer->DeactivateConstantSphere();
		};
		Draw( hitSphere.radius, hitColor );
		Draw( Parameter::GetSkullShield().subtractorRadius, subColor );

		Donya::DepthStencil::Deactivate();
		pRenderer->DeactivateRasterizerSphere();
		pRenderer->DeactivateShaderSphere();
	#endif // DEBUG_MODE
	}
	Donya::Collision::Sphere3F SkullShield::GetHitSphereSubtractor() const
	{
		Donya::Collision::Sphere3F tmp = hitSphere;
		tmp.radius = Parameter::GetSkullShield().subtractorRadius;
		tmp.exist  = true; // It represents the subtractor is valid
		return tmp;
	}
	Kind SkullShield::GetKind() const
	{
		return Kind::SkullShield;
	}
	void SkullShield::GenerateCollidedEffect() const
	{
		Effect::Admin::Get().GenerateInstance( Effect::Kind::Hit_SkullShield, GetPosition() );
	}
	void SkullShield::PlayCollidedSE() const
	{
		Donya::Sound::Play( Music::Bullet_HitShield );
	}
	Definition::Damage SkullShield::GetDamageParameter() const
	{
		return Parameter::GetSkullShield().basic.damage;
	}
	void SkullShield::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data	= Parameter::GetSkullShield().basic;
		hitSphere.pos		= wsPos;
		hitSphere.offset	= orientation.RotateVector( data.hitBoxOffset );
		hitSphere.radius	= data.hitBoxSize.x;

		// Only enable sphere hit box
		hitSphere.exist	= true;
		body.size		= 0.0f;
		body.exist		= false;
	}
#if USE_IMGUI
	void SkullShieldParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"îƒópê›íË" );
		ImGui::DragFloat ( u8"âÒì]äpìx(degree)",		&rotateDegree,		1.0f );
		ImGui::DragFloat ( u8"ÉpÅ[Écà íuÇÃîºåa",		&drawPartOffset,	0.01f );

		ImGui::DragFloat ( u8"å∏éZåãçáîªíËÇÃîºåa",	&subtractorRadius,	0.01f );
		subtractorRadius = std::max( 0.0f, subtractorRadius );
	}
#endif // USE_IMGUI
}
