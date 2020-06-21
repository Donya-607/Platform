#pragma once

namespace Common
{
	int		ScreenWidth();
	float	ScreenWidthF();
	long	ScreenWidthL();

	int		ScreenHeight();
	float	ScreenHeightF();
	long	ScreenHeightL();

	int		HalfScreenWidth();
	float	HalfScreenWidthF();
	long	HalfScreenWidthL();

	int		HalfScreenHeight();
	float	HalfScreenHeightF();
	long	HalfScreenHeightL();

	void	SetShowCollision( bool newState );
	void	ToggleShowCollision();
	/// <summary>
	/// If when release mode, returns false.
	/// </summary>
	bool	IsShowCollision();
}
