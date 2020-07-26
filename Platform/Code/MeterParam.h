#pragma once

#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"

namespace Meter
{
	struct MeterParam
	{
	public:
		Donya::Vector2 gaugeTexOrigin	{  0.0f,  0.0f };
		Donya::Vector2 gaugeTexSize		{ 32.0f, 32.0f };
		Donya::Vector2 amountTexOrigin	{  0.0f,  0.0f };
		Donya::Vector2 amountTexSize	{ 32.0f, 32.0f };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( gaugeTexOrigin	),
				CEREAL_NVP( gaugeTexSize	),
				CEREAL_NVP( amountTexOrigin	),
				CEREAL_NVP( amountTexSize	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Meter::MeterParam, 0 )
