#pragma once

#include <string>
#include <vector>

#undef max
#undef min
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/UseImGui.h"	// Use USE_IMGUI macro
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "CSVLoader.h"
#include "ObjectBase.h"


/// <summary>
/// A piece of map(map-chip).
/// </summary>
class Tile : public Solid
{
public:
	static constexpr float unitWholeSize = 1.0f; // Whole size of a standard tile.
private:
	using Solid::body;
	Donya::Int2 texOffset;	// Texture space, Left-Top
#if USE_IMGUI
	bool wantRemove = false;
#endif // USE_IMGUI
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
	bool ShouldRemove() const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Tile, 0 )
// CEREAL_REGISTER_TYPE( Tile )
// CEREAL_REGISTER_POLYMORPHIC_RELATION( Solid, Tile )


/// <summary>
/// A container of the Tiles of per stage.
/// </summary>
class Map
{
public:
	/// <summary>
	/// Convert from 2D row/column(screen space) to 3D XYZ(world space, Z is zero).
	/// "alignToCenterOfTile" adds an half size of tile.
	/// </summary>
	static Donya::Vector3 ToWorldPos( size_t row, size_t column, bool alignToCenterOfTile = true );
private: // shared_ptr<> make be able to copy
	using ElementType = std::shared_ptr<Tile>;
	std::vector<std::vector<ElementType>> tilePtrs; // [Row][Column], [Y][X]. "nullptr" means that placing coordinate is space(empty).
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
	/// <summary>
	/// Returns tiles forming as: [Row][Column], [Y][X]. "nullptr" means that placing coordinate is space(empty).
	/// </summary>
	const std::vector<std::vector<std::shared_ptr<Tile>>> &GetTiles() const;
private:
	/// <summary>
	/// It calls Method() with an argument of "std::shared_ptr&lt;Tile&gt;(ElementType) &amp;".
	/// </summary>
	template<typename ElementMethod>
	void ForEach( ElementMethod Method )
	{
		for ( auto &itr : tilePtrs )
		{
			for ( auto &pIt : itr )
			{
				Method( pIt );
			}
		}
	}
	/// <summary>
	/// It calls Method() with an argument of "std::shared_ptr&lt;Tile&gt;(ElementType) &amp;".
	/// </summary>
	template<typename ElementMethod>
	void ForEach( ElementMethod Method ) const
	{
		for ( const auto &itr : tilePtrs )
		{
			for ( const auto &pIt : itr )
			{
				Method( pIt );
			}
		}
	}
	bool LoadMap( int stageNumber, bool fromBinary );
#if USE_IMGUI
public:
	void RemakeByCSV( const CSVLoader &loadedData );
	void SaveMap( int stageNumber, bool fromBinary );
public:
	void ShowImGuiNode( const std::string &nodeCaption, int stageNo );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Map, 0 )
