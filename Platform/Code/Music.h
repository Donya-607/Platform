#pragma once

#include "Donya/Constant.h"	// Use for DEBUG_MODE macro.

namespace Music
{
	enum ID
	{
		BGM_Title,
		BGM_Game,
		BGM_Boss,
		BGM_Over,
		BGM_Result,

		Boss_Appear,
		Boss_Defeated,

		Bullet_HitBuster,
		Bullet_HitShield,
		Bullet_Protected,
		Bullet_ShotBuster,
		Bullet_ShotShield_Expand,
		Bullet_ShotShield_Throw,
		Bullet_ShotSkullBuster,

		Charge_Complete,
		Charge_Loop,
		Charge_Start,
		
		Player_1UP,
		Player_Appear,
		Player_Damage,
		Player_Dash,
		Player_Jump,
		Player_Landing,
		Player_Leave,
		Player_Miss,
		
		RecoverHP,

		Skull_Landing,
		Skull_Jump,
		Skull_Roar,

		SuperBallMachine_Shot,
		
	#if DEBUG_MODE
		DEBUG_Strong,
		DEBUG_Weak,
	#endif // DEBUG_MODE

		MUSIC_COUNT
	};
}
