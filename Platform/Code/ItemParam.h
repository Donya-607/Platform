#pragma once

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"

namespace Item
{
	struct ItemParam
	{
	public:
		struct General
		{
		public:
			float gravity = 1.0f;
			Donya::Collision::Box3F body;
			Donya::Collision::Box3F catchArea;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( gravity		),
					CEREAL_NVP( body		),
					CEREAL_NVP( catchArea	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		public:
		#if USE_IMGUI
			void ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode = true );
		#endif // USE_IMGUI
		};
	public:
		General extraLife;
		General lifeEnergyBig;
		General lifeEnergySmall;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( extraLife		),
				CEREAL_NVP( lifeEnergyBig	),
				CEREAL_NVP( lifeEnergySmall	)
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
CEREAL_CLASS_VERSION( Item::ItemParam,			0 )
CEREAL_CLASS_VERSION( Item::ItemParam::General,	0 )
