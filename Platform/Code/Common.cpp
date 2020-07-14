#include "Common.h"

#include "Donya/Constant.h"	// Use scast, DEBUG_MODE macros.

namespace Common
{
#if DEBUG_MODE
	static bool showCollision = true;
#endif // DEBUG_MODE
	void	SetShowCollision( bool newState )
	{
	#if DEBUG_MODE
		showCollision = newState;
	#endif // DEBUG_MODE
	}
	void	ToggleShowCollision()
	{
	#if DEBUG_MODE
		showCollision = !showCollision;
	#endif // DEBUG_MODE
	}
	bool	IsShowCollision()
	{
	#if DEBUG_MODE
		return showCollision;
	#else
		return false;
	#endif // DEBUG_MODE
	}
}
