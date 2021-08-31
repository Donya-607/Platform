#include "GameStatus.h"

#include "Donya.h" // Use Donya::GetElapsedTime().

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
		return Donya::GetElapsedTime();
	}
}
