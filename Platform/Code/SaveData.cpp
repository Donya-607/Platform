#include "SaveData.h"

#include "Donya/Useful.h"

#include "FilePath.h"
#include "Parameter.h"

namespace SaveData
{
#if USE_IMGUI
	void File::ShowImGuiNode( const char *nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption ) ) { return; }
		// else

		int intNo = scast<int>( fileNumber );
		ImGui::InputInt( u8"ファイル番号", &intNo );
		intNo = std::max( 0, intNo );
		fileNumber = scast<unsigned int>( intNo );

		availableWeapons.ShowImGuiNode( u8"解放済の武器" );

		ImGui::TreePop();
	}
#endif // USE_IMGUI


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
	void Admin::Write( const Definition::WeaponAvailableStatus &status )
	{
		nowData.availableWeapons = status;
	}
	void Admin::Write( const File &file )
	{
		nowData = file;
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
#if USE_IMGUI
	void Admin::ShowImGuiNode( const char *nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption ) ) { return; }
		// else

		nowData.ShowImGuiNode( u8"現在のファイル状態" );

		const auto result = ParameterHelper::ShowIONode();
		using Op = ParameterHelper::IOOperation;
		if ( result == Op::Save )
		{
			SaveBin();
			SaveJson();
		}
		else if ( result == Op::LoadBinary )
		{
			LoadBin();
		}
		else if ( result == Op::LoadJson )
		{
			LoadJson();
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI
}
