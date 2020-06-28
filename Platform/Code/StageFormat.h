#pragma once

#include <climits>	// Use INT_MAX

namespace StageFormat
{
	enum ID
	{
		StartPointRight	= -1, // Looking right
		StartPointLeft	= -2, // Looking left
		Space			= 0,
		Normal			= 1,

		RoomStart		= 100, // [RoomStart <= N < RoomEnd] is room identifier
		RoomEnd			= 200, // [RoomStart <= N < RoomEnd] is room identifier
		EmptyValue		= INT_MAX
	};
}
