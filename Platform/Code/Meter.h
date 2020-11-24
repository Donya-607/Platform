#pragma once

#include "FontHelper.h"
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

	enum class Icon
	{
		Player,
		Skull,
	};

	class Drawer
	{
	private:
		float				current		= 0.0f;
		float				destination	= 0.0f;
		float				maxAmount	= 1.0f;
		float				shakeTimer	= 0.0f;
		float				shakeRadian = 0.0f;
		float				shakeAmpl	= 0.0f;
		Donya::Vector2		shake;
		mutable UIObject	sprite;
	public:
		void Init( float maxAmount, float startAmount, float destinationAmount );
		void Update( float elapsedTime );
		void Draw		( float drawDepth = 0.0f ) const;
		void DrawIcon	( Icon kind, float drawDepth = 0.0f ) const;
		void DrawRemains( FontAttribute font, int amount, float drawDepth = 0.0f ) const;
	public:
		void SetDestination( float currentAmount );
		void SetDrawOption( const Donya::Vector2 &ssPosLeftTop, const Donya::Vector3 &drawBlendColor, const Donya::Vector2 &drawScale );
		bool NowRecovering() const;
	private:
		void ShakeInit( float damage );
		void ShakeUpdate( float elapsedTime );
		void DrawGauge( float drawDepth ) const;
		void DrawAmount( float drawDepth ) const;
	};
}
