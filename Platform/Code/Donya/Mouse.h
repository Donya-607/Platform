#ifndef INCLUDED_DONYA_MOUSE_H_
#define INCLUDED_DONYA_MOUSE_H_

#include <Windows.h>

#include "Vector.h"	// Int2, Vector2

namespace Donya
{
	namespace Mouse
	{
		/// <summary>
		/// Please call when received WM_MOUSEMOVE at window procedure.
		/// </summary>
		void UpdateMouseCoordinate( LPARAM lParam );

		/// <summary>
		/// Please call when received WM_MOUSEWHEEL or WM_MOUSEHWHEEL at window procedure.
		/// </summary>
		void CalledMouseWheelMessage( bool isVertical, WPARAM wParam, LPARAM lParam );

		/// <summary>
		/// Please call once when every game-loop.
		/// </summary>
		void ResetMouseWheelRot();

		/// <summary>
		/// Mouse coordinate is cliant space.
		/// </summary>
		Donya::Int2 Coordinate();
		/// <summary>
		/// Float version of Coordinate(), that converted by static_cast<float>.
		/// Mouse coordinate is cliant space.
		/// </summary>
		Donya::Vector2 CoordinateF();

		/// <summary>
		/// Returns (Width,Height) of the mouse cursor, in pixels.
		/// These values are fetched by GetSystemMetrics() with SM_CXCURSOR and SM_CYCURSOR.
		/// </summary>
		Donya::Int2 Size();
		/// <summary>
		/// Float version of Size(), that converted by static_cast<float>.
		/// Returns (Width,Height) of the mouse cursor, in pixels.
		/// These values are fetched by GetSystemMetrics() with SM_CXCURSOR and SM_CYCURSOR.
		/// </summary>
		Donya::Vector2 SizeF();

		/// <summary>
		/// Returns :
		/// positive : rotate to up.
		/// zero : not rotate.
		/// negative : rotate to down.
		/// </summary>
		int WheelRot( bool isVertical = true );

		enum Mode
		{
			PRESS = 0,
			RELEASE,
			REPEAT,
			TRIGGER
		};
		enum Kind
		{
			LEFT	= VK_LBUTTON,
			MIDDLE	= VK_MBUTTON,
			RIGHT	= VK_RBUTTON
		};

		// bool State( Mode inputMode, Kind checkButton );

		/// <summary>
		/// Same as call Donya::Keyboard::Press( checkButton ).
		/// </summary>
		int Press( Kind checkButton );
		/// <summary>
		/// Same as call Donya::Keyboard::Release( checkButton ).
		/// </summary>
		int Release( Kind checkButton );
		/// <summary>
		/// Same as call Donya::Keyboard::Repeat( checkButton ).
		/// If current pressing frame is equal to "lowestNeedFrame", return TRUE(1).
		/// </summary>
		int Repeat( Kind checkButton, int repeatIntervalFrame, int lowestNeedFrame = 0 );
		/// <summary>
		/// Same as call Donya::Keyboard::Trigger( checkButton ).
		/// </summary>
		int Trigger( Kind checkButton );
	}
}

#endif // INCLUDED_DONYA_MOUSE_H_