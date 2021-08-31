#pragma once

namespace Status
{
	/// <summary>
	/// Register current delta-time, that will be returned in GetDeltaTime().
	/// </summary>
	void SetDeltaTime( float currentDeltaTime );
	/// <summary>
	/// Returns current delta-time registered by SetDeltaTime().
	/// </summary>
	float GetDeltaTime();

	/// <summary>
	/// Returns raw delta-time, fetched by Donya library.
	/// </summary>
	float GetRawDeltaTime();
}
