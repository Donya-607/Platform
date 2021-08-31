#include "Mouse.h"

#include "Keyboard.h" // Use for GetMouseButtonState().

namespace Donya
{
	namespace Mouse
	{
		static Donya::Int2 coordinate; // cliant space.

		static Donya::Int2 wheelFraction{};
		static Donya::Int2 rotateAmount{};

		void UpdateMouseCoordinate( LPARAM lParam )
		{
			coordinate.x = LOWORD( lParam );
			coordinate.y = HIWORD( lParam );
		}

		void CalledMouseWheelMessage( bool isVertical, WPARAM wParam, LPARAM lParam )
		{
			// see http://black-yuzunyan.lolipop.jp/archives/2544

			// If update here, the coordinate will be screen-space.
			// but the coordinate is client-space, so I don't update.
			// UpdateMouseCoordinate( lParam );

			int *fraction = ( isVertical ) ? &wheelFraction.y : &wheelFraction.x;
			int *rotation = ( isVertical ) ? &rotateAmount.y  : &rotateAmount.x;

			int delta = GET_WHEEL_DELTA_WPARAM( wParam );
			delta += *fraction;

			*fraction  = delta % WHEEL_DELTA;

			int  notch = delta / WHEEL_DELTA;
			if ( notch < 0 )
			{
				( *rotation )--;
			}
			else if ( 0 < notch )
			{
				( *rotation )++;
			}
			else
			{
				*rotation = 0;
			}
		}

		void ResetMouseWheelRot()
		{
			rotateAmount.x =
			rotateAmount.y = 0;
		}

		Int2 Coordinate()
		{
			return coordinate;
		}
		Vector2 CoordinateF()
		{
			return Coordinate().Float();
		}

		Int2 Size()
		{
			Int2 size
			{
				GetSystemMetrics( SM_CXCURSOR ),
				GetSystemMetrics( SM_CYCURSOR )
			};
			return size;
		}
		Donya::Vector2 SizeF()
		{
			return Size().Float();
		}

		int  WheelRot( bool isVertical )
		{
			return ( isVertical ) ? rotateAmount.y : rotateAmount.x;
		}

		/*
		int State( Kind checkButton, Mode inputMode )
		{
			using Donya::Keyboard::Mode;

			switch ( inputMode )
			{
			case PRESS:		return Donya::Keyboard::Press( checkButton );	// break;
			case RELEASE:	return Donya::Keyboard::Release( checkButton );	// break;
			case REPEAT:	return Donya::Keyboard::Repeat( checkButton );	// break;
			case TRIGGER:	return Donya::Keyboard::Trigger( checkButton );	// break;
			default:		break;
			}

			return Donya::Keyboard::Press( checkButton );
		}
		*/

		int Press( Kind checkButton )
		{
			return Donya::Keyboard::Press( checkButton );
		}

		int Release( Kind checkButton )
		{
			return Donya::Keyboard::Release( checkButton );
		}

		int Repeat( Kind checkButton, int interval, int lowestFrame )
		{
			return Donya::Keyboard::Repeat( checkButton, interval, lowestFrame );
		}

		int Trigger( Kind checkButton )
		{
			return Donya::Keyboard::Trigger( checkButton );
		}

	}
}