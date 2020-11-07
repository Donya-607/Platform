#pragma once

#include <vector>

#undef max
#undef min
#include "cereal/types/vector.hpp"

#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Input.h"
#include "UI.h"

namespace Input
{
	struct Parameter
	{
		std::vector<UIObject> texViews; // size() == Input::Type::TypeCount
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( texViews ) );

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
CEREAL_CLASS_VERSION( Input::Parameter, 0 )
