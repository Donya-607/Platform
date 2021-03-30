#pragma once

#include "../Player.h"

namespace PlayerMover
{
	class Normal : public Player::MoverBaseFOO
	{
	private:
		bool gotoSlide = false;
		bool gotoLadder = false;
		bool braceOneself = false;
	public:
		void Update( Player &instance, float elapsedTime, const Map &terrain ) = 0;
		std::unique_ptr<StateMachine::IState<Player>> MakeNextStateOrNull( Player &ownerInstance ) = 0;
	public:
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
	public:
		Player::MotionKind GetNowMotionKind( const Player &instance ) const override;
	private:
		void UpdateVertical( Player &instance, float elapsedTime, const Map &terrain );
	#if USE_IMGUI
	public:
		const char *GetMoverName() const { return u8"í èÌ"; }
	#endif // USE_IMGUI
	};
}
