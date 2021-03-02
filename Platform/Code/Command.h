#pragma once

#include <vector>

#include "Donya/UseImGui.h"

#include "NumPad.h"

namespace Command
{
	struct Part
	{
		float marginSecond = 0.1f;
		std::vector<NumPad::Value> sticks;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( marginSecond	),
				CEREAL_NVP( sticks			)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const char *nodeCaption );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Command::Part, 0 )

