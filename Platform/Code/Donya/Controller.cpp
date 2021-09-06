#include "Controller.h"

#include <array>
#include <memory>

#include "GamepadXInput.h"

namespace Donya
{
	namespace Controller
	{
		// Currently, I supports only the XInput controller
		static std::array<Impl::XInput, PadNumber::MAX_PAD_COUNT> controllers;
		static bool isInitialized = false;


		void Init()
		{
			if ( isInitialized ) { return; }
			// else


			// Construct the controllers
			for ( int i = 0; i < MAX_PAD_COUNT; ++i )
			{
				controllers[i].Init( static_cast<PadNumber>( i ) );
			}


			// Completed
			isInitialized = true;
		}
		void Update()
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else


			for ( auto &it : controllers )
			{
				it.Update();
			}
		}
		void Uninit()
		{
			Impl::XInput::Uninit();


			isInitialized = false;
		}


		bool IsConnected( PadNumber no )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return false; }
			// else


			// Verify all
			if ( no < 0 || MAX_PAD_COUNT <= no )
			{
				for ( auto &it : controllers )
				{
					if ( it.IsConnected() )
					{
						return true;
					}
				}

				return false;
			}
			// else


			// Verify the one
			return controllers[no].IsConnected();
		}

		int  Press  ( Button kind, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return NULL; }
			// else

			return controllers[padNo].Press( kind );
		}
		bool Trigger( Button kind, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return false; }
			// else

			return controllers[padNo].Trigger( kind );
		}
		bool Release( Button kind, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return false; }
			// else

			return controllers[padNo].Release( kind );
		}
		bool Repeat ( Button kind, int interval, int lowestFrame, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return false; }
			// else

			return controllers[padNo].Repeat( kind, interval, lowestFrame );
		}

		int  PressStick  ( StickDirection dir, bool leftStick, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return NULL; }
			// else

			return controllers[padNo].PressStick( dir, leftStick );
		}
		bool TriggerStick( StickDirection dir, bool leftStick, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return false; }
			// else

			return controllers[padNo].TriggerStick( dir, leftStick );
		}
		bool ReleaseStick( StickDirection dir, bool leftStick, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return false; }
			// else

			return controllers[padNo].ReleaseStick( dir, leftStick );
		}
		bool RepeatStick ( StickDirection dir, int interval, int lowestFrame, bool leftStick, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return false; }
			// else

			return controllers[padNo].RepeatStick( dir, interval, lowestFrame, leftStick );
		}

		Vector2 Stick( bool leftStick, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return Donya::Vector2::Zero(); }
			// else

			return controllers[padNo].Stick( leftStick );
		}

		void Vibrate( int vibrateFrame, float leftStrength, float rightStrength, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			controllers[padNo].Vibrate( vibrateFrame, leftStrength, rightStrength );
		}
		void Vibrate( int vibrateFrame, float strength, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			controllers[padNo].Vibrate( vibrateFrame, strength );
		}
		void StopVibration( PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			controllers[padNo].StopVibration();
		}
	}
}
