#include "GameStatus.h"

#include <algorithm>		// Use std::min()

#include "Donya/Donya.h"	// Use Donya::GetElapsedTime()

#include "Common.h"			// Use LargestDeltaTime()

#undef max
#undef min

namespace Status
{
	static float gDeltaTime = 1.0f / 60.0f;

	void SetDeltaTime( float currentDeltaTime )
	{
		gDeltaTime = currentDeltaTime;
	}
	float GetDeltaTime()
	{
		return gDeltaTime;
	}

	float GetRawDeltaTime()
	{
		// Prevent to be very large
		return std::min( Common::LargestDeltaTime(), Donya::GetElapsedTime() );
	}
}
