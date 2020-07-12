#pragma once

#include "../Bullet.h"

namespace Bullet
{
	/// <summary>
	/// Kind of the skull fires.
	/// </summary>
	class SkullBuster final : public Base
	{
	public:
		void Uninit() override;
	public:
		Kind				GetKind()	const override;
		Definition::Damage	GetDamage()	const override;
	private:
		void AssignBodyParameter( const Donya::Vector3 &wsPos ) override;
	};

	struct SkullBusterParam
	{
	public:
		Donya::Vector3		hitBoxOffset{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3		hitBoxSize  { 1.0f, 1.0f, 1.0f };
		Definition::Damage	damage;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( hitBoxOffset	),
				CEREAL_NVP( hitBoxSize		),
				CEREAL_NVP( damage			)
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
CEREAL_CLASS_VERSION( Bullet::SkullBusterParam, 0 )