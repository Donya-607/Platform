#include "SaveData.h"

#include "Donya/Useful.h"

#include "FilePath.h"

namespace SaveData
{
	bool Admin::Save( unsigned int fileNo )
	{
		bool fileIsExist = SaveBin( fileNo );
	#if DEBUG_MODE
		     fileIsExist = SaveJson( fileNo );
	#endif // DEBUG_MODE
		return fileIsExist;
	}
	bool Admin::Load( unsigned int fileNo )
	{
	#if DEBUG_MODE
		const bool fileIsExist = LoadJson( fileNo );
	#else
		const bool fileIsExist = LoadBin( fileNo );
	#endif // DEBUG_MODE

		nowData.fileNumber = fileNo;

		return fileIsExist;
	}
	const File &Admin::NowData() const
	{
		return nowData;
	}
	void Admin::Clear()
	{
		nowData.availableWeapons.Reset();
	}
	void Admin::Write( Definition::WeaponKind add )
	{
		nowData.availableWeapons.Activate( add );
	}
	namespace
	{
		std::string MakeFileName( unsigned int fileNo )
		{
			return "Save" + Donya::MakeArraySuffix( fileNo );
		}
		std::string MakeFilePathImpl( const std::string &fileName, bool useBinary )
		{
			const std::string path =
				( useBinary )
				? MakeSaveDataPathBin ( fileName )
				: MakeSaveDataPathJson( fileName );

			MakeDirectoryIfNotExists( path );
			
			return path;
		}
	}
	bool Admin::SaveBin( unsigned int fileNo )
	{
		constexpr bool fromBinary = true;

		const std::string fileName = MakeFileName( fileNo );
		const std::string filePath = MakeFilePathImpl( fileName, fromBinary );

		Donya::Serializer tmp;
		return tmp.SaveBinary( nowData, filePath.c_str(), fileName.c_str() );
	}
	bool Admin::SaveJson( unsigned int fileNo )
	{
		constexpr bool fromBinary = false;

		const std::string fileName = MakeFileName( fileNo );
		const std::string filePath = MakeFilePathImpl( fileName, fromBinary );

		Donya::Serializer tmp;
		return tmp.SaveJSON( nowData, filePath.c_str(), fileName.c_str() );
	}
	bool Admin::LoadBin( unsigned int fileNo )
	{
		constexpr bool fromBinary = true;

		const std::string fileName = MakeFileName( fileNo );
		const std::string filePath = MakeFilePathImpl( fileName, fromBinary );

		Donya::Serializer tmp;
		return tmp.LoadBinary( nowData, filePath.c_str(), fileName.c_str() );
	}
	bool Admin::LoadJson( unsigned int fileNo )
	{
		constexpr bool fromBinary = false;

		const std::string fileName = MakeFileName( fileNo );
		const std::string filePath = MakeFilePathImpl( fileName, fromBinary );

		Donya::Serializer tmp;
		return tmp.LoadJSON( nowData, filePath.c_str(), fileName.c_str() );
	}
}
