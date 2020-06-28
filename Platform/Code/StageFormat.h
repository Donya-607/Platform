#pragma once

#include <climits>	// Use INT_MAX

namespace StageFormat
{
	enum ID
	{
		EmptyValue		= -1,
		Space			= EmptyValue,
		StartPointRight	= 0,	// Looking right
		StartPointLeft	= 1,	// Looking left
		Normal			= 2,

		RoomStart		= 8,	// [RoomStart <= N < RoomEnd] is room identifier
		RoomEnd			= 15,	// [RoomStart <= N < RoomEnd] is room identifier
	};
}
