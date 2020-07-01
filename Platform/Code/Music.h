#pragma once

#include "Donya/Constant.h"	// Use for DEBUG_MODE macro.

namespace Music
{
	enum ID
	{
		// BGM_Title = 0,

		Bullet_HitBuster,
		
		Player_Jump,
		Player_Landing,
		Player_Shot,
		
	#if DEBUG_MODE
		DEBUG_Strong,
		DEBUG_Weak,
	#endif // DEBUG_MODE

		MUSIC_COUNT
	};
}
