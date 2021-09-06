#include "Controller.h"

#include <array>
#include <memory>

#include "GamepadXInput.h"

namespace Donya
{
	namespace Controller
	{
		// Currently, I supports only the XInput controller
		static std::array<std::unique_ptr<Impl::XInput>, PadNumber::MAX_PAD_COUNT>
			controllerPtrs{};
		static bool isInitialized = false;


		void Init()
		{
			if ( isInitialized ) { return; }
			// else


			// Construct the pointers
			for ( int i = 0; i < MAX_PAD_COUNT; ++i )
			{
				controllerPtrs[i] = std::make_unique<Impl::XInput>( i );
			}


			// Completed
			isInitialized = true;
		}
		void Update()
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else


			for ( auto &pIt : controllerPtrs )
			{
				pIt->Update();
			}
		}
		void Uninit()
		{
			Impl::XInput::Uninit();
		}


		bool IsConnected( PadNumber no )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else


			// Verify all
			if ( no < 0 || MAX_PAD_COUNT <= no )
			{
				for ( auto &pIt : controllerPtrs )
				{
					if ( pIt->IsConnected() )
					{
						return true;
					}
				}

				return false;
			}
			// else


			// Verify the one
			return controllerPtrs[no]->IsConnected();
		}

		int  Press  ( Button kind, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			return controllerPtrs[padNo]->Press( kind );
		}
		bool Trigger( Button kind, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			return controllerPtrs[padNo]->Trigger( kind );
		}
		bool Release( Button kind, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			return controllerPtrs[padNo]->Release( kind );
		}
		bool Repeat ( Button kind, int interval, int lowestFrame, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			return controllerPtrs[padNo]->Repeat( kind, interval, lowestFrame );
		}

		int  PressStick  ( StickDirection dir, bool leftStick, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			return controllerPtrs[padNo]->PressStick( dir, leftStick );
		}
		bool TriggerStick( StickDirection dir, bool leftStick, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			return controllerPtrs[padNo]->TriggerStick( dir, leftStick );
		}
		bool ReleaseStick( StickDirection dir, bool leftStick, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			return controllerPtrs[padNo]->ReleaseStick( dir, leftStick );
		}
		bool RepeatStick ( StickDirection dir, int interval, int lowestFrame, bool leftStick, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			return controllerPtrs[padNo]->RepeatStick( dir, interval, lowestFrame, leftStick );
		}

		Vector2 Stick( bool leftStick, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			return controllerPtrs[padNo]->Stick( leftStick );
		}

		void Vibrate( int vibrateFrame, float leftStrength, float rightStrength, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			controllerPtrs[padNo]->Vibrate( vibrateFrame, leftStrength, rightStrength );
		}
		void Vibrate( int vibrateFrame, float strength, PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			controllerPtrs[padNo]->Vibrate( vibrateFrame, strength );
		}
		void StopVibration( PadNumber padNo )
		{
			if ( !isInitialized ) { _ASSERT_EXPR( 0, L"ERROR: Controller system is not initialized!" ); return; }
			// else

			controllerPtrs[padNo]->StopVibration();
		}
	}
}
