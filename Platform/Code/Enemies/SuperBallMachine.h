#pragma once

#include "../Bullet.h"
#include "../Damage.h"
#include "../Enemy.h"

namespace Enemy
{
	class SuperBallMachine : public Base
	{
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				cereal::base_class<Base>( this )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox ) override;
	public:
		Kind GetKind() const override;
		Definition::Damage GetTouchDamage() const override;
	private:
		int  GetInitialHP() const override;
		void AssignMyBody( const Donya::Vector3 &wsPos ) override;
	public:
	#if USE_IMGUI
		/// <summary>
		/// Returns the return value of ImGui::TreeNode().
		/// </summary>
		bool ShowImGuiNode( const std::string &nodeCaption ) override;
	#endif // USE_IMGUI
	};

	struct SuperBallMachineParam
	{
	public:
		BasicParam			basic;
		Donya::Vector3		capturingArea{ 1.0f, 1.0f, 0.0f }; // Relative area. Machine will capture a target is there in this
		float				fireIntervalSecond	= 1.0f;
		float				fireDegree			= 45.0f;	// XY axis. Right side angle. If you using it when left side, use as: "180 - reflectDegree"
		Bullet::FireDesc	fireDesc;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( basic				),
				CEREAL_NVP( capturingArea		),
				CEREAL_NVP( fireIntervalSecond	),
				CEREAL_NVP( fireDegree			),
				CEREAL_NVP( fireDesc			)
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
CEREAL_CLASS_VERSION( Enemy::SuperBallMachine, 0 )
CEREAL_REGISTER_TYPE( Enemy::SuperBallMachine )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Enemy::Base, Enemy::SuperBallMachine )
CEREAL_CLASS_VERSION( Enemy::SuperBallMachineParam, 2 )
