#pragma once

#include <vector>

#undef max
#undef min
#include "cereal/types/vector.hpp"

#include "../Donya/Serializer.h"
#include "../Donya/UseImGui.h"
#include "../Donya/Vector.h"

#include "../UI.h"

namespace Performer
{
	struct LoadParam
	{
	public:
		struct Icon
		{
			UIObject	config; // "pos" is used as posOffset
			float		cycleSecond = 1.0f;
			std::vector<Donya::Vector2> bounceOffsets;
			std::vector<Donya::Vector2> bounceStretches;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( config			),
					CEREAL_NVP( cycleSecond		),
					CEREAL_NVP( bounceOffsets	),
					CEREAL_NVP( bounceStretches	)
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
		struct String
		{
			UIObject		config; // "pos" is used as posOffset
			float			staySecond		= 1.0f;
			float			partPopSecond	= 1.0f;
			float			popInflRange	= 0.2f;
			Donya::Vector2	popOffset;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( config			),
					CEREAL_NVP( staySecond		),
					CEREAL_NVP( partPopSecond	),
					CEREAL_NVP( popInflRange	),
					CEREAL_NVP( popOffset		)
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
	public:
		Icon	partIcon;
		String	partString;
		float	fadeSecond = 0.5f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( partIcon	),
				CEREAL_NVP( partString	)
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( fadeSecond ) );
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
CEREAL_CLASS_VERSION( Performer::LoadParam,			1 )
CEREAL_CLASS_VERSION( Performer::LoadParam::Icon,	0 )
CEREAL_CLASS_VERSION( Performer::LoadParam::String,	0 )
