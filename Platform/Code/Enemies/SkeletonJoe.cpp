#include "SkeletonJoe.h"

#include "../Bullet.h"
#include "../Bullets/Bone.h"
#include "../Donya/Sound.h"
#include "../Music.h"
#include "../Parameter.h"

namespace Enemy
{
	namespace Parameter
	{
		static ParamOperator<SkeletonJoeParam> skeletonJoeParam{ "SkeletonJoe", "Enemy/" };
		const SkeletonJoeParam &GetSkeletonJoe()
		{
			return skeletonJoeParam.Get();
		}

		namespace Impl
		{
			void LoadSkeletonJoe()
			{
				skeletonJoeParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateSkeletonJoe( const std::string &nodeCaption )
			{
				skeletonJoeParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	namespace
	{
		constexpr size_t motionCount = scast<size_t>( SkeletonJoe::MotionKind::MotionCount );
		constexpr const char *GetMotionName( SkeletonJoe::MotionKind kind )
		{
			switch ( kind )
			{
			case SkeletonJoe::MotionKind::Idle:			return u8"Prepare";
			case SkeletonJoe::MotionKind::Fire:			return u8"Fire";
			case SkeletonJoe::MotionKind::Break:		return u8"Break";
			case SkeletonJoe::MotionKind::ReAssemble:	return u8"ReAssemble";
			default: break;
			}

			return "ERROR_KIND";
		}
	}

	void SkeletonJoe::Init( const InitializeParam &parameter, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox )
	{
		Base::Init( parameter, wsTargetPos, wsScreenHitBox );
		ChangeState( MotionKind::Idle );
	}
	void SkeletonJoe::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		Base::Update( elapsedTime, wsTargetPos, wsScreen );
		if ( NowWaiting() ) { return; }
		// else

	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		AssignMyBody( body.pos );
	#endif // USE_IMGUI

		const auto &data = Parameter::GetSkeletonJoe();
		const size_t stateIndex = std::min( scast<size_t>( status ), motionCount );

		timer += elapsedTime;
		if ( data.stateSeconds[stateIndex] <= timer )
		{
			ToNextState( wsTargetPos );
		}

		velocity.y -= data.gravity * elapsedTime;

		UpdateOrientation( ( wsTargetPos - GetPosition() ).Unit() );

		const float motionAcceleration = data.animePlaySpeeds[stateIndex];
		UpdateMotionIfCan( elapsedTime * motionAcceleration, stateIndex );
	}
	Kind SkeletonJoe::GetKind() const { return Kind::SkeletonJoe; }
	Definition::Damage SkeletonJoe::GetTouchDamage() const
	{
		return Parameter::GetSkeletonJoe().basic.touchDamage;
	}
	int  SkeletonJoe::GetInitialHP() const
	{
		return Parameter::GetSkeletonJoe().basic.hp;
	}
	void SkeletonJoe::AssignMyBody( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetSkeletonJoe().basic;
		body.pos		= wsPos;
		body.offset		= data.hitBoxOffset;
		body.size		= data.hitBoxSize;
		hurtBox.pos		= wsPos;
		hurtBox.offset	= data.hurtBoxOffset;
		hurtBox.size	= data.hurtBoxSize;

		body.offset		= orientation.RotateVector( body.offset		);
		hurtBox.offset	= orientation.RotateVector( hurtBox.offset	);
	}
	void SkeletonJoe::UpdateOrientation( const Donya::Vector3 &direction )
	{
		if ( direction.IsZero() ) { return; }
		// else
		
		orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), direction );

