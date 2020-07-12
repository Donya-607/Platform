#pragma once

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"		// use US_IMGUI macro

#include "Damage.h"

namespace Bullet
{
	struct GeneralParam
	{
	public:
		float reflectSpeed	= 1.0f;
		float reflectDegree	= 45.0f;	// XY axis. Right side angle. If you using it when left side, use as: "180 - reflectDegree"
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( reflectSpeed  ),
				CEREAL_NVP( reflectDegree )
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
CEREAL_CLASS_VERSION( Bullet::GeneralParam, 0 )
