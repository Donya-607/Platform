#pragma once

#include <array>

#include "Donya/GamepadXInput.h"
#include "Donya/Vector.h"

#include "Player.h"

namespace Input
{
	template<typename T>
	constexpr bool HasTrue( const std::array<T, Player::Input::variationCount> &arr )
	{
		for ( const auto &it : arr )
		{
			if ( it ) { return true; }
		}
		return false;
	}
	bool HasButtonInput( const Player::Input &input );
	bool HasStickInput( const Player::Input &input );
	
	Player::Input MakeCurrentInput( const Donya::XInput &controller, const Donya::Vector2 &stickDeadZone );

	bool IsPauseRequested( const Donya::XInput &controller );
}
