#pragma once

#include <string>

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"		// use US_IMGUI macro

#include "Bullet.h"				// Use Bullet::FireDesc

struct PlayerParam
{
public:
	int		maxHP				= 28;
	int		maxBusterCount		= 3;		// Max generatable count of buster at same time.
	float	moveSpeed			= 1.0f;
	float	slideMoveSpeed		= 1.0f;
	float	slideMoveSeconds	= 1.0f;
	float	jumpStrength		= 1.0f;
	float	gravity				= 1.0f;
	float	gravityResistance	= 0.5f;		// Multiply to gravity if while pressing a jump key
	float	resistableSeconds	= 0.5f;
	float	maxFallSpeed		= 1.0f;
	float	knockBackSeconds	= 0.5f;
	float	knockBackSpeed		= 1.0f;		// X speed
	float	invincibleSeconds	= 2.0f;
	float	flushingInterval	= 0.1f;		// Seconds
	Donya::Collision::Box3F	hitBox;			// VS a terrain
	Donya::Collision::Box3F	hurtBox;		// VS an attack(e.g. enemy)
	Donya::Collision::Box3F	slideHitBox;	// VS a terrain when sliding
	Donya::Collision::Box3F	slideHurtBox;	// VS an attack(e.g. enemy) when sliding
	Bullet::FireDesc		fireParam;
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
			archive( CEREAL_NVP( fireParam ) );
		}
		if ( 2 <= version )
		{
			archive( CEREAL_NVP( maxBusterCount ) );
		}
		if ( 3 <= version )
		{
			archive
			(
				CEREAL_NVP( maxHP				),
				CEREAL_NVP( knockBackSeconds	),
				CEREAL_NVP( knockBackSpeed		),
				CEREAL_NVP( invincibleSeconds	),
				CEREAL_NVP( flushingInterval	)
			);
		}
		if ( 4 <= version )
		{
			archive
			(
				CEREAL_NVP( slideMoveSpeed		),
				CEREAL_NVP( slideMoveSeconds	),
				CEREAL_NVP( slideHitBox			),
				CEREAL_NVP( slideHurtBox		)
			);
		}
		if ( 5 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
#if USE_IMGUI
	void ShowImGuiNode();
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( PlayerParam, 4 )
