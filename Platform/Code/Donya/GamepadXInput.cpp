#include "GamepadXInput.h"

#pragma comment( lib, "Xinput.lib" )

#include <algorithm>
#include <array>
#include <Windows.h> // You must include this before <Xinput.h>.
#include <Xinput.h>
#include <vector>

#include "Constant.h"
#include "Template.h"
#include "Useful.h"

#undef max
#undef min

namespace Donya
{
	namespace Controller
	{
		namespace Impl
		{
			bool IsValid( Button kind )
			{
				return ( kind < 0 || Button::MAX_BUTTON_COUNT <= kind ) ? false : true;
			}
			bool IsValid( StickDirection dir )
			{
				return ( dir < 0 || StickDirection::MAX_STICK_DIRECTION_COUNT <= dir ) ? false : true;
			}


		#pragma region Gamepad

			struct Gamepad::Impl
			{
				PadNumber padNo = PadNumber::Pad1;
		
				struct Stick
				{
					Vector2 thumb{}; // -1.0f ~ 1.0f
					std::array<unsigned int, StickDirection::MAX_STICK_DIRECTION_COUNT> button{};
					std::array<unsigned int, StickDirection::MAX_STICK_DIRECTION_COUNT> prevButton{};
				private:
					void IncrementOrZero( unsigned int *pOutput, bool increment )
					{
						*pOutput =	( increment )
									? *pOutput + 1
									: 0;
					}
				public:
					void UpdateButton()
					{
						prevButton = button;

						const int signX = Donya::SignBit( thumb.x );
						const int signY = Donya::SignBit( thumb.y );
						IncrementOrZero( &button[StickDirection::UpTilt   ], signY == +1 );
						IncrementOrZero( &button[StickDirection::DownTilt ], signY == -1 );
						IncrementOrZero( &button[StickDirection::LeftTilt ], signX == -1 );
						IncrementOrZero( &button[StickDirection::RightTilt], signX == +1 );
					}
				};
				Stick stickL{};
				Stick stickR{};
		
				std::array<unsigned int, Button::MAX_BUTTON_COUNT> input{};		// It has one-to-one correspondence
				std::array<unsigned int, Button::MAX_BUTTON_COUNT> prevInput{};	// It has one-to-one correspondence

				bool isConnected = false;
			};

			Gamepad::Gamepad( PadNumber padNo )
				: pImpl( std::make_unique<Gamepad::Impl>( padNo ) )
			{
				pImpl->padNo = padNo;
			}
			Gamepad::~Gamepad()
			{
				pImpl.reset( nullptr );
			}

			PadNumber Gamepad::PadNo() const
			{
				return pImpl->padNo;
			}
			bool Gamepad::IsConnected() const
			{
				return pImpl->isConnected;
			}

			int Gamepad::Press( Button kind ) const
			{
				if ( !IsValid( kind ) ) { return NULL; }
				// else

				return pImpl->input[kind];
			}
			bool Gamepad::Trigger( Button kind ) const
			{
				return ( Press( kind ) == 1 ) ? true : false;
			}
			bool Gamepad::Release( Button kind ) const
			{
				if ( !IsValid( kind ) )			{ return false; }
				if ( !pImpl->prevInput[kind] )	{ return false; }
				if ( pImpl->input[kind] )		{ return false; }
				// else
				return true;
			}
			bool Gamepad::Repeat( Button kind, int interval, int lowestFrame ) const
			{
				int current = Press( kind );
				current -= lowestFrame;

				// The frame must greater than the "lowestFrame"
				if ( current <= 0 ) { return false; }
				// else


				// Validate the frame is the timing of "interval"

				// Prevent zero divide
				if ( interval == 0 ) { return ( current ) ? true : false; }
				// else

				return ( ( current % interval ) == 1 ) ? true : false;
			}

			int  Gamepad::PressStick( StickDirection dir, bool leftStick ) const
			{
				if ( !IsValid( dir ) ) { return NULL; }
				// else

				int iDir = scast<int>( dir );

				return ( leftStick ) ? pImpl->stickL.button[iDir] : pImpl->stickR.button[iDir];
			}
			bool Gamepad::TriggerStick( StickDirection dir, bool leftStick ) const
			{
				return ( PressStick( dir, leftStick ) == 1 ) ? true : false;
			}
			bool Gamepad::ReleaseStick( StickDirection dir, bool leftStick ) const
			{
				if ( !IsValid( dir ) )	{ return false; }
				// else

				auto &input = ( leftStick ) ? pImpl->stickL : pImpl->stickR;
				int iDir = scast<int>( dir );

				if ( !input.prevButton[iDir] )	{ return false; }
				if ( input.button[iDir] )		{ return false; }
				// else
				return true;
			}
			bool Gamepad::RepeatStick( StickDirection dir, int interval, int lowestFrame, bool leftStick ) const
			{
				if ( !IsValid( dir ) ) { return false; }
				// else


				int current = PressStick( dir );
				current -= lowestFrame;

				// The frame must greater than the "lowestFrame"
				if ( current <= 0 ) { return false; }
				// else


				// Validate the frame is the timing of "interval"

				// Prevent zero divide
				if ( interval == 0 ) { return ( current ) ? true : false; }
				// else

				return ( ( current % interval ) == 1 ) ? true : false;
			}

