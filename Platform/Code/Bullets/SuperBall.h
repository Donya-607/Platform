#pragma once

#include "../Bullet.h"
#include "../BulletParam.h"

namespace Bullet
{
	/// <summary>
	/// Kind of the skull fires.
	/// </summary>
	class SuperBall final : public Base
	{
	private:
		int accelCount = 0;
	public:
		void Init( const FireDesc &parameter ) override;
		void Uninit() override;
		void PhysicUpdate( float elapsedTime, const Map &terrain ) override;
	public:
		Kind GetKind() const override;
	private:
		Definition::Damage GetDamageParameter() const override;
		void AssignBodyParameter( const Donya::Vector3 &wsPos ) override;
	};

	struct SuperBallParam
	{
	public:
		BasicParam	basic;
		int			accelerateCount		= 3;
		float		acceleratePercent	= 1.1f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( basic				),
				CEREAL_NVP( accelerateCount		),
				CEREAL_NVP( acceleratePercent	)
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
CEREAL_CLASS_VERSION( Bullet::SuperBallParam, 0 )
