#pragma once

#include "../Donya/Serializer.h"
#include "../Donya/UseImGui.h"
#include "../Donya/Vector.h"

namespace Performer
{
	struct LoadParam
	{
	public:
		struct General
		{

		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP()
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP(  )
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
CEREAL_CLASS_VERSION( Performer::LoadParam,				0 )
CEREAL_CLASS_VERSION( Performer::LoadParam::General,	0 )
