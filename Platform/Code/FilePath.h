#pragma once

#include <string>

enum class FontAttribute
{
	Meiryo
};
std::string MakeFontPathBinary( FontAttribute fontAttribute );
std::wstring MakeFontPathFnt( FontAttribute fontAttribute );
std::string MakeParameterPathBinary( std::string parameterName );
std::string MakeParameterPathJson( std::string parameterName );
std::string MakeModelPath( std::string modelName );
std::string MakeStageParamPathBinary( std::string objectName, int stageNumber );
std::string MakeStageParamPathJson( std::string objectName, int stageNumber );

/// <summary>
/// Returns false if the directory was not created.
/// The directory path was not founded(may contain a not exists directory?), or already exists.
/// </summary>
bool MakeDirectoryIfNotExists( const std::string &relativeFilePath );
/// <summary>
/// Creates empty file.<para></para>
/// Returns false if the file was created.
/// The file path was not founded(may contain a not exists directory?), or already exists.
/// </summary>
bool MakeFileIfNotExists( const std::string &relativeFilePath, bool binaryMode );

enum class SpriteAttribute
{
	FMODLogoBlack,
	FMODLogoWhite,

	NowLoading,
	TitleLogo,
	
	Meter,
};
std::wstring	GetSpritePath( SpriteAttribute spriteAttribute );
size_t			GetSpriteInstanceCount( SpriteAttribute spriteAttribute );
