#pragma once

namespace StageFormat
{
	enum ID
	{
		EmptyValue		= -1,
		Space			= EmptyValue,
		StartPointRight	= 0,	// Looking right
		StartPointLeft	= 1,	// Looking left
		Normal			= 2,
		ClearEvent		= 3,

		RoomStart		= 8,	// [RoomStart <= N < RoomEnd] is room identifier
		RoomEnd			= 16,	// [RoomStart <= N < RoomEnd] is room identifier

		EnemyStart		= 16,	// [EnemyStart <= N < EnemyEnd] is room identifier
		EnemyEnd		= 24,	// [EnemyStart <= N < EnemyEnd] is room identifier
	};
}
