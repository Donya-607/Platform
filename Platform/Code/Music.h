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
		Bullet_Protected,
		Bullet_ShotBuster,
		Bullet_ShotShield,

		Charge_Complete,
		Charge_Loop,
		Charge_Start,
		
		Player_1UP,
		Player_Damage,
		Player_Dash,
		Player_Jump,
		Player_Landing,
		Player_Miss,
		
		RecoverHP,

		SuperBallMachine_Shot,
		
	#if DEBUG_MODE
		DEBUG_Strong,
		DEBUG_Weak,
	#endif // DEBUG_MODE

		MUSIC_COUNT
	};
}
