#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#undef max
#undef min
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Collision.h"
#include "Donya/Easing.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"		// use US_IMGUI macro

#include "Bullet.h"				// Use Bullet::FireDesc
#include "ModelHelper.h"		// Use PartApply

struct PlayerParam
{
public:
	int		maxHP					= 28;
	int		maxBusterCount			= 3;			// Max generatable count of buster at same time.
	int		maxRemainCount			= 9;
	int		initialRemainCount		= 2;
	float	moveSpeed				= 1.0f;
	float	inertialMoveSpeed		= 1.0f;
	float	shotBufferSecond		= 0.2f;			// Allow second of pre-input
	float	slideMoveSpeed			= 1.0f;
	float	slideMoveSecond			= 1.0f;
	float	slideBufferSecond		= 0.2f;			// Allow second of pre-input
	float	ladderMoveSpeed			= 1.0f;
	float	ladderShotLagSecond		= 0.5f;
	float	jumpStrength			= 1.0f;
	float	jumpBufferSecond		= 0.2f;			// Allow second of pre-input
	float	jumpCancelledVSpeedMax	= 1.0f;			// Max Y speed of the moment that jump input now released
	float	gravityRising			= 1.0f;
	float	gravityRisingAccel		= 1.0f;
	float	gravityFalling			= 1.0f;
	float	gravityFallingAccel		= 1.0f;
	float	gravityMax				= 1.0f;
	float	maxFallSpeed			= 1.0f;
	float	knockBackSecond			= 0.5f;
	float	knockBackSpeed			= 1.0f;			// X speed
	float	braceStandFactor		= 2.0f;
	float	invincibleSecond		= 2.0f;
	float	flushingInterval		= 0.1f;			// Seconds
	float	emissiveTransFactor		= 0.3f;
	float	appearDelaySecond		= 0.5f;
	float	leaveDelaySecond		= 0.5f;
	Donya::Collision::Box3F			hitBox;			// VS a terrain
	Donya::Collision::Box3F			hurtBox;		// VS an attack(e.g. enemy)
	Donya::Collision::Box3F			slideHitBox;	// VS a terrain when sliding
	Donya::Collision::Box3F			slideHurtBox;	// VS an attack(e.g. enemy) when sliding
	Donya::Collision::Box3F			ladderGrabArea;	// It using for considering to continue to grab the ladder
	Bullet::FireDesc				fireParam;
	std::vector<float>				animePlaySpeeds;	// It size() == Player::MotionKind::MotionCount
	std::vector<float>				animeTransSeconds;	// It interest only destination motion kind. It size() == Player::MotionKind::MotionCount

	std::unordered_map<int, ModelHelper::PartApply> partMotions; // Key is Player::MotionKind

	struct PerChargeLevel
	{
		float					chargeSecond		= 0.0f;
		float					emissiveCycleSecond	= 1.0f; // Cycle of the sinf()
		float					emissiveMinBias		= 0.0f; // Minimum value of the factor of color
		Donya::Easing::Kind		emissiveEaseKind	= Donya::Easing::Kind::Linear;
		Donya::Easing::Type		emissiveEaseType	= Donya::Easing::Type::Out;
		Donya::Vector3			emissiveColor{ 0.0f, 0.0f, 0.0f }; // Max lighten color
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( chargeSecond		),
				CEREAL_NVP( emissiveCycleSecond	),
				CEREAL_NVP( emissiveMinBias		),
				CEREAL_NVP( emissiveEaseKind	),
				CEREAL_NVP( emissiveEaseType	),
				CEREAL_NVP( emissiveColor		)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
	std::vector<PerChargeLevel>		chargeParams;	// It size() == Player::ShotLevel::LevelCount

	float							shieldThrowSpeed = 1.0f;
	Donya::Vector3					shieldPosOffset{ 0.0f, 0.0f, 0.0f };

	std::vector<Donya::Vector3>		themeColors;	// It size() == Player::GunKind::GunCount
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( moveSpeed			),
			CEREAL_NVP( jumpStrength		),
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
				CEREAL_NVP( knockBackSecond		),
				CEREAL_NVP( knockBackSpeed		),
				CEREAL_NVP( invincibleSecond	),
				CEREAL_NVP( flushingInterval	)
			);
		}
		if ( 4 <= version )
		{
			archive
			(
				CEREAL_NVP( slideMoveSpeed	),
				CEREAL_NVP( slideMoveSecond	),
				CEREAL_NVP( slideHitBox		),
				CEREAL_NVP( slideHurtBox	)
			);
		}
		if ( 5 <= version )
		{
			archive
			(
				CEREAL_NVP( maxRemainCount		),
				CEREAL_NVP( initialRemainCount	)
			);
		}
		if ( 6 <= version )
		{
			archive
			(
				CEREAL_NVP( ladderMoveSpeed		),
				CEREAL_NVP( ladderShotLagSecond	),
				CEREAL_NVP( ladderGrabArea		)
			);
		}
		if ( 7 <= version )
		{
			archive( CEREAL_NVP( animePlaySpeeds ) );
		}
		//if ( 8 <= version && version < 18 )
		if ( 9 <= version )
		{
			archive( CEREAL_NVP( chargeParams ) );
		}
		if ( 10 <= version )
		{
			archive
			(
				CEREAL_NVP( shieldThrowSpeed	),
				CEREAL_NVP( shieldPosOffset		)
			);
		}
		if ( 11 <= version )
		{
			archive( CEREAL_NVP( jumpBufferSecond ) );
		}
		if ( 12 <= version )
		{
			archive( CEREAL_NVP( appearDelaySecond ) );
		}
		if ( 13 <= version )
		{
			archive
			(
				CEREAL_NVP( braceStandFactor	),
				CEREAL_NVP( emissiveTransFactor	),
				CEREAL_NVP( leaveDelaySecond	)
			);
		}
		if ( 14 <= version )
		{
			archive( CEREAL_NVP( themeColors ) );
		}
		if ( 15 <= version )
		{
			archive
			(
				CEREAL_NVP( jumpCancelledVSpeedMax	),
				CEREAL_NVP( gravityRising			),
				CEREAL_NVP( gravityRisingAccel		),
				CEREAL_NVP( gravityFalling			),
				CEREAL_NVP( gravityFallingAccel		),
				CEREAL_NVP( gravityMax				)
			);
		}
		if ( 16 <= version )
		{
			archive( CEREAL_NVP( inertialMoveSpeed ) );
		}
		//if ( 17 <= version && version < 18 )
		if ( 18 <= version )
		{
			archive( CEREAL_NVP( partMotions ) );
		}
		if ( 19 <= version )
		{
			archive( CEREAL_NVP( animeTransSeconds ) );
		}
		if ( 20 <= version )
		{
			archive
			(
				CEREAL_NVP( shotBufferSecond  ),
				CEREAL_NVP( slideBufferSecond )
			);
		}
		if ( 21 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
#if USE_IMGUI
	void ShowImGuiNode();
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( PlayerParam, 20 )
