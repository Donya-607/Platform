#pragma once

#include "../Bullet.h"
#include "../BulletParam.h"
#include "../Donya/Serializer.h"

namespace Bullet
{
	/// <summary>
	/// Kind of the skull fires.
	/// </summary>
	class Bone final : public Base
	{
	public:
		void Uninit() override;
		void Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox ) override;
	public:
		Kind GetKind() const override;
	private:
		void GenerateCollidedEffect() const override;
		void PlayCollidedSE() const override;
	private:
		Definition::Damage GetDamageParameter() const override;
		void AssignBodyParameter( const Donya::Vector3 &wsPos ) override;
	};

	struct BoneParam
	{
	public:
		BasicParam	basic;
		float		gravity = 1.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( basic	),
				CEREAL_NVP( gravity	)
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
CEREAL_CLASS_VERSION( Bullet::BoneParam, 0 )
