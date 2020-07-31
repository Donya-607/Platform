#pragma once

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"		// Use US_IMGUI macro

#include "Damage.h"

namespace Bullet
{
	/// <summary>
	/// The parameter that will be applied to all bullet
	/// </summary>
	struct GeneralParam
	{
	public:
		float reflectSpeed	= 1.0f;
		float reflectDegree	= 45.0f;	// XY axis. Right side angle. If you using it when left side, use as: "180 - reflectDegree"
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( reflectSpeed  ),
				CEREAL_NVP( reflectDegree )
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

	/// <summary>
	/// The basically parameter used for level design
	/// </summary>
	struct BasicParam
	{
	public:
		Donya::Vector3		hitBoxOffset{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3		hitBoxSize	{ 1.0f, 1.0f, 1.0f }; // If it is used for a sphere as radius, the radius is as the X component.
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
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Bullet::GeneralParam,	0 )
CEREAL_CLASS_VERSION( Bullet::BasicParam,	0 )
