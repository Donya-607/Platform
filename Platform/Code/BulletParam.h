#pragma once

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"		// use US_IMGUI macro

namespace Bullet
{
	struct BusterParam
	{
	public:
		Donya::Vector3 hitBoxOffset{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3 hitBoxSize  { 1.0f, 1.0f, 1.0f };
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
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Bullet::BusterParam, 0 )
