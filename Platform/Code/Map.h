#pragma once

#include <string>
#include <vector>

#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/UseImGui.h"	// Use USE_IMGUI macro
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "ObjectBase.h"


/// <summary>
/// A piece of map(map-chip).
/// </summary>
class Tile : public Solid
{
private:
	using Solid::pos;
	using Solid::hitBox;
	Donya::Int2 texOffset;	// Texture space, Left-Top
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<Solid>( this ),
			CEREAL_NVP( texOffset )
		);
		if ( 1 <= version )
		{
			// archive();
		}
	}
public:
	void Init( const Donya::Vector3 &wsTilePos, const Donya::Vector3 &wsTileWholeSize, const Donya::Int2 &texCoordOffset );
	void Uninit();
	void Update( float elapsedTime );
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Tile, 0 )
// CEREAL_REGISTER_TYPE( Tile )
// CEREAL_REGISTER_POLYMORPHIC_RELATION( Solid, Tile )


/// <summary>
/// A container of the Tiles.
/// </summary>
class Map
{
private:
	std::vector<Tile> tiles;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive( CEREAL_NVP( tiles ) );
		if ( 1 <= version )
		{
			// archive();
		}
	}
	static constexpr const char *ID = "Map";
public:
	bool Init( int stageNumber );
	void Uninit();
	void Update( float elapsedTime );
	void Draw( RenderingHelper *pRenderer ) const;
	void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
public:
	const std::vector<Tile> &GetTiles() const;
private:
	bool LoadMap( int stageNumber, bool fromBinary );
#if USE_IMGUI
	void SaveMap( int stageNumber, bool fromBinary );
public:
	void ShowImGuiNode( const std::string &nodeCaption, int stageNo );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Map, 0 )
