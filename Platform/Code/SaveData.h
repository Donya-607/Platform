#pragma once

#include "Donya/Serializer.h"
#include "Donya/Template.h"		// Use Singleton<> template
#include "Donya/UseImGui.h"		// Use USE_IMGUI macro

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
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const char *nodeCaption );
	#endif // USE_IMGUI
	};

	class Admin : public Donya::Singleton<Admin>
	{
		friend Donya::Singleton<Admin>;
	private:
		File nowData;
	public:
		/// <summary>
		/// Returns true if the file was saved successfully
		/// </summary>
		bool Save( unsigned int fileNumber = 0 );
		/// <summary>
		/// Returns true if the file was loaded successfully(i.e. save file is correctly exist)
		/// </summary>
		bool Load( unsigned int fileNumber = 0 );
	public:
		/// <summary>
		/// It returns now "changes" data. It is not the "saved" data!
		/// </summary>
		const File &NowData() const;
		void Clear();
		void Write( Definition::WeaponKind addAvailableWeapon );
		void Write( const Definition::WeaponAvailableStatus &overwriteStatus );
		void Write( const File &overwriteFile );
	private:
		bool SaveBin( unsigned int fileNumber = 0 );
		bool SaveJson( unsigned int fileNumber = 0 );
		bool LoadBin( unsigned int fileNumber = 0 );
		bool LoadJson( unsigned int fileNumber = 0 );
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const char *nodeCaption );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( SaveData::File, 0 )