			Vector2 Gamepad::Stick( bool leftStick ) const
			{
				return ( leftStick ) ? pImpl->stickL.thumb : pImpl->stickR.thumb;
			}

			// region Gamepad
		#pragma endregion



		#pragma region XInput

			constexpr int XINPUT_MAX_BUTTON_COUNT = Button::MAX_BUTTON_COUNT - 2/* The LT, RT is not contain in constants of XInput buttons */;
			constexpr std::array<unsigned int, XINPUT_MAX_BUTTON_COUNT> XInputButtons
			{
				XINPUT_GAMEPAD_DPAD_UP,
				XINPUT_GAMEPAD_DPAD_DOWN,
				XINPUT_GAMEPAD_DPAD_LEFT,
				XINPUT_GAMEPAD_DPAD_RIGHT,
				XINPUT_GAMEPAD_START,
				XINPUT_GAMEPAD_BACK,
				XINPUT_GAMEPAD_LEFT_THUMB,
				XINPUT_GAMEPAD_RIGHT_THUMB,
				XINPUT_GAMEPAD_LEFT_SHOULDER,
				XINPUT_GAMEPAD_RIGHT_SHOULDER,
				XINPUT_GAMEPAD_A,
				XINPUT_GAMEPAD_B,
				XINPUT_GAMEPAD_X,
				XINPUT_GAMEPAD_Y
			};

		#pragma warning( push )
		#pragma warning( disable : 4995 )
			void XInput::Uninit()
			{
				// HACK: error C4995: 名前が避けられた #pragma として記述されています。
				XInputEnable( FALSE );
			}
		#pragma warning( pop )
			float XInput::GetDeadZoneLeftStick()
			{
				constexpr float border = scast<float>( XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ) / maxStickTilt;
				return border;
			}
			float XInput::GetDeadZoneRightStick()
			{
				constexpr float border = scast<float>( XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ) / maxStickTilt;
				return border;
			}

			struct XInput::Impl
			{
				static constexpr int VIBRATION_RANGE = 65535;
			public:
				XINPUT_STATE	state{};
				int				vibrateTimer{ 0 };
			public:
				XINPUT_VIBRATION MakeVibration( float leftStrength, float rightStrength ) const
				{
					leftStrength  = Donya::Clamp( leftStrength,  0.0f, 1.0f );
					rightStrength = Donya::Clamp( rightStrength, 0.0f, 1.0f );

					XINPUT_VIBRATION vibration{};
					vibration.wLeftMotorSpeed  = scast<WORD>( leftStrength  * VIBRATION_RANGE );
					vibration.wRightMotorSpeed = scast<WORD>( rightStrength * VIBRATION_RANGE );
					return vibration;
				}
				XINPUT_VIBRATION MakeVibration( float strength ) const
				{
					const float left  = strength;
					const float right = strength * 0.5f; // Magic number :(
					return MakeVibration( left, right );
				}
			public:
				/// <summary>
				/// Returns false if the controller does not connected.
				/// </summary>
				bool SetVibration( Donya::Controller::PadNumber padNo, int vibrateFrame, XINPUT_VIBRATION vibration )
				{
					auto result = XInputSetState( padNo, &vibration );
					if ( result == ERROR_DEVICE_NOT_CONNECTED )
					{
						// Error process here if necessary.
						return false;
					}
					// else

					vibrateTimer = vibrateFrame;

					return true;
				}
				/// <summary>
				/// Returns false if the controller does not connected.
				/// </summary>
				bool StopVibration( Donya::Controller::PadNumber padNo )
				{
					XINPUT_VIBRATION stop{};
					stop.wLeftMotorSpeed  = 0;
					stop.wRightMotorSpeed = 0;

					auto result = XInputSetState( padNo, &stop );
					if ( result == ERROR_DEVICE_NOT_CONNECTED )
					{
						// Error process here if necessary.
						return false;
					}
					// else

					vibrateTimer = 0;

					return true;
				}

				void UpdateVibration( Donya::Controller::PadNumber padNo )
				{
					if ( vibrateTimer <= 0 ) { return; }
					// else

					vibrateTimer--;
					if ( vibrateTimer <= 0 )
					{
						StopVibration( padNo );
					}
				}
			};

