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


namespace
{
	struct SpriteSet
	{
		const std::wstring	path			= L"ERROR_PATH";
		const size_t		instanceCount	= 32U;
	public:
		SpriteSet( const std::wstring &filePath, size_t instanceCount )
			: path( filePath ), instanceCount( instanceCount )
		{}
		SpriteSet( const SpriteSet &copy )
			: path( copy.path ), instanceCount( copy.instanceCount )
		{}
	};

	SpriteSet Make( const wchar_t *sprName, size_t instanceCount )
	{
		constexpr const wchar_t *directory = L"./Data/Images/";
		return SpriteSet{ std::wstring{ directory + std::wstring{ sprName } }, instanceCount };
	}
	SpriteSet GetSpriteInfo( SpriteAttribute attr )
	{
		switch ( attr )
		{
		case SpriteAttribute::FMODLogoBlack:
			return Make( L"Rights/FMOD Logo Black - White Background.png", 2U );
		case SpriteAttribute::FMODLogoWhite:
			return Make( L"Rights/FMOD Logo White - Black Background.png", 2U );

		case SpriteAttribute::NowLoading:
			return Make( L"UI/NowLoading.png",	1U );
		case SpriteAttribute::TitleLogo:
			return Make( L"Title/Logo.png",		2U );

		default: break;
		}

		_ASSERT_EXPR( 0, L"Error : Specified unexpect sprite type!" );
		return SpriteSet{ L"ERROR", 0U };
	}
}
std::wstring	GetSpritePath( SpriteAttribute attr )
{
	return GetSpriteInfo( attr ).path;
}
size_t			GetSpriteInstanceCount( SpriteAttribute attr )
{
	return GetSpriteInfo( attr ).instanceCount;
}
