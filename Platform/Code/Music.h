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

		Bullet_HitBone,
		Bullet_HitBuster,
		Bullet_HitShield,
		Bullet_HitSuperBall,
		Bullet_Protected,
		Bullet_ShotBone,
		Bullet_ShotBuster,
		Bullet_ShotShield_Expand,
		Bullet_ShotShield_Throw,
		Bullet_ShotSkullBuster,

		CatchItem,

		Charge_Complete,
		Charge_Loop,
		Charge_Start,

		Door_OpenClose,

		Performance_AppearBoss,
		Performance_ClearStage,

		Player_1UP,
		Player_Appear,
		Player_Damage,
		Player_Dash,
		Player_Jump,
		Player_Landing,
		Player_Leave,
		Player_Miss,
		Player_ShiftGun,
		Player_Shoryuken,
		
		RecoverHP,

		SkeletonJoe_Break,
		SkeletonJoe_ReAssemble_Begin,
		SkeletonJoe_ReAssemble_End,

		Skull_Landing,
		Skull_Jump,
		Skull_Roar,

		SuperBallMachine_Shot,

		UI_Choose,
		UI_Decide,
		
	#if DEBUG_MODE
		DEBUG_Strong,
		DEBUG_Weak,
	#endif // DEBUG_MODE

		MUSIC_COUNT
	};
}
