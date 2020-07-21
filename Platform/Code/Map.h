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
#include "StageFormat.h"


/// <summary>
/// A piece of map(map-chip).
/// </summary>
class Tile : public Solid
{
public:
	static constexpr float unitWholeSize = 1.0f; // Whole size of a standard tile.
private:
	using Solid::body;
	StageFormat::ID tileID = StageFormat::ID::Space;
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
			CEREAL_NVP( tileID )
		);
		if ( 1 <= version )
		{
			// archive();
		}
	}
public:
	void Init( StageFormat::ID tileID, const Donya::Vector3 &wsTilePos, const Donya::Vector3 &wsTileWholeSize );
	void Uninit();
	void Update( float elapsedTime );
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
public:
	bool ShouldRemove() const;
	StageFormat::ID GetID() const;
private:
	Donya::Vector4 GetDrawColor() const;
public:
#if USE_IMGUI
	/// <summary>
	/// Returns the return value of ImGui::TreeNode().
	/// </summary>
	bool ShowImGuiNode( const std::string &nodeCaption );
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
	/// Convert from 2D row/column(tile/screen space) to 3D XYZ(world space, Z is zero).
	/// "alignToCenterOfTile" adds half size of tile.
	/// </summary>
	static Donya::Vector3 ToWorldPos( size_t row, size_t column, bool alignToCenterOfTile = true );
	/// <summary>
	/// Convert from 3D XYZ(world space, Z is zero) to 2D row/column(tile/screen space).
	/// "alignToLeftTopOfTile" subtracts half size of tile. It makes a compatible to ToWorldPos().
	/// </summary>
	static Donya::Vector2 ToTilePosF( const Donya::Vector3 &wsPos, bool alignToLeftTopOfTile = false );
	/// <summary>
	/// Convert from 3D XYZ(world space, Z is zero) to 2D row/column(tile/screen space).
	/// "alignToLeftTopOfTile" subtracts half size of tile. It makes a compatible to ToWorldPos().
	/// </summary>
	static Donya::Int2 ToTilePos( const Donya::Vector3 &wsPos, bool alignToLeftTopOfTile = false );
	/// <summary>
	/// Convert from "const Tile &amp;" vector to "Collision::Box3F" vector. An empty tiles will be removed(if "removeEmpties" is true). Or to be Nil(if "removeEmpties" is false).
	/// </summary>
	static std::vector<Donya::Collision::Box3F> ToAABB( const std::vector<std::shared_ptr<const Tile>> &constTilePtrs, bool removeEmpties = true );
private: // shared_ptr<> make be able to copy
	using ElementType = std::shared_ptr<Tile>;
	std::vector<std::vector<ElementType>> tilePtrs; // [Row][Column], [Y][X]. "nullptr" means that placing coordinate is space(empty).
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive( CEREAL_NVP( tilePtrs ) );
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
	/// <summary>
	/// Returns a tile that there on argument position. Or nullptr if that position is empty.
	/// </summary>
	std::shared_ptr<const Tile> GetPlaceTileOrNullptr( const Donya::Vector3 &wsPos ) const;
	/// <summary>
	/// Call GetPlaceTileOrNullptr() as many argument count as.
	/// </summary>
	std::vector<std::shared_ptr<const Tile>> GetPlaceTiles( const std::vector<Donya::Vector3> &wsPositions ) const;
	/// <summary>
	/// Call GetPlaceTileOrNullptr() as filling the argument area as.
	/// [Option] "wsSearchersVelocity" can be extend the search area.
	/// </summary>
	std::vector<std::shared_ptr<const Tile>> GetPlaceTiles( const Donya::Collision::Box3F &wsSearchArea, const Donya::Vector3 &wsSearchersVelocity = { 0.0f, 0.0f, 0.0f } ) const;
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
