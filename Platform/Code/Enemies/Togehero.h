#pragma once

#include "../Damage.h"
#include "../Enemy.h"

namespace Enemy
{
	class Togehero : public Base
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

	struct TogeheroParam
	{
	public:
		BasicParam	basic;
		float		moveSpeed		= 1.0f;		// [m/s]
		float		rotateSpeed		= 360.0f;	// [degree/s]
		float		animePlaySpeed	= 1.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			if ( 2 <= version )
			{
				archive
				(
					CEREAL_NVP( basic		),
					CEREAL_NVP( moveSpeed	),
					CEREAL_NVP( rotateSpeed	)
				);
			}
			if ( 3 <= version )
			{
				archive( CEREAL_NVP( animePlaySpeed ) );
			}
			if ( 4 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
			return;
			// else

			/*
			archive
			(
				CEREAL_NVP( hitBoxOffset	),
				CEREAL_NVP( hurtBoxOffset	),
				CEREAL_NVP( hitBoxSize		),
				CEREAL_NVP( hurtBoxSize		),
				CEREAL_NVP( moveSpeed		),
				CEREAL_NVP( rotateSpeed		)
			);

			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( hp			),
					CEREAL_NVP( touchDamage	)
				);
			}
			*/
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Enemy::Togehero, 0 )
CEREAL_REGISTER_TYPE( Enemy::Togehero )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Enemy::Base, Enemy::Togehero )
CEREAL_CLASS_VERSION( Enemy::TogeheroParam, 3 )
