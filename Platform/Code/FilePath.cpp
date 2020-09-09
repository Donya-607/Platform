#include "FilePath.h"

#include <cassert>
#include <fstream>

#include "Donya/Constant.h"	// Use DEBUG_MODE.
#include "Donya/Useful.h"	// Use IsExistFile().

namespace
{
	static constexpr const char		*DIR_PARAMETERS	= "./Data/Parameters/";
	static constexpr const char		*DIR_FONTS		=  "./Data/Images/Fonts/";
	static constexpr const wchar_t	*DIR_FONTS_W	= L"./Data/Images/Fonts/";
	static constexpr const char		*DIR_MODELS		= "./Data/Models/";
	static constexpr const char		*EXT_BINARY		= ".bin";
	static constexpr const wchar_t	*EXT_FONT		= L".fnt";
	static constexpr const char		*EXT_JSON		= ".json";
}

namespace
{
	std::string  GetFontName( FontAttribute attr )
	{
		switch ( attr )
		{
		case FontAttribute::Main: return "Main";
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected attribute!" );
		return GetFontName( FontAttribute::Main ); // Fail safe
	}
	std::wstring GetFontNameW( FontAttribute attr )
	{
		return Donya::UTF8ToWide( GetFontName( attr ) );
	}
}
std::string MakeFontPathBinary( FontAttribute attr )
{
	return DIR_FONTS + GetFontName( attr ) + EXT_BINARY;
}
std::wstring MakeFontPathFnt( FontAttribute attr )
{
	return DIR_FONTS_W + GetFontNameW( attr ) + EXT_FONT;
}
std::string MakeParameterPathBinary( std::string id )
{
	return DIR_PARAMETERS + id + EXT_BINARY;
}
std::string MakeParameterPathJson( std::string id )
{
	return DIR_PARAMETERS + id + EXT_JSON;
}
std::string MakeModelPath( std::string id )
{
	return DIR_MODELS + id + EXT_BINARY;
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
		
		case SpriteAttribute::Meter:
			return Make( L"UI/Meter.png",		16U );

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
