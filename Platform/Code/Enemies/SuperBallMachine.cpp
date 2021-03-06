#include "SuperBallMachine.h"

#include "../Donya/Sound.h"
#include "../Music.h"
#include "../Parameter.h"

namespace Enemy
{
	namespace Parameter
	{
		static ParamOperator<SuperBallMachineParam> superBallMachineParam{ "SuperBallMachine", "Enemy/" };
		const SuperBallMachineParam &GetSuperBallMachine()
		{
			return superBallMachineParam.Get();
		}

		namespace Impl
		{
			void LoadSuperBallMachine()
			{
				superBallMachineParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateSuperBallMachine( const std::string &nodeCaption )
			{
				superBallMachineParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	namespace
	{
		constexpr size_t motionCount = scast<size_t>( SuperBallMachine::MotionKind::MotionCount );
		constexpr const char *GetMotionName( SuperBallMachine::MotionKind kind )
		{
			switch ( kind )
			{
			case SuperBallMachine::MotionKind::Ready:	return u8"Ready";
			case SuperBallMachine::MotionKind::Fire:	return u8"Fire";
			default: break;
			}

			return "ERROR_KIND";
		}
	}

	void SuperBallMachine::Init( const InitializeParam &parameter, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox )
	{
		Base::Init( parameter, wsTargetPos, wsScreenHitBox );

		currentMotion = MotionKind::Ready;
		intervalTimer = Parameter::GetSuperBallMachine().initialTimerSecond;

		ChangeMotion( MotionKind::Ready );
	}
	void SuperBallMachine::Uninit()
	{
		Base::Uninit();
		intervalTimer = 0.0f;
	}
	void SuperBallMachine::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		Base::Update( elapsedTime, wsTargetPos, wsScreen );
		if ( NowWaiting() ) { return; }
		// else

	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		AssignMyBody( body.pos );
	#endif // USE_IMGUI

		const auto &data = Parameter::GetSuperBallMachine();

		velocity.y -= data.gravity * elapsedTime;

		if ( currentMotion == MotionKind::Fire && model.animator.WasEnded() )
		{
			ChangeMotion( MotionKind::Ready );
		}

		if ( currentMotion != MotionKind::Fire )
		{
			intervalTimer += elapsedTime;
		}

		ShotIfNeeded( elapsedTime, wsTargetPos );

		const auto   &playSpeeds		= data.animePlaySpeeds;
		const size_t currentMotionIndex	= scast<size_t>( currentMotion );
		const size_t playSpeedCount		= playSpeeds.size();
		const float  motionAcceleration	= ( playSpeedCount <= currentMotionIndex ) ? 1.0f : playSpeeds[currentMotionIndex];
		UpdateMotionIfCan( elapsedTime * motionAcceleration, currentMotionIndex );
	}
	Kind SuperBallMachine::GetKind() const { return Kind::SuperBallMachine; }
	Definition::Damage SuperBallMachine::GetTouchDamage() const
	{
		return Parameter::GetSuperBallMachine().basic.touchDamage;
	}
	int  SuperBallMachine::GetInitialHP() const
	{
		return Parameter::GetSuperBallMachine().basic.hp;
	}
	void SuperBallMachine::AssignMyBody( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetSuperBallMachine().basic;
		body.pos		= wsPos;
		body.offset		= data.hitBoxOffset;
		body.size		= data.hitBoxSize;
		hurtBox.pos		= wsPos;
		hurtBox.offset	= data.hurtBoxOffset;
		hurtBox.size	= data.hurtBoxSize;

		body.offset		= orientation.RotateVector( body.offset		);
		hurtBox.offset	= orientation.RotateVector( hurtBox.offset	);
	}
	void SuperBallMachine::ChangeMotion( MotionKind nextKind )
	{
		if ( nextKind != MotionKind::Ready && nextKind != MotionKind::Fire )
		{
			_ASSERT_EXPR( 0, L"Error: Unexpected motion kind!" );
			return;
		}
		// else

		currentMotion = nextKind;

		model.animator.ResetTimer();

		( nextKind == MotionKind::Fire )
		? model.animator.DisableLoop()
		: model.animator.EnableLoop();

		model.AssignMotion( scast<int>( nextKind ) );
	}
	void SuperBallMachine::ShotIfNeeded( float elapsedTime, const Donya::Vector3 &wsTargetPos )
	{
		const auto &data = Parameter::GetSuperBallMachine();

		// Judging to shotable

		if ( intervalTimer < data.fireIntervalSecond ) { return; }
		// else

		Donya::Collision::Box3F capturingArea{};
		capturingArea.pos  = GetPosition();
		capturingArea.size = data.capturingArea;
		if ( !Donya::Collision::IsHit( wsTargetPos, capturingArea, /* considerExistFlag = */ false ) ) { return; }
		// else


		// Shot here

		intervalTimer = 0.0f;

		float lookingSign = 1.0f;
		{
			const Donya::Vector3 front = orientation.LocalFront();
			const float dot = Donya::Dot( front, Donya::Vector3::Right() );
			lookingSign = Donya::SignBitF( dot );
		}
		const Donya::Quaternion lookingRotation = Donya::Quaternion::Make
		(
			Donya::Vector3::Up(), ToRadian( 90.0f ) * lookingSign
		);

		Bullet::FireDesc desc = data.fireDesc;
		desc.position	=  lookingRotation.RotateVector( desc.position ); // Rotate the local space offset
		desc.position	+= GetPosition(); // Convert to world space
		desc.owner		=  hurtBox.id;
		
		const float shotRadian			= ToRadian( data.fireDegree );
		const float shotRadianLeftVer	= ToRadian( 180.0f ) - shotRadian;
		desc.direction.x	= cosf( ( Donya::SignBit( lookingSign ) == -1 ) ? shotRadianLeftVer : shotRadian );
		desc.direction.y	= sinf( shotRadian );
		desc.direction.z	= 0.0f;

		Bullet::Admin::Get().RequestFire( desc );
		ChangeMotion( MotionKind::Fire );

		Donya::Sound::Play( Music::SuperBallMachine_Shot );
	}
#if USE_IMGUI
	bool SuperBallMachine::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else

		Base::ShowImGuiNode( u8"ξκͺ" );

		if ( ImGui::TreeNode( u8"hΆͺ" ) )
		{
			ImGui::DragFloat( u8"b^C}E­ΛΤu", &intervalTimer, 0.01f );

			ImGui::TreePop();
		}

		ImGui::TreePop();
		return true;
	}
	void SuperBallMachineParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"Δpέθ" );
		fireDesc.ShowImGuiNode( u8"­Λeέθ" );

