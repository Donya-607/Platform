#pragma once

#include <vector>

#include "Donya/UseImGui.h"	// Use USE_IMGUI macro

namespace Input
{
	class BufferedInput
	{
	private:
		struct Part
		{
			float	elapsedSecond	= 0.0f;	// Elapsed second from pushed
			bool	pressed			= false;
			mutable	bool pickedUp	= false;
		};
	private:
		float lifeSpanSecond  = 1.0f;
		bool  lastAddedStatus = false;	// "status" means "pressed"
		std::vector<Part> buffer;		// begin:Last ~ end:Latest
		std::vector<Part> publicBuffer;	// Support multiple call of accessor in same frame
	public:
		/// <summary>
		/// Clear records
		/// </summary>
		void Reset();
		/// <summary>
		/// Clear records and set a life span of recorded inputs
		/// </summary>
		void Init( float maxRecordSecond = 1.0f );
		/// <summary>
		/// Update records' life span and record new input
		/// </summary>
		void Update( float elapsedTime, bool currentFrameIsPressed );
	public:
		/// <summary>
		/// It returns found part's "elapsedSecond" or negative value like -1.0f if not found.
		/// </summary>
		float PressingSecond( float allowSecond, bool ignorePickedUpInstance = true, bool discardFoundInstance = true ) const;
	public:
		bool IsReleased( float allowSecond, bool ignorePickedUpInstance = true, bool discardFoundInstance = true ) const;
		bool IsTriggered( float allowSecond, bool ignorePickedUpInstance = true, bool discardFoundInstance = true ) const;
	public:
		/// <summary>
		/// Second
		/// </summary>
		float GetLifeSpan() const;
		void  SetLifeSpan( float newMaxRecordSecond );
	private:
		void DiscardByLifeSpan();
		void AppendIfRecordable( bool pressed );
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const char *nodeCaption, size_t lowestContentCount = 3 );
	#endif // USE_IMGUI
	};
}
