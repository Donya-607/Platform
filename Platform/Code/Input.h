#pragma once

#include "Donya/GamepadXInput.h"
#include "Donya/Vector.h"

#include "Player.h"

namespace Input
{
	Player::Input MakeCurrentInput( const Donya::XInput &controller, const Donya::Vector2 &stickDeadZone );
}
