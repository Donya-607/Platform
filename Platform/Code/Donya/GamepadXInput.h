#pragma once

#include <memory>

#include "Controller.h" // Use enum definitions
#include "Vector.h"

namespace Donya
{
	namespace Controller
	{
		namespace Impl
		{
			class Gamepad
			{
			protected:
				struct Impl;
				std::unique_ptr<Impl> pImpl;
			public:
				Gamepad();
				Gamepad( const Gamepad &  ) = default;
				Gamepad(       Gamepad && ) = default;
				Gamepad &operator = ( const Gamepad &  ) = default;
				Gamepad &operator = (       Gamepad && ) = default;
				virtual ~Gamepad();
			public:
				void Init( PadNumber padNumber );
			public:
				PadNumber PadNo() const;
			public:
				bool IsConnected() const;

				// Returns pressing frame, zero means no press.
				int  Press  ( Button kind ) const;
				bool Trigger( Button kind ) const;
				bool Release( Button kind ) const;
				// Returns true in every frame of "repeatIntervalFrame" when the Press() amount is greater than "lowestPressingFrame".
				bool Repeat ( Button kind, int repeatIntervalFrame, int lowestPressingFrame = 0 ) const;

				// Returns pressing frame, zero means no press.
				int  PressStick  ( StickDirection direction, bool leftStick = true ) const;
				bool TriggerStick( StickDirection direction, bool leftStick = true ) const;
				bool ReleaseStick( StickDirection direction, bool leftStick = true ) const;
				// Returns true in every frame of "repeatIntervalFrame" when the Press() amount is greater than "lowestPressingFrame".
				bool RepeatStick ( StickDirection direction, int repeatIntervalFrame, int lowestPressingFrame, bool leftStick = true ) const;

				/// <summary>
				/// Returns the stick's tilt, range is [-1.0f ~ 1.0f].
				/// Positive is [X]Right, [Y]Up.
				/// Negative is [X]Left,  [Y]Down.
				/// </summary>
				Vector2 Stick( bool leftStick ) const;
			};



			class XInput : public Gamepad
			{
			public:
				/// <summary>
				/// Please call when end the application.
				/// </summary>
				static void Uninit();
			public:
				// These constants' reference: https://docs.microsoft.com/ja-jp/windows/win32/api/xinput/ns-xinput-xinput_gamepad

				/// <summary>
				/// The max value of the stick's tilt
				/// </summary>
				static constexpr float maxStickTilt = 32767.0f;
				/// <summary>
				/// The min value of the stick's tilt
				/// </summary>
				static constexpr float minStickTilt = -32768.0f;
				/// <summary>
				/// Returns normalized(0.0f ~ 1.0f) threshold of dead zone
				/// </summary>
				static float GetDeadZoneLeftStick();
				/// <summary>
				/// Returns normalized(0.0f ~ 1.0f) threshold of dead zone
				/// </summary>
				static float GetDeadZoneRightStick();
			private:
				struct XImpl;
				std::unique_ptr<XImpl> pXImpl;
			public:
				XInput();
				XInput( const XInput &  ) = default;
				XInput(       XInput && ) = default;
				XInput &operator = ( const XInput &  ) = default;
				XInput &operator = (       XInput && ) = default;
				~XInput();
			public:
				void Update();
			private:
				void UpdateInputArray();
			public:
				/// <summary>
				/// Start the vibration.
				/// Strength range is [0.0f ~ 1.0f].
				/// If you want vibrate always, set -1 to "vibrateFrame".
				/// </summary>
				void Vibrate( int vibrateFrame, float leftStrength, float rightStrength );
				/// <summary>
				/// Start the vibration.
				/// Strength range is [0.0f ~ 1.0f].
				/// If you want vibrate always, set -1 to "vibrateFrame".
				/// </summary>
				void Vibrate( int vibrateFrame, float strength );
				// Start the vibration.
				void StopVibration();
			};
		}
	}

}
