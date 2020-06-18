#pragma once

#include <string>
#include <vector>

#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/UseImGui.h"	// Use USE_IMGUI macro
#include "Donya/Vector.h"

#include "ObjectBase.h"


/// <summary>
/// A piece of map(map-chip).
/// </summary>
class Tile : public Solid2D
{
private:
	using Solid2D::pos;
	using Solid2D::posRemainder;
	using Solid2D::hitBox;
	Donya::Int2 tileSize;	// World space, Whole size
	Donya::Int2 texCoord;	// Texture space, Left-Top
	Donya::Int2 texSize;	// Texture space, Whole size
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<Solid2D>( this ),
			CEREAL_NVP( tileSize ),
			CEREAL_NVP( texCoord ),
			CEREAL_NVP( texSize  )
		);
		if ( 1 <= version )
		{
			// archive();
		}
	}
public:
	void Init( const Donya::Int2 &wsTilePos, const Donya::Int2 &wsTileWholeSize, const Donya::Int2 &texPartCoord, const Donya::Int2 &texPartWholeSize );
	void Uninit() {}
	void Draw( size_t spriteIndex ) const;
};
CEREAL_CLASS_VERSION( Tile, 0 )
// CEREAL_REGISTER_TYPE( Tile )
// CEREAL_REGISTER_POLYMORPHIC_RELATION( Solid2D, Tile )


/// <summary>
/// A container of the Tiles.
/// </summary>
class Map
{
private:
	int					stageNo		= 0;
	size_t				spriteIndex	= 0;
	Donya::Int2			texPartSize{ 32, 32 }; // Texture space, Whole size
	std::vector<Tile>	tiles;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( stageNo	),
			CEREAL_NVP( tiles	)
		);
		if ( 1 <= version )
		{
			// archive();
		}
	}
	static constexpr const char *ID = "Map";
public:
	void Uninit();
public:
	/// <summary>
	/// Returns false if a map file of specified stage is not found.
	/// </summary>
	bool LoadMap( int stageNumber );
	/// <summary>
	/// Returns loading result.
	/// </summary>
	bool LoadTileTexture( const std::wstring &tileTexturePath, size_t maxInstanceCount = 2048U );
public:
	/// <summary>
	/// Specify the size of each tile.
	/// </summary>
	void SetTileSize( const Donya::Int2 &tileWholeSize );
public:
	void Draw() const;
	void DrawHitBoxes( const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
private:
#if USE_IMGUI
	void Save( int stageNo );
public:
	// void ShowImGuiNode( const std::string &nodeCaption, int stageNo );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Map, 0 )
