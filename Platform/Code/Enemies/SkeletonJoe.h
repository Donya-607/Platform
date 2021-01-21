#pragma once

#include "../Bullet.h"
#include "../Damage.h"
#include "../Enemy.h"

namespace Enemy
{
	class SkeletonJoe : public Base
	{
	public:
		enum class MotionKind
		{
			Idle = 0,
			Fire,
			Break,
			ReAssemble,

			MotionCount
		};
	private:
		MotionKind	status	= MotionKind::Idle;
		float		timer	= 0.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( cereal::base_class<Base>( this ) );
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		void Init( const InitializeParam &parameter, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox ) override;
		void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox ) override;
		void PhysicUpdate( float elapsedTime, const Map &terrain, bool considerBodyExistence = true ) override;
	public:
		Kind GetKind() const override;
		Definition::Damage GetTouchDamage() const override;
	private:
		void ApplyReceivedDamageIfHas() override;
	private:
		int  GetInitialHP() const override;
		void AssignMyBody( const Donya::Vector3 &wsPos ) override;
	private:
		void UpdateOrientation( const Donya::Vector3 &direction );
		void ChangeMotion( MotionKind nextKind );
		void ChangeState( MotionKind nextKind );
		void ToNextState( const Donya::Vector3 &wsTargetPos );
		void Shot( const Donya::Vector3 &wsTargetPos );
	private:
		void IdleInit();
		void FireInit( const Donya::Vector3 &wsTargetPos );
		void BreakInit();
		void ReAssembleInit();
	public:
	#if USE_IMGUI
		/// <summary>
		/// Returns the return value of ImGui::TreeNode().
		/// </summary>
		bool ShowImGuiNode( const std::string &nodeCaption ) override;
	#endif // USE_IMGUI
	};

	struct SkeletonJoeParam
	{
	public:
		BasicParam			basic;
		Bullet::FireDesc	fireDesc;
		float				gravity				= 1.0f;
		float				impactSecond		= 1.0f;
		std::vector<float>	stateSeconds;		// Staying second of each state. size() == SkeletonJoe::MotionKind::MotionCount
		std::vector<float>	animePlaySpeeds;	// size() == SkeletonJoe::MotionKind::MotionCount
		std::vector<float>	animeTransSeconds;	// It interest only destination motion kind. It size() == Player::MotionKind::MotionCount
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( basic			),
				CEREAL_NVP( fireDesc		),
				CEREAL_NVP( gravity			),
				CEREAL_NVP( impactSecond	),
				CEREAL_NVP( stateSeconds	),
				CEREAL_NVP( animePlaySpeeds	)
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( animeTransSeconds ) );
			}
			if ( 2 <= version )
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
CEREAL_CLASS_VERSION( Enemy::SkeletonJoe, 0 )
CEREAL_REGISTER_TYPE( Enemy::SkeletonJoe )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Enemy::Base, Enemy::SkeletonJoe )
CEREAL_CLASS_VERSION( Enemy::SkeletonJoeParam, 1 )
