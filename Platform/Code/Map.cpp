#include "Map.h"

#include "Donya/Sprite.h"
#include "Donya/Constant.h"	// Use scast macro

#include "FilePath.h"

void Tile::Init( const Donya::Int2 &wsTilePos, const Donya::Int2 &wsTileSize, const Donya::Int2 &texPartCoord, const Donya::Int2 &texPartSize )
{
	pos				= wsTilePos;
	posRemainder	= Donya::Vector2::Zero();
	hitBox.pos		= Donya::Int2::Zero();
	hitBox.size.x	= wsTileSize.x >> 1;
	hitBox.size.y	= wsTileSize.y >> 1;
	hitBox.exist	= true;
	tileSize		= wsTileSize;
	texCoord		= texPartCoord;
	texSize			= texPartSize;
}
void Tile::Draw( size_t sprIndex ) const
{
	Donya::Vector2 scale{ 1.0f, 1.0f };
	if ( tileSize != texSize )
	{
		scale.x = ( !texSize.x ) ? 0.0f : scast<float>( tileSize.x ) / scast<float>( texSize.x );
		scale.y = ( !texSize.y ) ? 0.0f : scast<float>( tileSize.y ) / scast<float>( texSize.y );
	}

	const Donya::Vector2 wsPosF		= GetPositionFloat();
	const Donya::Vector2 texCoordF	= texCoord.Float();
	const Donya::Vector2 texSizeF	= texSize.Float();
	Donya::Sprite::DrawPartExt
	(
		sprIndex,
		wsPosF.x,		wsPosF.y,
		texCoordF.x,	texCoordF.y,
		texSizeF.x,		texSizeF.y,
		scale.x,		scale.y
	);
}


namespace
{
#if DEBUG_MODE
	constexpr bool IOFromBinaryFile = false;
#else
	constexpr bool IOFromBinaryFile = true;
#endif // DEBUG_MODE
}


void Map::Uninit()
{
	for ( auto &it : tiles )
	{
		it.Uninit();
	}
}
bool Map::LoadMap( int stageNumber )
{
	const std::string filePath = MakeMapPath( ID, stageNumber );
	if ( !Donya::IsExistFile( filePath ) ) { return false; }
	// else

	return Donya::Serializer::Load( *this, filePath.c_str(), ID, IOFromBinaryFile );
}
bool Map::LoadTileTexture( const std::wstring &texturePath, size_t maxInstanceCount )
{
	spriteIndex = Donya::Sprite::Load( texturePath, maxInstanceCount );
	return ( spriteIndex != NULL ) ? true : false;
}
void Map::SetTileSize( const Donya::Int2 &tileSize )
{
	texPartSize = tileSize;
}
void Map::Draw() const
{
	for ( const auto &it : tiles )
	{
		it.Draw( spriteIndex );
	}
}
void Map::DrawHitBoxes( const Donya::Vector4 &color ) const
{
	for ( const auto &it : tiles )
	{
		it.DrawHitBox( color );
	}
}
#if USE_IMGUI
void Map::Save( int stageNumber )
{
	const std::string filePath = MakeMapPath( ID, stageNumber );
	MakeFileIfNotExists( filePath, IOFromBinaryFile );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, IOFromBinaryFile );
}
#endif // USE_IMGUI
