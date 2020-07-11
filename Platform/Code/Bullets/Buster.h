#pragma once

#include "../Bullet.h"

namespace Bullet
{
	/// <summary>
	/// Kind of the player fires.
	/// You must call Init() when generate and Uninit() before remove. Because these method manages instance count.
	/// </summary>
	class Buster : public Base
	{
	private:
		static int livingCount;
	public:
		static int GetLivingCount();
	private:
	public:
		Buster( const Buster &  ) = default;
		Buster(       Buster && ) = default;
		Buster &operator = ( const Buster &  ) = default;
		Buster &operator = (       Buster && ) = default;
		virtual ~Buster() = default;
	public:
		void Init( const FireDesc &parameter ) override;
		void Uninit() override;
	public:
		Kind				GetKind()	const override;
		Definition::Damage	GetDamage()	const override;
	private:
		void AssignBodyParameter( const Donya::Vector3 &wsPos ) override;
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption ) override;
	#endif // USE_IMGUI
	};


	struct BusterParam
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
				CEREAL_NVP( hitBoxOffset ),
				CEREAL_NVP( hitBoxSize   )
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( damage ) );
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
CEREAL_CLASS_VERSION( Bullet::BusterParam, 1 )
