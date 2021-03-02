#pragma once

#include "../Bullet.h"
#include "../BulletParam.h"

namespace Bullet
{
	/// <summary>
	/// Just the collision of Shoryuken.
	/// </summary>
	class ShoryuCol final : public Base
	{
	public:
		void Init( const FireDesc &parameter ) override;
		void Uninit() override;
		void Draw( RenderingHelper *pRenderer ) const override;
	public:
		Kind GetKind() const override;
	private:
		void GenerateCollidedEffect() const override;
		void PlayCollidedSE() const override;
	private:
		Definition::Damage GetDamageParameter() const override;
		void AssignBodyParameter( const Donya::Vector3 &wsPos ) override;
	};

	struct ShoryuColParam
	{
	public:
		BasicParam basic;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( basic ) );

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
CEREAL_CLASS_VERSION( Bullet::ShoryuColParam, 0 )
