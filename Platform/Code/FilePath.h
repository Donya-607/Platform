#pragma once

#include <string>

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
};

std::wstring GetSpritePath( SpriteAttribute spriteAttribute );