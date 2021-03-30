#include "PlayerMover_Normal.h"

void Player::AccessTest::Foo( Player &inst )
{
	inst.body.size.x = 0.5f;
}

namespace PlayerMover
{
	void Normal::Update( Player &inst, float elapsedTime, const Map &terrain )
	{
		inst.MoveHorizontal( elapsedTime );
		UpdateVertical( inst, elapsedTime, terrain );

		inst.ShotIfRequested( elapsedTime );

		const Donya::Vector2 &moveDir = inst.inputManager.CurrentMoveDirection();

		// Try to grabbing ladder if the game time is not pausing
		if ( !gotoSlide && !IsZero( elapsedTime ) )
		{
			const auto pLadder = inst.FindGrabbingLadderOrNullptr( moveDir.y, terrain );
			if ( pLadder )
			{
				inst.pTargetLadder = pLadder;
				gotoLadder = true;
			}
		}

		braceOneself = ( moveDir.y < 0.0f && inst.onGround && IsZero( inst.velocity.x ) );
		MotionUpdate( inst, elapsedTime );
	}
	std::unique_ptr<StateMachine::IState<Player>> Normal::MakeNextStateOrNull( Player &ownerInstance )
	{
		if ( gotoSlide )
		{
			// return std::make_unique<PlayerMover::Slide>();
		}
		// else
		if ( gotoLadder )
		{
			// return std::make_unique<PlayerMover::GrabLadder>();
		}
		// else

		return nullptr;
	}

	void Normal::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
	{
		MoveOnlyHorizontal( inst, elapsedTime, terrain, roomLeftBorder, roomRightBorder );
		MoveOnlyVertical  ( inst, elapsedTime, terrain );
	}
	Player::MotionKind Normal::GetNowMotionKind( const Player &inst ) const
	{
		const bool nowMoving = IsZero( inst.velocity.x ) ? false : true;
		const bool &onGround = inst.onGround;
	
		if ( !onGround )
		{
			return	( Donya::SignBit( inst.velocity.y ) == 1 )
					? Player::MotionKind::Jump_Rise
					: Player::MotionKind::Jump_Fall;
		}
		// else

		if ( nowMoving ) { return Player::MotionKind::Run;	}
		// else

		if ( braceOneself ) { return Player::MotionKind::Brace; }
		// else

		return Player::MotionKind::Idle;
	}
	void Normal::UpdateVertical( Player &inst, float elapsedTime, const Map &terrain )
	{
		// Deformity of inst.MoveVertical()

		// Make to can not act if game time is pausing
		if ( IsZero( elapsedTime ) )
		{
			inst.Fall( elapsedTime );
			return;
		}
		// else

		const auto &input = inst.inputManager;

		// The buffer-input process of NowUseDash() will discards a recorded input.
		// So if you evaluate the NowUseDash() when can not use the slide,
		// the buffered input will be discarded wastefully.
		const bool useSlide = inst.onGround && input.NowUseDash();

		// Jump condition and resolve vs slide condition
		if ( inst.WillUseJump() )
		{
			const bool pressDown = input.CurrentMoveDirection().y < 0.0f;
			if ( pressDown )
			{
				gotoSlide = true;

				// Certainly doing Fall() if do not jump
				inst.Fall( elapsedTime );

				// Make do not take this jump input in next status
				inst.inputManager.DetainNowJumpInput();

				return;
			}
			// else

			inst.Jump();

			// Enable the inertial-like jump even if was inputted the "jump" and "slide" in same time
			if ( useSlide )
			{
				inst.wasJumpedWhileSlide = true;
				inst.GenerateSlideEffects();
			}

			return;
		}
		// else

		inst.pressJumpSinceSlide = false;

		if ( useSlide )
		{
			gotoSlide = true;
		}

		inst.Fall( elapsedTime );
	}
}
