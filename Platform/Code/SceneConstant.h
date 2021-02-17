#pragma once

#include <Windows.h> // Use DEFINE_ENUM_FLAG_OPERATORS

namespace Definition
{
	enum class DrawTarget
	{
		Map		= 1 << 0,
		Door	= 1 << 1,
		Bullet	= 1 << 2,
		Player	= 1 << 3,
		Vision	= 1 << 4,
		Boss	= 1 << 5,
		Enemy	= 1 << 6,
		Item	= 1 << 7,

		All		= Map | Door | Bullet | Player | Vision | Boss | Enemy | Item
	};
}
DEFINE_ENUM_FLAG_OPERATORS( Definition::DrawTarget )
