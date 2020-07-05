#pragma once

namespace StageFormat
{
	enum ID
	{
		EmptyValue		= -1,
		Space			= 0,
		StartPointRight	= 1,	// Looking right
		StartPointLeft	= 2,	// Looking left
		ClearEvent		= 3,
		Normal			= 4,

		RoomStart		= 8,				// [RoomStart <= N <= RoomLast] is room identifier
		RoomLast		= RoomStart + 15,	// [RoomStart <= N <= RoomLast] is room identifier

		EnemyStart		= RoomLast + 1,		// [EnemyStart <= N <= EnemyLast] is enemy identifier
		EnemyLast		= EnemyStart + 7,	// [EnemyStart <= N <= EnemyLast] is enemy identifier
		
		BossStart		= EnemyLast + 1,	// [BossStart <= N <= BossLast] is boss identifier
		BossLast		= BossStart + 7,	// [BossStart <= N <= BossLast] is boss identifier
	};
}
