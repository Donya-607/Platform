#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/Template.h"		// Use Donya::Singleton<>
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "ObjectBase.h"
#include "Renderer.h"

namespace Bullet
{
	/// <summary>
	/// Generate parameters. Descriptor.
	/// </summary>
	struct FireDesc
	{
	public:
		float			initialSpeed = 1.0f;			// [m/s]
		Donya::Vector3	direction{ 1.0f, 0.0f, 0.0f };	// Unit vector
		Donya::Vector3	wsPos;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( initialSpeed	),
				CEREAL_NVP( direction		),
				CEREAL_NVP( wsPos			)
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	};


	/// <summary>
	/// Kind of the player fires.
	/// </summary>
	class Buster : Solid
	{
	private:
		using Solid::body;
		Donya::Vector3	velocity; // [m/s]
		bool			wantRemove = false;
	public:
		void Init( const FireDesc &parameter );
		void Uninit();
		void Update( float elasedTime );
		void Draw( RenderingHelper *pRenderer ) const;
		void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		bool ShouldRemove() const;
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
		Admin();
	public:
		void Update( float elapsedTime );
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
