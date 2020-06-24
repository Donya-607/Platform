#pragma once

#include <climits>	// Use INT_MAX

namespace StageFormat
{
	enum ID
	{
		StartPoint	= -1,
		Space		= 0,
		Normal		= 1,

		EmptyValue	= INT_MAX
	};
}
