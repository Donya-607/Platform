#pragma once

namespace Definition
{
	/// <summary>
	/// The player use's weapons
	/// </summary>
	enum class WeaponKind
	{
		Buster = 0,
		SkullShield,

		WeaponCount
	};
	constexpr const char *GetWeaponName( WeaponKind kind )
	{
		switch ( kind )
		{
		case WeaponKind::Buster:		return "Buster";
		case WeaponKind::SkullShield:	return "Shield";
		default: break;
		}

		return "ERROR";
	}

	class WeaponAvailableStatus
	{
	private:
		/// <summary>
		/// WeaponKind::(0) -> 0
		/// WeaponKind::(1) -> 1
		/// WeaponKind::(2) -> 2
		/// WeaponKind::(3) -> 4
		/// WeaponKind::(4) -> 8
		/// ...
		/// </summary>
		static constexpr unsigned int KindToBit( WeaponKind kind )
		{
			if ( kind == WeaponKind::Buster ) { return 0; }
			// else

			unsigned int uintKind = static_cast<unsigned int>( kind );

			// Adjust the shift count to:
			// the WeaponKind::(1) does not shift,
			// the WeaponKind::(2) shifts the once,
			// ...
			uintKind -= 1;

			// Convert to the multiplier of 2
			uintKind = 1 << uintKind;

			return uintKind;
		}
		/// <summary>
		/// *The not multiplier of 2 number may not convert to correct kind!*
		/// 0 -> WeaponKind::(0)
		/// 1 -> WeaponKind::(1)
		/// 2 -> WeaponKind::(2)
		/// 4 -> WeaponKind::(3)
		/// 8 -> WeaponKind::(4)
		/// ...
		/// </summary>
		static constexpr WeaponKind BitToKind( unsigned int bit )
		{
			unsigned int shiftCount = 0;
			while ( 0 < bit )
			{
				bit >>= 1;
				shiftCount++;
			}
			return static_cast<WeaponKind>( shiftCount );
		}
	private:
		unsigned int kindBits = 0; // It contains the WeaponKind data by bitwise operation. But the Buster(0) is absolutely has.
	public:
		bool IsAvailable( WeaponKind kind );
	public:
		void Reset();
		void Activate( WeaponKind activateKind );
		void Deactivate( WeaponKind deactivateKind );
	};
}
