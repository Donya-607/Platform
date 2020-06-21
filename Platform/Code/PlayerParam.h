#pragma once

#include <string>

#include "Donya/Collision.h"
#include "Donya/Serializer.h"

struct PlayerParam
{
public:
	float	moveSpeed			= 1.0f;
	float	jumpStrength		= 1.0f;
	float	gravity				= 1.0f;
	float	gravityResistance	= 0.5f;	// Multiply to gravity if while pressing a jump key
	float	resistableSeconds	= 0.5f;
	float	maxFallSpeed		= 1.0f;
	Donya::Collision::Box3F hitBox;		// VS a terrain
	Donya::Collision::Box3F hurtBox;	// VS an attack(e.g. enemy)
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( moveSpeed			),
			CEREAL_NVP( jumpStrength		),
			CEREAL_NVP( gravity				),
			CEREAL_NVP( gravityResistance	),
			CEREAL_NVP( resistableSeconds	),
			CEREAL_NVP( maxFallSpeed		),
			CEREAL_NVP( hitBox				),
			CEREAL_NVP( hurtBox				)
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
CEREAL_CLASS_VERSION( PlayerParam, 0 )
