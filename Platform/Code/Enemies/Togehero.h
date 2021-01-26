#pragma once

#include "../Damage.h"
#include "../Enemy.h"

namespace Enemy
{
	/// <summary>
	/// Generator of Bullet::TogeheroBody
	/// </summary>
	class Togehero : public Base
	{
	private:
		float prevIncludeSecond = 0.0f;
		float currIncludeSecond = 0.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				cereal::base_class<Base>( this ),
				CEREAL_NVP( body )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		void Init( const InitializeParam &parameter, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox ) override;
		void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox ) override;
		void PhysicUpdate( float elapsedTime, const Map &terrain, bool considerBodyExistence = true ) override;
		void Draw( RenderingHelper *pRenderer ) const override;
		void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const override;
	public:
		Kind				GetKind()			const override;
		Definition::Damage	GetTouchDamage()	const override;
	private:
		int  GetInitialHP() const override;
		void AssignMyBody( const Donya::Vector3 &wsPos ) override;
	private:
		void Generate( const Donya::Vector3 &wsTargetPos ) const;
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
		float		bodySpeed			= 1.0f;		// [m/s]
		float		generateInterval	= 1.0f;
		float		generatePosOffsetH	= 10.0f;	// Second
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( basic				),
				CEREAL_NVP( bodySpeed			),
				CEREAL_NVP( generateInterval	),
				CEREAL_NVP( generatePosOffsetH	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP(  ) );
			}
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
CEREAL_CLASS_VERSION( Enemy::TogeheroParam, 0 )
