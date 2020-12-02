#pragma once

#include "Donya/Serializer.h"
#include "Donya/Template.h"

#include "Weapon.h"

namespace SaveData
{
	struct File
	{
		unsigned int fileNumber = 0;
		Definition::WeaponAvailableStatus availableWeapons;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( availableWeapons ) );
			if ( 1 <= version )
			{
				// archive();
			}
		}
	};

	class Admin : public Donya::Singleton<Admin>
	{
		friend Donya::Singleton<Admin>;
	private:
		File nowData;
	public:
		void Save( unsigned int fileNumber = 0 );
		void Load( unsigned int fileNumber = 0 );
	public:
		/// <summary>
		/// It returns now "changes" data. It is not the "saved" data!
		/// </summary>
		const File &NowData() const;
		void Write( Definition::WeaponKind addAvailableWeapon );
	private:
		void SaveBin( unsigned int fileNumber = 0 );
		void SaveJson( unsigned int fileNumber = 0 );
		void LoadBin( unsigned int fileNumber = 0 );
		void LoadJson( unsigned int fileNumber = 0 );
	};
}
CEREAL_CLASS_VERSION( SaveData::File, 0 )
