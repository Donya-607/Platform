#pragma once

#include "Donya/Collision.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

namespace Door
{
	struct Parameter
	{
		Donya::Collision::Box3F hitBox;				// The "pos" is not used
		Donya::Collision::Box3F triggerAreaBase;	// The "pos" is not used
		float					triggerAreaAddSize	= 0.1f;	// The size of magnifying to the direction that can pass-throughing
		float animeSpeedOpen	= 1.0f;
		float animeSpeedClose	= 1.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( hitBox				),
				CEREAL_NVP( triggerAreaBase		),
				CEREAL_NVP( triggerAreaAddSize	),
				CEREAL_NVP( animeSpeedOpen		),
				CEREAL_NVP( animeSpeedClose		)
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
CEREAL_CLASS_VERSION( Door::Parameter, 0 )
