#include "Weapon.h"

#include <cassert> // Use _ASSERT_EXPR()

namespace Definition
{
	bool WeaponAvailableStatus::IsAvailable( WeaponKind kind )
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
	void WeaponAvailableStatus::Reset()
	{
		constexpr unsigned int busterOnlyBit = KindToBit( WeaponKind::Buster );
		kindBits = busterOnlyBit;
	}
	void WeaponAvailableStatus::Activate( WeaponKind kind )
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
	void WeaponAvailableStatus::Deactivate( WeaponKind kind )
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
#if USE_IMGUI
	void WeaponAvailableStatus::ShowImGuiNode( const char *nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption ) ) { return; }
		// else

		using WP = WeaponKind;
		constexpr int count = static_cast<int>( WP::WeaponCount );

		std::string str;
		for ( int i = 0; i < count; ++i )
		{
			const WP wp = scast<WP>( i );
			if ( IsAvailable( wp ) )
			{
				str += "[";
				str += GetWeaponName( wp );
				str += "]";
			}
		}
		ImGui::Text( u8"åªç›ÅF%s", str.c_str() );

		for ( int i = 0; i < count; ++i )
		{
			const WP wp = scast<WP>( i );

			bool enabled = IsAvailable( wp );
			ImGui::Checkbox( GetWeaponName( wp ), &enabled );

			( enabled )
			? Activate( wp )
			: Deactivate( wp );

			// Show two checkboxes per line
			if ( ( i % 2 ) == 0 && i < count - 1 )
			{
				ImGui::SameLine();
			}
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI
}
