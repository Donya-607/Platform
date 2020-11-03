#pragma once

#include <vector>

#undef max
#undef min
#include "cereal/types/vector.hpp"

#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

struct PauseParam
{
public:
	float chooseItemScale		= 1.5f;
	float otherItemScale		= 1.0f;
	std::vector<Donya::Vector2> itemPositions; // Screen space. size() == PauseProcessor::Choice::ItemCount.
	float			titleScale	= 1.0f;
	Donya::Vector2	titlePos;
	float soundVolume			= 0.5f;
	float fadeSoundSecond		= 0.5f;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( chooseItemScale	),
			CEREAL_NVP( otherItemScale	),
			CEREAL_NVP( itemPositions	)
		);

		if ( 1 <= version )
		{
			archive
			(
				CEREAL_NVP( titleScale	),
				CEREAL_NVP( titlePos	)
			);
		}
		if ( 2 <= version )
		{
			archive( CEREAL_NVP( soundVolume ) );
		}
		if ( 3 <= version )
		{
			archive( CEREAL_NVP( fadeSoundSecond ) );
		}
		if ( 4 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
#if USE_IMGUI
	void ShowImGuiNode();
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( PauseParam, 3 )
