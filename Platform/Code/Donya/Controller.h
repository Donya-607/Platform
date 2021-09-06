#pragma once

#include "Vector.h"

namespace Donya
{
	// Support only XInput
	namespace Controller
	{
		enum PadNumber
		{
			Pad0 = 0,
			Pad1,
			Pad2,
			Pad3,

			MAX_PAD_COUNT
		};
		enum Button
		{
			UP = 0,		// Up key of D-pad
			DOWN,		// Down key of D-pad
			LEFT,		// Left key of D-pad
			RIGHT,		// Right key of D-pad
			START,		// Center button of right side
			SELECT,		// Center button of left side
			PRESS_L,	// L thumb(Push L stick)
			PRESS_R,	// R thumb(Push R stick)
			LB,			// L button of shoulder
			RB,			// R button of shoulder
			A,			// Following to XInput style
			B,			// Following to XInput style
			X,			// Following to XInput style
			Y,			// Following to XInput style
			LT,			// L trigger
			RT,			// R trigger

			MAX_BUTTON_COUNT
		};
		enum StickDirection
		{
			UpTilt = 0,
			DownTilt,
			LeftTilt,
			RightTilt,

			MAX_STICK_DIRECTION_COUNT
		};



		// Initialize the controller system.
		// Please call this only once at the program's initialization.
		void Init();
		// Update the controller system.
		// Please call this every frame at the program's main loop.
		void Update();
		// Uninitialize the controller system.
		// Please call this only once at the program's finalization.
		void Uninit();


		// Verify the controller of specified number is connected(true) or not(false).
		// If you use MAX_PAD_COUNT, it verifies all and returns true if a controller is connected even one.
		bool IsConnected( PadNumber verifyNo );

		// Returns pressing frame, zero means no press.
		int  Press  ( Button kind, PadNumber padNo = PadNumber::Pad0 );
		// Returns true if now is just beginning to press.
		bool Trigger( Button kind, PadNumber padNo = PadNumber::Pad0 );
		// Returns true if now is just beginning to release.
		bool Release( Button kind, PadNumber padNo = PadNumber::Pad0 );
		// Returns true in every frame of "repeatIntervalFrame" when the Press() amount is greater than "lowestPressingFrame".
		bool Repeat ( Button kind, int repeatIntervalFrame, int lowestPressingFrame = 0, PadNumber padNo = PadNumber::Pad0 );

		// Returns tilting frame, zero means no press.
		int  PressStick  ( StickDirection direction, bool leftStick = true, PadNumber padNo = PadNumber::Pad0 );
		// Returns true if now is just beginning to tilt.
		bool TriggerStick( StickDirection direction, bool leftStick = true, PadNumber padNo = PadNumber::Pad0 );
		// Returns true if now is just beginning to release.
		bool ReleaseStick( StickDirection direction, bool leftStick = true, PadNumber padNo = PadNumber::Pad0 );
		// Returns true in every frame of "repeatIntervalFrame" when the PressStick() amount is greater than "lowestPressingFrame".
		bool RepeatStick ( StickDirection direction, int repeatIntervalFrame, int lowestPressingFrame, bool leftStick = true, PadNumber padNo = PadNumber::Pad0 );

		/// <summary>
		/// Returns the stick's tilt, range is [-1.0f ~ 1.0f].
		/// Positive is [X]Right, [Y]Up.
		/// Negative is [X]Left,  [Y]Down.
		/// </summary>
		Vector2 Stick( bool leftStick, PadNumber padNo = PadNumber::Pad0 );

		/// <summary>
		/// Start the vibration.
		/// Strength range is [0.0f ~ 1.0f].
		/// If you want vibrate always, set -1 to "vibrateFrame".
		/// </summary>
		void Vibrate( int vibrateFrame, float leftStrength, float rightStrength, PadNumber padNo = PadNumber::Pad0 );
		/// <summary>
		/// Start the vibration.
		/// Strength range is [0.0f ~ 1.0f].
		/// If you want vibrate always, set -1 to "vibrateFrame".
		/// </summary>
		void Vibrate( int vibrateFrame, float strength, PadNumber padNo = PadNumber::Pad0 );
		// Stop the vibration.
		void StopVibration( PadNumber padNo = PadNumber::Pad0 );
	}
}
