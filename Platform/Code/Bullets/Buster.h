#pragma once

#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "../Donya/ModelCommon.h"

#include "../Bullet.h"
#include "../BulletParam.h"
#include "../ModelHelper.h"	// Use serialize method
#include "../Player.h"		// Use Player::ShotLevel

namespace Bullet
{
	/// <summary>
	/// Kind of the player fires.
	/// You must call Init() when generate and Uninit() before remove. Because these method manages instance count.
	/// </summary>
	class Buster final : public Base
	{
	private:
		static int livingCount;
	public:
		static int GetLivingCount();
	private:
		Player::ShotLevel chargeLevel;
	public:
		void Init( const FireDesc &parameter ) override;
		void Uninit() override;
		void Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox ) override;
	public:
		Kind GetKind() const override;
	private:
		Definition::Damage GetDamageParameter() const override;
		void AssignBodyParameter( const Donya::Vector3 &wsPos ) override;
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption ) override;
	#endif // USE_IMGUI
	};

	struct BusterParam
	{
	public:
		struct PerLevel
		{
			BasicParam	basic;
			float		accelRate = 1.0f; // Multiplies the fired speed
			Donya::Model::Constants::PerScene::PointLight lightSource; // The "wsPos" will be used as an offset of world space
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( basic		),
					CEREAL_NVP( accelRate	)
				);

				if ( 1 <= version )
				{
					archive( CEREAL_NVP( lightSource ) );
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
	public:
		std::vector<PerLevel> params; // size() == Player::ShotLevel::LevelCount
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			if ( 2 <= version )
			{
				archive( CEREAL_NVP( params ) );
			}

			return;

			/*
			archive
			(
				CEREAL_NVP( hitBoxOffset ),
				CEREAL_NVP( hitBoxSize   )
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( damage ) );
			}
			*/
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Bullet::BusterParam,				2 )
CEREAL_CLASS_VERSION( Bullet::BusterParam::PerLevel,	1 )
