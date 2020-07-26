#pragma once

#include "UI.h"

namespace Meter
{
	struct MeterParam;
	namespace Parameter
	{
		void Load();

	#if USE_IMGUI
		void Update( const std::string &nodeCaption );
	#endif // USE_IMGUI
		
		const MeterParam &GetMeter();

		namespace Impl
		{
			void LoadMeter();
		#if USE_IMGUI
			void UpdateMeter( const std::string &nodeCaption );
		#endif // USE_IMGUI
		}
	}

	bool LoadResource();

	class Drawer
	{
	private:
		float	current		= 0.0f;
		float	maxAmount	= 1.0f;
		mutable UIObject sprite;
	public:
		void Init( float maxAmount, float initialAmount );
		void Update( float elapsedTime );
		void Draw( float drawDepth = 0.0f ) const;
	public:
		void SetCurrent( float currentAmount );
		void SetDrawOption( const Donya::Vector2 &ssPosLeftTop, const Donya::Vector3 &drawBlendColor );
	private:
		void DrawGauge( float drawDepth ) const;
		void DrawAmount( float drawDepth ) const;
	};
}
