#pragma once

#include "../Bullet.h"
#include "../BulletParam.h"

namespace Bullet
{
	/// <summary>
	/// Main body part of the Enemy::Togehero.
	/// </summary>
	class TogeheroBody final : public Base
	{
	private:
		float	zigzagTimer			= 0.0f;
		bool	wantRemoveByOutSide	= false;
	public:
		void Init( const FireDesc &parameter ) override;
		void Uninit() override;
		void Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox ) override;
	private:
		void UpdateVerticalVelocity( bool halfWay = false );
	public:
		bool Destructible() const override;
		bool WasProtected() const override;
		void CollidedToObject( bool otherIsBroken, bool otherIsBullet ) const override;
		void ProtectedBy( const Donya::Collision::Box3F &protectObjectBody ) const override;
		void ProtectedBy( const Donya::Collision::Sphere3F &protectObjectBody ) const override;
	private:
		void ProtectedByImpl( float distLeft, float distRight ) const override;
		void ProcessOnOutSide() override;
	public:
		Kind GetKind() const override;
	private:
		void GenerateCollidedEffect() const override;
		void GenerateProtectedEffect() const override;
		void PlayCollidedSE() const override;
		void PlayProtectedSE() const override;
		void CollidedProcess() override;
		void ProtectedProcess() override;
	private:
		Definition::Damage GetDamageParameter() const override;
		void AssignBodyParameter( const Donya::Vector3 &wsPos ) override;
	};

	struct TogeheroBodyParam
	{
	public:
		BasicParam	basic;
		float		rotateSpeed		= 360.0f;	// [degree/s]
		float		zigzagDistance	= 1.0f;
		float		zigzagInterval	= 1.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( basic			),
				CEREAL_NVP( rotateSpeed		),
				CEREAL_NVP( zigzagDistance	),
				CEREAL_NVP( zigzagInterval	)
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
CEREAL_CLASS_VERSION( Bullet::TogeheroBodyParam, 0 )
