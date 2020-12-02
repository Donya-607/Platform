#include "Weapon.h"

#include <cassert> // Use _ASSERT_EXPR()

namespace Definition
{
	bool AvailableStatus::IsAvailable( WeaponKind kind )
	{
		if ( kind == WeaponKind::Buster ) { return true; }
		// else
		if ( kind == WeaponKind::WeaponCount )
		{
			_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
			return false;
		}
		// else

		// Convert to the multiplier of 2
		const unsigned int kindAsBit = KindToBit( kind );

		return ( kindBits & kindAsBit ) != 0;
	}
	void AvailableStatus::Activate( WeaponKind kind )
	{
		if ( kind == WeaponKind::Buster ) { return; }
		// else
		if ( kind == WeaponKind::WeaponCount )
		{
			_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
			return;
		}
		// else

		// Convert to the multiplier of 2
		const unsigned int kindAsBit = KindToBit( kind );

		kindBits |= kindAsBit;
	}
	void AvailableStatus::Deactivate( WeaponKind kind )
	{
		if ( kind == WeaponKind::Buster ) { return; }
		// else
		if ( kind == WeaponKind::WeaponCount )
		{
			_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
			return;
		}
		// else

		// Convert to the multiplier of 2
		const unsigned int kindAsBit = KindToBit( kind );

		kindBits &= ~kindAsBit;
	}
}
