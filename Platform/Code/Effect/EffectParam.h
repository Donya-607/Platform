#pragma once

#include <cereal/types/vector.hpp>

#include "../Donya/Serializer.h"	
#include "../Donya/UseImGui.h"		// Use USE_IMGUI macro

namespace Effect
{
	struct Param
	{
	public:
		std::vector<float> effectScales; // size() == Effect::Kind::KindCount
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( effectScales ) );

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode(); // Implement at EffectAdmin.cpp
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Effect::Param, 0 )
