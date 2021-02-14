#pragma once

#include <array>

#include "Donya/Vector.h"

namespace NumPad
{
	/// <summary>
	/// Value + 1 == Number-pad's number
	/// </summary>
	enum Value
	{
		_7 = 6,		_8 = 7,		_9 = 8,
		_4 = 3,		_5 = 4,		_6 = 5,
		_1 = 0,		_2 = 1,		_3 = 2,
	};
	constexpr unsigned int keyCount = 9;

	namespace Impl
	{
		using Dir = Donya::Vector2;
		constexpr float h = 0.70710678118f; // Half
		constexpr std::array<Dir, keyCount> directions
		{
			Dir{-h, +h},	Dir{+0, +1},	Dir{+h, +h},
			Dir{-1, +0},	Dir{+0, +0},	Dir{+1, +0},
			Dir{-h, -h},	Dir{+0, -1},	Dir{+h, -h},
		};
	}

	/// <summary>
	/// Value::_1 -> '0',
	/// ( Value )( 42 ) -> 'Z'(it works correctly, but unexpected usage)
	/// </summary>
	constexpr char ToChar( const Value &v )
	{
		return '0' + static_cast<char>( v );
	}
	/// <summary>
	/// Returns unit vector.
	/// Value::_1 -> {-0.7071f, -0.7071f},
	/// ( Value )( 42 ) -> {+0, +0}(does not support)
	/// </summary>
	constexpr Donya::Vector2 ToDirection( const Value &v )
	{
		const unsigned int i = static_cast<unsigned int>( v );
		return ( i < keyCount ) ? Impl::directions[i] : Donya::Vector2::Zero();
	}
}
