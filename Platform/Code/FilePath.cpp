#include "FilePath.h"

#include <cassert>
#include <fstream>

#include "Donya/Constant.h"	// Use DEBUG_MODE.
#include "Donya/Useful.h"	// Use IsExistFile().

namespace
{
	static constexpr const char *DIR_PARAMETERS	= "./Data/Parameters/";
	static constexpr const char *DIR_MODELS		= "./Data/Models/";
	static constexpr const char *EXT_MODEL		= ".bin";
}

std::string MakeParameterPathBinary( std::string id )
{
	return DIR_PARAMETERS + id + ".bin";
}
std::string MakeParameterPathJson( std::string id )
{
	return DIR_PARAMETERS + id + ".json";
}
std::string MakeModelPath( std::string id )
{
	return DIR_MODELS + id + EXT_MODEL;
}
std::string MakeStageParamPathBinary( std::string id, int stageNo )
{
	const std::string folder = "Stage" + Donya::MakeArraySuffix( stageNo ) + "/";
	return MakeParameterPathBinary( folder + id );
}
std::string MakeStageParamPathJson( std::string id, int stageNo )
{
	const std::string folder = "Stage" + Donya::MakeArraySuffix( stageNo ) + "/";
	return MakeParameterPathJson( folder + id );
}

bool MakeDirectoryIfNotExists( const std::string &filePath )
{
	const std::string fullPath		= Donya::ToFullPath( filePath );
	const std::string fileDirectory	= Donya::ExtractFileDirectoryFromFullPath( fullPath );
	const int result = Donya::MakeDirectory( fileDirectory );
	return  ( result == 0 ) ? true : false;
}
bool MakeFileIfNotExists( const std::string &filePath, bool binaryMode )
{
	if ( Donya::IsExistFile( filePath ) ) { return false; }
	// else

	const auto openMode = ( binaryMode ) ? std::ios::out | std::ios::binary : std::ios::out;
	std::ofstream ofs{ filePath, openMode };

	// The file is created in here.

	const bool wasCreated = ofs.is_open();

	ofs.close();

	return wasCreated;
}

std::wstring GetSpritePath( SpriteAttribute sprAttribute )
{
	switch ( sprAttribute )
	{
	case SpriteAttribute::FMODLogoBlack:
		return L"./Data/Images/Rights/FMOD Logo Black - White Background.png";	// break;
	case SpriteAttribute::FMODLogoWhite:
		return L"./Data/Images/Rights/FMOD Logo White - Black Background.png";	// break;
	case SpriteAttribute::NowLoading:
		return L"./Data/Images/UI/NowLoading.png";	// break;

	default:
		assert( !"Error : Specified unexpect sprite type." ); break;
	}

	return L"ERROR_ATTRIBUTE";
}