			XInput::XInput( PadNumber padNo )
				: Gamepad( padNo ), pXImpl( std::make_unique<XInput::Impl>() )
			{}
			XInput::XInput( const XInput &ref )
				: Gamepad( ref.PadNo() ), pXImpl( Donya::Clone( ref.pXImpl ) )
			{}
			XInput &XInput::operator = ( const XInput &ref )
			{
				if ( this == &ref ) { return *this; }
				// else

				pXImpl = Donya::Clone( ref.pXImpl );

				return *this;
			}
			XInput::~XInput()
			{
				pXImpl.reset( nullptr );
			}

			void XInput::Update()
			{
				auto result = XInputGetState( PadNo(), &pXImpl->state );

				if ( result == ERROR_DEVICE_NOT_CONNECTED )
				{
					pImpl->isConnected = false;
					return;
				}
				if ( result != ERROR_SUCCESS ) { return; }
				// else

				pImpl->isConnected = true;

				UpdateInputArray();

				pXImpl->UpdateVibration( PadNo() );
			}
			bool IsOverTriggerThreshold( int LRTrigger )
			{
				return	( XINPUT_GAMEPAD_TRIGGER_THRESHOLD < abs( LRTrigger ) )
						? true
						: false;
			}
			void XInput::UpdateInputArray()
			{
				pImpl->prevInput = pImpl->input;

				auto &pad = pXImpl->state.Gamepad;


				// Button::UP ~ Button::Y
				for ( size_t i = 0; i < XINPUT_MAX_BUTTON_COUNT; ++i )
				{
					if ( pad.wButtons & XInputButtons[i] )
					{
						pImpl->input[i]++;
					}
					else
					{
						pImpl->input[i] = 0;
					}
				}


				// Button::LT, Button::RT
				{
					pImpl->input[Button::LT] =	IsOverTriggerThreshold( pad.bLeftTrigger )
												? pImpl->input[Button::LT] + 1
												: 0;
					pImpl->input[Button::RT] =	IsOverTriggerThreshold( pad.bRightTrigger )
												? pImpl->input[Button::RT] + 1
												: 0;
				}


				// Stick and stick's tilt
				{
					// Assign
					pImpl->stickL.thumb.x = scast<float>( pad.sThumbLX );
					pImpl->stickL.thumb.y = scast<float>( pad.sThumbLY );
					pImpl->stickR.thumb.x = scast<float>( pad.sThumbRX );
					pImpl->stickR.thumb.y = scast<float>( pad.sThumbRY );

					// Normalize
					pImpl->stickL.thumb.x /= maxStickTilt;
					pImpl->stickL.thumb.y /= maxStickTilt;
					pImpl->stickR.thumb.x /= maxStickTilt;
					pImpl->stickR.thumb.y /= maxStickTilt;

					// Apply a dead zone
				#if 0 // Discard each axis individually
					float deadZone = GetDeadZoneLeftStick();
					if ( pImpl->stickL.thumb.x < deadZone ) { pImpl->stickL.thumb.x = 0.0f; }
					if ( pImpl->stickL.thumb.y < deadZone ) { pImpl->stickL.thumb.y = 0.0f; }
					deadZone = GetDeadZoneRightStick();
					if ( pImpl->stickR.thumb.x < deadZone ) { pImpl->stickR.thumb.x = 0.0f; }
					if ( pImpl->stickR.thumb.y < deadZone ) { pImpl->stickR.thumb.y = 0.0f; }
				#else // Discard by input vector's magnitude
					float deadZone  = GetDeadZoneLeftStick();
					float magnitude = pImpl->stickL.thumb.LengthSq();
					if ( magnitude < deadZone * deadZone ) { pImpl->stickL.thumb = 0.0f; }
					deadZone  = GetDeadZoneRightStick();
					magnitude = pImpl->stickR.thumb.LengthSq();
					if ( magnitude < deadZone * deadZone ) { pImpl->stickR.thumb = 0.0f; }
				#endif // DEAD_ZONE_PREFERENCE


					// Update the stick's tilt as a button
					pImpl->stickL.UpdateButton();
					pImpl->stickR.UpdateButton();
				}
			}

			void XInput::Vibrate( int vibrateFrame, float leftStrength, float rightStrength )
			{
				auto vibration = pXImpl->MakeVibration( leftStrength, rightStrength );
				bool result = pXImpl->SetVibration( PadNo(), vibrateFrame, vibration );

				if ( !result )
				{
					pImpl->isConnected = false;
				}
			}
			void XInput::Vibrate( int vibrateFrame, float strength )
			{
				auto vibration = pXImpl->MakeVibration( strength );
				bool result = pXImpl->SetVibration( PadNo(), vibrateFrame, vibration );

				if ( !result )
				{
					pImpl->isConnected = false;
				}
			}
			void XInput::StopVibration()
			{
				pXImpl->StopVibration( PadNo() );
			}

			// region XInput
		#pragma endregion

		}
	}
}