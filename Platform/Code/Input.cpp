#include "Input.h"

#include "Donya/Keyboard.h"

namespace Input
{
	Player::Input MakeCurrentInput( const Donya::XInput &controller, const Donya::Vector2 &deadZone )
	{
		Player::Input input{};

		bool pressLeft	= false;
		bool pressRight	= false;
		bool pressUp	= false;
		bool pressDown	= false;

		// TODO: To be changeable the input key or button

		if ( controller.IsConnected() )
		{
			using Button	= Donya::Gamepad::Button;
			using Direction	= Donya::Gamepad::StickDirection;

			const auto stick = controller.LeftStick();
		
			pressLeft	= controller.Press( Button::LEFT	) || ( stick.x <= -deadZone.x );
			pressRight	= controller.Press( Button::RIGHT	) || ( stick.x >= +deadZone.x );
			pressUp		= controller.Press( Button::UP		) || ( stick.y >= +deadZone.y );
			pressDown	= controller.Press( Button::DOWN	) || ( stick.y <= -deadZone.y );
		
			input.useJumps[0]	= controller.Press( Button::A	);
			input.useShots[0]	= controller.Press( Button::X	);
			input.useDashes[0]	= controller.Press( Button::LT	);
			input.shiftGuns[0]	= controller.Press( Button::PRESS_R	);
			if ( 2 <= Player::Input::variationCount )
			{
			input.useJumps[1]	= controller.Press( Button::B	);
			input.useShots[1]	= controller.Press( Button::Y	);
			input.useDashes[1]	= controller.Press( Button::RT	);
			}
		}
		else
		{
			pressLeft	= Donya::Keyboard::Press( VK_LEFT	);
			pressRight	= Donya::Keyboard::Press( VK_RIGHT	);
			pressUp		= Donya::Keyboard::Press( VK_UP		);
			pressDown	= Donya::Keyboard::Press( VK_DOWN	);

			input.useJumps[0]	= Donya::Keyboard::Press( 'Z'	);
			input.useShots[0]	= Donya::Keyboard::Press( 'X'	);
			input.useDashes[0]	= Donya::Keyboard::Press( 'A'	);
			input.shiftGuns[0]	= Donya::Keyboard::Press( 'C'	);
			if ( 2 <= Player::Input::variationCount )
			{
			input.useJumps[1]	= Donya::Keyboard::Press( VK_RSHIFT	);
			input.useShots[1]	= Donya::Keyboard::Press( 'S'	);
			}
		}

		if ( pressLeft	) { input.moveVelocity.x -= 1.0f; }
		if ( pressRight	) { input.moveVelocity.x += 1.0f; }
		if ( pressUp	) { input.moveVelocity.y += 1.0f; } // World space direction
		if ( pressDown	) { input.moveVelocity.y -= 1.0f; } // World space direction

		return input;
	}
}
