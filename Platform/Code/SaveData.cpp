#include "SaveData.h"

#include "FilePath.h"

namespace SaveData
{
	void Admin::Save( unsigned int fileNo )
	{
		SaveBin( fileNo );
	#if DEBUG_MODE
		SaveJson( fileNo );
	#endif // DEBUG_MODE
	}
	void Admin::Load( unsigned int fileNo )
	{
	#if DEBUG_MODE
		LoadJson( fileNo );
	#else
		LoadBin( fileNo );
	#endif // DEBUG_MODE

		nowData.fileNumber = fileNo;
	}
	const File &Admin::NowData() const
	{
		return nowData;
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
			MakeFileIfNotExists( path, useBinary );
			return path;
		}
	}
	void Admin::SaveBin( unsigned int fileNo )
	{
		constexpr bool fromBinary = true;

		const std::string fileName = MakeFileName( fileNo );
		const std::string filePath = MakeFilePathImpl( fileName, fromBinary );

		Donya::Serializer tmp;
		tmp.SaveBinary( nowData, filePath.c_str(), fileName.c_str() );
	}
	void Admin::SaveJson( unsigned int fileNo )
	{
		constexpr bool fromBinary = false;

		const std::string fileName = MakeFileName( fileNo );
		const std::string filePath = MakeFilePathImpl( fileName, fromBinary );

		Donya::Serializer tmp;
		tmp.SaveJSON( nowData, filePath.c_str(), fileName.c_str() );
	}
	void Admin::LoadBin( unsigned int fileNo )
	{
		constexpr bool fromBinary = true;

		const std::string fileName = MakeFileName( fileNo );
		const std::string filePath = MakeFilePathImpl( fileName, fromBinary );

		Donya::Serializer tmp;
		tmp.LoadBinary( nowData, filePath.c_str(), fileName.c_str() );
	}
	void Admin::LoadJson( unsigned int fileNo )
	{
		constexpr bool fromBinary = false;

		const std::string fileName = MakeFileName( fileNo );
		const std::string filePath = MakeFilePathImpl( fileName, fromBinary );

		Donya::Serializer tmp;
		tmp.LoadJSON( nowData, filePath.c_str(), fileName.c_str() );
	}
}
