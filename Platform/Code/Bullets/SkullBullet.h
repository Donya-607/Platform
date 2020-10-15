#pragma once

#include "../Bullet.h"
#include "../BulletParam.h"

namespace Bullet
{
	/// <summary>
	/// Kind of the skull fires.
	/// </summary>
	class SkullBuster final : public Base
	{
	public:
		void Uninit() override;
		void Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox ) override;
	public:
		Kind GetKind() const override;
	private:
		void GenerateHitEffect() const override;
		void PlayCollidedSE() const override;
	private:
		Definition::Damage GetDamageParameter() const override;
		void AssignBodyParameter( const Donya::Vector3 &wsPos ) override;
	};

	struct SkullBusterParam
	{
	public:
		BasicParam basic;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			if ( 1 <= version )
			{
				archive( CEREAL_NVP( basic ) );
			}
			return;

			/*
			archive
			(
				CEREAL_NVP( hitBoxOffset	),
				CEREAL_NVP( hitBoxSize		),
				CEREAL_NVP( damage			)
			);
			*/
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
	

	/// <summary>
	/// Kind of the skull shields.
	/// </summary>
	class SkullShield final : public Base
	{
	private:
		float currentDegree = 0.0f;
	public:
		void Init( const FireDesc &parameter ) override;
		void Uninit() override;
		void Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox ) override;
		void Draw( RenderingHelper *pRenderer ) const override;
		void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const override;
	public:
		Donya::Collision::Sphere3F GetHitSphereSubtractor() const override;
		Kind GetKind() const override;
	private:
		void GenerateHitEffect() const override;
		void PlayCollidedSE() const override;
	private:
		Definition::Damage GetDamageParameter() const override;
		void AssignBodyParameter( const Donya::Vector3 &wsPos ) override;
	};

	struct SkullShieldParam
	{
	public:
		BasicParam			basic;
		int					partCount			= 4;
		float				rotateDegree		= 1.0f; // per second
		float				drawPartOffset		= 1.0f;
		float				subtractorRadius	= 1.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( basic			),
					CEREAL_NVP( partCount		),
					CEREAL_NVP( rotateDegree	),
					CEREAL_NVP( drawPartOffset	)
				);

				if ( 2 <= version )
				{
					archive( CEREAL_NVP( subtractorRadius ) );
				}
				if ( 3 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
			return;

			/*
			archive
			(
				CEREAL_NVP( partCount		),
				CEREAL_NVP( rotateDegree	),
				CEREAL_NVP( drawPartOffset	),
				CEREAL_NVP( hitBoxOffset	),
				CEREAL_NVP( hitBoxRadius	),
				CEREAL_NVP( damage			)
			);
			*/
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Bullet::SkullBusterParam, 1 )
CEREAL_CLASS_VERSION( Bullet::SkullShieldParam, 2 )
