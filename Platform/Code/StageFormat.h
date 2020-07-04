#pragma once

namespace StageFormat
{
	enum ID
	{
		EmptyValue		= -1,
		Space			= 0,
		StartPointRight	= 1,	// Looking right
		StartPointLeft	= 2,	// Looking left
		Normal			= 3,
		ClearEvent		= 4,

		RoomStart		= 8,	// [RoomStart <= N <= RoomLast] is room identifier
		RoomLast		= 15,	// [RoomStart <= N <= RoomLast] is room identifier

		EnemyStart		= 16,	// [EnemyStart <= N <= EnemyLast] is room identifier
		EnemyLast		= 23,	// [EnemyStart <= N <= EnemyLast] is room identifier
	};
}
