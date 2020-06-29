#pragma once

#include "Donya/Quaternion.h"

#include "Enemy.h"

namespace Enemy
{
	class Terry : public Base
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
		void Init( const InitializeParam &parameter ) override;
		void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos ) override;
	public:
		Kind GetKind() const override;
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption ) override;
	#endif // USE_IMGUI
	};

	struct TerryParam
	{
	public:
		Donya::Vector3 hitBoxOffset	{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3 hurtBoxOffset{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3 hitBoxSize	{ 1.0f, 1.0f, 1.0f };
		Donya::Vector3 hurtBoxSize	{ 1.0f, 1.0f, 1.0f };

		float moveSpeed		= 1.0f;		// [m/s]
		float rotateSpeed	= 360.0f;	// [degree/s]
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
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
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Enemy::Terry, 0 )
CEREAL_REGISTER_TYPE( Enemy::Terry )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Enemy::Base, Enemy::Terry )
