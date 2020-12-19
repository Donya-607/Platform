#include "Bone.h"

#include "../Donya/Sound.h"

#include "../Effect/EffectAdmin.h"
#include "../Effect/EffectKind.h"
#include "../Music.h"
#include "../Parameter.h"

namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<BoneParam> boneParam{ "Bone", "Bullet/" };

		const BoneParam &GetBone()
		{
			return boneParam.Get();
		}

		namespace Impl
		{
			void LoadBone()
			{
				boneParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateBone( const std::string &nodeCaption )
			{
				boneParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}


	void Bone::Uninit() {} // No op
	void Bone::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox )
	{
		Base::Update( elapsedTime, wsScreenHitBox );

		const auto &data = Parameter::GetBone();

		velocity.y -= data.gravity * elapsedTime;
		if ( !velocity.IsZero() )
		{
			UpdateOrientation( velocity.Unit() );
		}

		UpdateMotionIfCan( elapsedTime * data.basic.animePlaySpeed, 0 );
	}
	Kind Bone::GetKind() const
	{
		return Kind::Bone;
	}
	void Bone::GenerateCollidedEffect() const
	{
		Effect::Admin::Get().GenerateInstance( Effect::Kind::DefeatEnemy_Small, GetPosition() );
	}
	void Bone::PlayCollidedSE() const
	{
		// Donya::Sound::Play( Music::Bullet_HitBone ); TODO
	}
	Definition::Damage Bone::GetDamageParameter() const
	{
		return Parameter::GetBone().basic.damage;
	}
	void Bone::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data	= Parameter::GetBone().basic;
		hitSphere.pos		= wsPos;
		hitSphere.offset	= orientation.RotateVector( data.hitBoxOffset );
		hitSphere.radius	= data.hitBoxSize.x;

		// Only enable sphere hit box
		hitSphere.exist	= true;
		body.pos		= wsPos;
		body.size		= 0.0f;
		body.exist		= false;
	}
#if USE_IMGUI
	void BoneParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"îƒópê›íË" );

		ImGui::DragFloat( u8"èdóÕ", &gravity, 0.01f );
		gravity = std::max( 0.0f, gravity );
	}
#endif // USE_IMGUI

}
