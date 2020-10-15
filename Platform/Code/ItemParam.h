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
			float dropBoundStrength	= 1.0f;
			float gravity			= 1.0f;
			float animePlaySpeed	= 1.0f;
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
					archive( CEREAL_NVP( animePlaySpeed ) );
				}
				if ( 2 <= version )
				{
					archive( CEREAL_NVP( dropBoundStrength ) );
				}
				if ( 3 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		public:
		#if USE_IMGUI
			void ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode = true );
		#endif // USE_IMGUI
		};
		struct Energy
		{
		public:
			int		recoveryAmount = 1;
			General	general;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( recoveryAmount	),
					CEREAL_NVP( general			)
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
		General				extraLife;
		Energy				lifeEnergyBig;
		Energy				lifeEnergySmall;
		float				disappearSecond = 1.0f;	// It using for drop item
		std::vector<int>	dropPercents;			// Also contain the percent of do not drop. size() == Item::Kind::KindCount + 1
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
				archive
				(
					CEREAL_NVP( disappearSecond	),
					CEREAL_NVP( dropPercents	)
				);
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
CEREAL_CLASS_VERSION( Item::ItemParam,			1 )
CEREAL_CLASS_VERSION( Item::ItemParam::General,	2 )
CEREAL_CLASS_VERSION( Item::ItemParam::Energy,	0 )
