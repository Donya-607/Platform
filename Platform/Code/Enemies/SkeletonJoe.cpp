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
		size_t stateIndex = std::min( scast<size_t>( status ), motionCount );

		if ( !data.stateSeconds.empty() )
		{
			timer += elapsedTime;
			if ( data.stateSeconds[stateIndex] <= timer )
			{
				ToNextState( wsTargetPos );
				stateIndex = std::min( scast<size_t>( status ), motionCount );
			}
		}

		velocity.y -= data.gravity * elapsedTime;

		if ( status == MotionKind::Idle )
		{
			constexpr Donya::Vector3 right = Donya::Vector3::Right();
			const Donya::Vector3 toTarget = wsTargetPos - GetPosition();
			UpdateOrientation( ( toTarget.x < 0.0f ) ? -right : right );
		}

		const float motionAcceleration = ( data.animePlaySpeeds.size() <= stateIndex ) ? 1.0f : data.animePlaySpeeds[stateIndex];
		UpdateMotionIfCan( elapsedTime * motionAcceleration, stateIndex );
	}
	void SkeletonJoe::PhysicUpdate( float elapsedTime, const Map &terrain, bool considerBodyExistence )
	{
		// My body's existence will be changed by status, so I must be collide to terrain explicitly.
		Base::PhysicUpdate( elapsedTime, terrain, /* considerBodyExistence = */ false );
	}
	Kind SkeletonJoe::GetKind() const { return Kind::SkeletonJoe; }
	Definition::Damage SkeletonJoe::GetTouchDamage() const
	{
		return Parameter::GetSkeletonJoe().basic.touchDamage;
	}
	void SkeletonJoe::ApplyReceivedDamageIfHas()
	{
		if ( !pReceivedDamage ) { return; }
		// else

		const int damage = pReceivedDamage->amount;
		if ( hp <= damage )
		{
			Base::ApplyReceivedDamageIfHas();
			return;
		}
		// else

		BreakInit();

		pReceivedDamage.reset();
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

		( nextKind == MotionKind::Idle )
		? model.animator.EnableLoop()
		: model.animator.DisableLoop();

		model.animator.ResetTimer();

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
			FireInit( wsTargetPos );
			break;

		case MotionKind::Fire:
			next = MotionKind::Idle;
			IdleInit();
			break;

		case MotionKind::Break:
			next = MotionKind::ReAssemble;
			ReAssembleInit();
			break;

		case MotionKind::ReAssemble:
			next = MotionKind::Idle;
			IdleInit();
			break;

		default: return;
		}

		ChangeState( next );
	}
	void SkeletonJoe::Shot( const Donya::Vector3 &wsTargetPos )
	{
		// Using formula by: https://keisan.casio.jp/exec/system/1204505808

		const auto &data = Parameter::GetSkeletonJoe();

		const float t = data.impactSecond;
		if ( IsZero( t ) )
		{
			_ASSERT_EXPR( 0, L"Error: Division by zero!" );
			return;
		}
		// else

		const float diff = wsTargetPos.x - GetPosition().x;
		const float horizDist = fabsf( diff );
		const float &g = Bullet::Parameter::GetBone().gravity;
		const float gt = g*t;

		// v = sqrt( (l/t)^2 + (gt/2)^2 )
		const float lt = horizDist / t;
		const float speed = sqrtf( (lt*lt) + (gt*gt*0.25f) );

		// theta = atan( ( g*(t^2) ) / 2l )
		const float theta = atanf( ( gt*t ) / (2.0f*horizDist) );

		Bullet::FireDesc desc = data.fireDesc;
		desc.position	=  orientation.RotateVector( desc.position ); // Rotate the local space offset
		desc.position	+= GetPosition(); // Convert to world space
		desc.owner		=  hurtBox.id;
		desc.initialSpeed	= speed;
		desc.direction.x	= cosf( theta ) * Donya::SignBitF( diff );
		desc.direction.y	= sinf( theta );
		desc.direction.z	= 0.0f;

		Bullet::Admin::Get().RequestFire( desc );
		ChangeMotion( MotionKind::Fire );

		// TODO: Make shot SE
		// Donya::Sound::Play( Music::SkeletonJoe_Shot );
	}
	void SkeletonJoe::IdleInit()
	{
		body.exist		= true;
		hurtBox.exist	= true;
		timer			= 0.0f;
	}
	void SkeletonJoe::FireInit( const Donya::Vector3 &wsTargetPos )
	{
		timer = 0.0f;
		Shot( wsTargetPos );
	}
	void SkeletonJoe::BreakInit()
	{
		body.exist		= false;
		hurtBox.exist	= false;
		timer			= 0.0f;
		hp				= GetInitialHP();
		ChangeState( MotionKind::Break );
	}
	void SkeletonJoe::ReAssembleInit()
	{
		timer = 0.0f;
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
