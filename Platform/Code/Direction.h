#pragma once

#include <string>
#include <Windows.h>		// Use DEFINE_ENUM_FLAG_OPERATORS

#include "Donya/UseImGui.h"	// Use USE_IMGUI macro

namespace Definition
{
	/// <summary>
	/// You can use bitwise operation.
	/// </summary>
	enum class Direction
	{
		Nil		= 0,
		Up		= 1 << 0,
		Down	= 1 << 1,
		Left	= 1 << 2,
		Right	= 1 << 3,
	};

	/// <summary>
	/// GetContainName( Up | Down ) returns "[Up][Down]".
	/// </summary>
	std::string GetContainName( const Direction &value );

#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption, Direction *p, bool allowMultipleDirection = true, bool useTreeNode = true );
#endif // USE_IMGUI
}
DEFINE_ENUM_FLAG_OPERATORS( Definition::Direction )
namespace Definition
{
	// These functions using a bitwise operation, so I define that in here after defined the operators.

	/// <summary>
	/// lhs + rhs
	/// </summary>
	constexpr Direction Add( const Direction &lhs, const Direction &rhs )
	{
		return lhs | rhs;
	}
	/// <summary>
	/// lhs - rhs
	/// </summary>
	constexpr Direction Subtract( const Direction &lhs, const Direction &rhs )
	{
		return lhs & ~( rhs );
	}
	/// <summary>
	/// "value" has "verify"?
	/// </summary>
	constexpr bool Contain( const Direction &value, const Direction &verify )
	{
		return static_cast<int>( value & verify ) != 0;
	}
}
