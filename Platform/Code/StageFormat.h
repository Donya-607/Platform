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

		EmptyValue	= INT_MAX
	};
}
