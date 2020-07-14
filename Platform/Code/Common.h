#pragma once

namespace Common
{
	namespace Impl
	{
		// Screen size is 16:9

		constexpr long	screenWidthL		= 1600L;
		constexpr int	screenWidthI		= static_cast<int>	( screenWidthL	);
		constexpr float	screenWidthF		= static_cast<float>( screenWidthL	);
		constexpr long	screenHeightL		= 900L;
		constexpr int	screenHeightI		= static_cast<int>	( screenHeightL	);
		constexpr float	screenHeightF		= static_cast<float>( screenHeightL	);

		constexpr float lowestAllowFPS		= 10.0f;
		constexpr float largestDeltaTime	= 1.0f / lowestAllowFPS;
	}

	constexpr int	ScreenWidth()			{ return Impl::screenWidthI;		}
	constexpr float	ScreenWidthF()			{ return Impl::screenWidthF;		}
	constexpr long	ScreenWidthL()			{ return Impl::screenWidthL;		}

	constexpr int	ScreenHeight()			{ return Impl::screenHeightI;		}
	constexpr float	ScreenHeightF()			{ return Impl::screenHeightF;		}
	constexpr long	ScreenHeightL()			{ return Impl::screenHeightL;		}

	constexpr int	HalfScreenWidth()		{ return Impl::screenWidthI >> 1;	}
	constexpr float	HalfScreenWidthF()		{ return Impl::screenWidthF * 0.5f;	}
	constexpr long	HalfScreenWidthL()		{ return Impl::screenWidthL >> 1;	}

	constexpr int	HalfScreenHeight()		{ return Impl::screenHeightI >> 1;	}
	constexpr float	HalfScreenHeightF()		{ return Impl::screenHeightF * 0.5f;}
	constexpr long	HalfScreenHeightL()		{ return Impl::screenHeightL >> 1;	}

	constexpr float	LargestDeltaTime()		{ return Impl::largestDeltaTime;	}

	void	SetShowCollision( bool newState );
	void	ToggleShowCollision();
	/// <summary>
	/// If when release mode, returns false.
	/// </summary>
	bool	IsShowCollision();
}