		body.offset		= orientation.RotateVector( body.offset		);
		hurtBox.offset	= orientation.RotateVector( hurtBox.offset	);
	}
	void SkeletonJoe::ChangeMotion( MotionKind nextKind )
	{
		status = nextKind;

		model.animator.ResetTimer();

		( nextKind == MotionKind::Idle )
		? model.animator.EnableLoop()
		: model.animator.DisableLoop();

		model.AssignMotion( scast<int>( nextKind ) );
	}
	void SkeletonJoe::ChangeState( MotionKind nextKind )
	{
		const size_t intNext = scast<size_t>( nextKind );
		if ( motionCount <= intNext ) { return; }
		// else

		timer = 0.0f;
		ChangeMotion( nextKind );
	}
	void SkeletonJoe::ToNextState( const Donya::Vector3 &wsTargetPos )
	{
		MotionKind next = MotionKind::MotionCount;

		switch ( status )
		{
		case MotionKind::Idle:
			next = MotionKind::Fire;
			break;
		case MotionKind::Fire:
			Shot( wsTargetPos );
			next = MotionKind::Idle;
			break;
		case MotionKind::Break:
			next = MotionKind::ReAssemble;
			break;
		case MotionKind::ReAssemble:
			next = MotionKind::Idle;
			break;
		default: return;
		}

		ChangeState( next );
	}
	void SkeletonJoe::Shot( const Donya::Vector3 &wsTargetPos )
	{
		// Using formula by: https://keisan.casio.jp/exec/system/1204505808

		const auto &data = Parameter::GetSkeletonJoe();

		const float distance = ( wsTargetPos - GetPosition() ).Length();
		const float &gravity = Bullet::Parameter::GetBone().gravity;

		const float t = data.impactSecond;
		if ( IsZero( t ) )
		{
			_ASSERT_EXPR( 0, L"Error: Division by zero!" );
			return;
		}
		// else

		const float gt = gravity * t;
		const float rt = 1.0f / t;

		// v = sqrt( pow( 1/t, 2 ) + pow( gt/2, 2 ) )
		const float hGT = gt * 0.5f; // Half g*t
		const float speed = sqrtf( (rt*rt) + (hGT*hGT) );

		// theta = atan( pow( gt,2 ) / 2t )
		const float theta = atanf( (gt*gt) / (t*t) );

		Bullet::FireDesc desc = data.fireDesc;
		desc.position	=  orientation.RotateVector( desc.position ); // Rotate the local space offset
		desc.position	+= GetPosition(); // Convert to world space
		desc.owner		=  hurtBox.id;
		desc.direction.x = cosf( theta );
		desc.direction.y = sinf( theta );
		desc.direction.z = 0.0f;

		Bullet::Admin::Get().RequestFire( desc );
		ChangeMotion( MotionKind::Fire );

		// TODO: Make shot SE
		// Donya::Sound::Play( Music::SkeletonJoe_Shot );
	}
#if USE_IMGUI
	bool SkeletonJoe::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else

		Base::ShowImGuiNode( u8"基底部分" );

		if ( ImGui::TreeNode( u8"派生部分" ) )
		{
			ImGui::DragFloat( u8"内部タイマー", &timer, 0.01f );

			ImGui::TreePop();
		}

		ImGui::TreePop();
		return true;
	}
	void SkeletonJoeParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"汎用設定" );
		fireDesc.ShowImGuiNode( u8"発射弾設定" );

		if ( stateSeconds.size() != motionCount ) { stateSeconds.resize( motionCount, 1.0f ); }
		if ( animePlaySpeeds.size() != motionCount ) { animePlaySpeeds.resize( motionCount, 1.0f ); }
		if ( ImGui::TreeNode( u8"各ステートの滞在時間" ) )
		{
			for ( size_t i = 0; i < motionCount; ++i )
			{
				ImGui::DragFloat
				(
					GetMotionName( scast<SkeletonJoe::MotionKind>( i ) ),
					&stateSeconds[i], 0.01f
				);
				stateSeconds[i] = std::max( 0.0f, stateSeconds[i] );
			}

			ImGui::TreePop();
		}
		if ( ImGui::TreeNode( u8"モーション再生速度" ) )
		{
			for ( size_t i = 0; i < motionCount; ++i )
			{
				ImGui::DragFloat
				(
					GetMotionName( scast<SkeletonJoe::MotionKind>( i ) ),
					&animePlaySpeeds[i], 0.01f
				);
			}

			ImGui::TreePop();
		}

		ImGui::DragFloat( u8"重力",				&gravity,		0.01f );
		ImGui::DragFloat( u8"着弾にかかる秒数",	&impactSecond,	0.01f );
		gravity			= std::max( 0.0f,  gravity		);
		impactSecond	= std::max( 0.01f, impactSecond	);
	}
#endif // USE_IMGUI
}
