#include "TogeheroBody.h"

#include "../Donya/Sound.h"
#include "../Donya/Useful.h"	// Use SignBitF()

#include "../Effect/EffectAdmin.h"
#include "../Effect/EffectKind.h"
#include "../Item.h"
#include "../Music.h"
#include "../Parameter.h"

namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<TogeheroBodyParam> togeheroBodyParam{ "TogeheroBody", "Bullet/" };

		const TogeheroBodyParam &GetTogeheroBody()
		{
			return togeheroBodyParam.Get();
		}

		namespace Impl
		{
			void LoadTogeheroBody()
			{
				togeheroBodyParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateTogeheroBody( const std::string &nodeCaption )
			{
				togeheroBodyParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}


	void TogeheroBody::Init( const FireDesc &parameter )
	{
		Base::Init( parameter );

		zigzagTimer			= 0.0f;
		wantRemoveByOutSide	= false;
		UpdateVerticalVelocity( /* halfWay = */ true );
	}
	void TogeheroBody::Uninit()
	{
		if ( !wantRemoveByOutSide )
		{
			Item::DropItemByLottery( GetPosition() );
		}
	}
	void TogeheroBody::Update( float deltaTime, const Donya::Collision::Box3F &wsScreenHitBox )
	{
		Base::Update( deltaTime, wsScreenHitBox );

		const auto &data = Parameter::GetTogeheroBody();

		zigzagTimer += deltaTime;
		if ( data.zigzagInterval <= zigzagTimer )
		{
			zigzagTimer = 0.0f;
			UpdateVerticalVelocity();
		}

		const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( data.rotateSpeed * deltaTime ) );
		orientation.RotateBy( rotation );

		UpdateMotionIfCan( deltaTime * data.basic.animePlaySpeed, 0 );
	}
	void TogeheroBody::UpdateVerticalVelocity( bool halfWay )
	{
		const auto &data = Parameter::GetTogeheroBody();
		const float distance = ( halfWay ) ? data.zigzagDistance * 0.5f : data.zigzagDistance;
		const float interval = ( halfWay ) ? data.zigzagInterval * 0.5f : data.zigzagInterval;

		if ( IsZero( interval ) )
		{
			_ASSERT_EXPR( 0, L"Error: Divide by zero!" );
			velocity.y = 0.0f;
			return;
		}
		// else

		const float oldSign = Donya::SignBitF( velocity.y );
		velocity.y = distance / interval;
		velocity.y *= ( IsZero( oldSign ) ) ? 1.0f : -oldSign;
	}
	bool TogeheroBody::Destructible() const
	{
		return true;
	}
	bool TogeheroBody::WasProtected() const
	{
		return false; // Not available
	}
	void TogeheroBody::CollidedToObject( bool otherIsBroken, bool otherIsBullet ) const
	{
		// Except if the other is the body of target
		if ( otherIsBullet )
		{
			// Always behaves as defeted
			Base::CollidedToObject( /* otherIsBroken = */ false, otherIsBullet );
			wasCollided = true;
		}
	}
	void TogeheroBody::ProtectedBy( const Donya::Collision::Box3F &protectObjectBody ) const
	{
		ProtectedByImpl( 0.0f, 0.0f );
	}
	void TogeheroBody::ProtectedBy( const Donya::Collision::Sphere3F &protectObjectBody ) const
	{
		ProtectedByImpl( 0.0f, 0.0f );
	}
	void TogeheroBody::ProtectedByImpl( float distLeft, float distRight ) const
	{
		// Use the same reaction of otherIsBullet case
		CollidedToObject( /* otherIsBroken = */ false, /* otherIsBullet = */ true );
	}
	void TogeheroBody::ProcessOnOutSide()
	{
		Base::ProcessOnOutSide();
		wantRemoveByOutSide = true;
	}
	Kind TogeheroBody::GetKind() const
	{
		return Kind::TogeheroBody;
	}
	void TogeheroBody::GenerateCollidedEffect() const
	{
		Effect::Admin::Get().GenerateInstance( Effect::Kind::DefeatEnemy_Small, GetPosition() );
	}
	void TogeheroBody::GenerateProtectedEffect() const
	{
		Effect::Admin::Get().GenerateInstance( Effect::Kind::DefeatEnemy_Small, GetPosition() );
	}
	void TogeheroBody::PlayCollidedSE() const
	{
		// No op
	}
	void TogeheroBody::PlayProtectedSE() const
	{
		PlayCollidedSE();
	}
	void TogeheroBody::CollidedProcess()
	{
		Base::CollidedProcess();
	}
	void TogeheroBody::ProtectedProcess()
	{
		CollidedProcess();
	}
	Definition::Damage TogeheroBody::GetDamageParameter() const
	{
		return Parameter::GetTogeheroBody().basic.damage;
	}
	void TogeheroBody::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetTogeheroBody().basic;
		body.pos	= wsPos;
		body.offset	= orientation.RotateVector( data.hitBoxOffset );
		body.size	= data.hitBoxSize;
	}
#if USE_IMGUI
	void TogeheroBodyParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"汎用設定" );
		ImGui::DragFloat( u8"回転速度[degree/s]",	&rotateSpeed,		1.0f  );
		ImGui::DragFloat( u8"ジグザグ軌道の距離",		&zigzagDistance,	0.01f );
		ImGui::DragFloat( u8"片道にかける（秒数）",	&zigzagInterval,	0.01f );
		zigzagDistance = std::max( 0.01f, zigzagDistance );
		zigzagInterval = std::max( 0.01f, zigzagInterval );
	}
#endif // USE_IMGUI

}