		if ( animePlaySpeeds.size() != motionCount )
		{
			animePlaySpeeds.resize( motionCount, 1.0f );
		}
		if ( ImGui::TreeNode( u8"[VΔΆ¬x" ) )
		{
			for ( size_t i = 0; i < motionCount; ++i )
			{
				ImGui::DragFloat
				(
					GetMotionName( scast<SuperBallMachine::MotionKind>( i ) ),
					&animePlaySpeeds[i], 0.01f
				);
			}

			ImGui::TreePop();
		}

		ImGui::DragFloat3( u8"υGΝΝiΌaj",				&capturingArea.x,		0.01f	);
		ImGui::DragFloat ( u8"Ά¬Μoίb",				&initialTimerSecond,	0.01f	);
		ImGui::DragFloat ( u8"­ΛΤuibj",				&fireIntervalSecond,	0.01f	);
		ImGui::DragFloat ( u8"­ΛpxiDegreeEEό«j",	&fireDegree,			0.1f	);
		ImGui::DragFloat ( u8"dΝ",							&gravity,				0.01f	);

		capturingArea.x		= std::max( 0.01f, capturingArea.x		);
		capturingArea.y		= std::max( 0.01f, capturingArea.y		);
		capturingArea.z		= std::max( 0.01f, capturingArea.z		);
		fireIntervalSecond	= std::max( 0.01f, fireIntervalSecond	);
	}
#endif // USE_IMGUI
}
