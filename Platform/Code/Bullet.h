#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/Template.h"		// Use Donya::Singleton<>
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "ObjectBase.h"
#include "Renderer.h"

namespace Bullet
{
	bool LoadResource();

	/// <summary>
	/// Generate parameters. Descriptor.
	/// </summary>
	struct FireDesc
	{
	public:
		float			initialSpeed = 1.0f;			// [m/s]
		Donya::Vector3	direction{ 1.0f, 0.0f, 0.0f };	// Unit vector
		Donya::Vector3	position;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( initialSpeed	),
				CEREAL_NVP( direction		),
				CEREAL_NVP( position			)
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption, bool positionIsRelative = true );
	#endif // USE_IMGUI
	};


	/// <summary>
	/// Kind of the player fires.
	/// </summary>
	class Buster : Solid
	{
	private:
		using Solid::body;
		Donya::Vector3		velocity; // [m/s]
		Donya::Quaternion	orientation;
		bool				wantRemove = false;
	public:
		void Init( const FireDesc &parameter );
		void Uninit();
		void Update( float elasedTime, const Donya::Collision::Box2F &wsScreenHitBox );
		void PhysicUpdate( float elasedTime );
		void Draw( RenderingHelper *pRenderer ) const;
		void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		bool ShouldRemove() const;
		bool OnOutSide( const Donya::Collision::Box2F &wsScreenHitBox ) const;
	protected:
		Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const;
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};


	/// <summary>
	/// Container of all bullets.
	/// </summary>
	class Admin : public Donya::Singleton<Admin>
	{
		friend Donya::Singleton<Admin>;
	private:
		std::vector<Buster>		bullets;
		std::vector<FireDesc>	generateRequests;
	private:
		Admin() = default;
	public:
		void Update( float elapsedTime, const Donya::Collision::Box2F &wsScreenHitBox );
		void PhysicUpdate( float elasedTime );
		void Draw( RenderingHelper *pRenderer ) const;
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		void ClearInstances();
		void RequestFire( const FireDesc &parameter );
	private:
		void GenerateRequestedFires();
		void RemoveInstancesIfNeeds();
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Bullet::FireDesc, 0 )
