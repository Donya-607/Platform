#include "Map.h"

#include <algorithm>		// Use remove_if

#include "Donya/Constant.h"	// Use scast macro
#include "Donya/Mouse.h"
#include "Donya/Sprite.h"

#include "Common.h"			// Use IsShowCollision()
#include "CSVLoader.h"
#include "FilePath.h"
#include "Parameter.h"		// Use ParameterHelper
#include "StageFormat.h"


void Tile::Init( const Donya::Vector3 &wsTilePos, const Donya::Vector3 &wsTileWholeSize, const Donya::Int2 &texCoordOffset )
{
	body.pos	= wsTilePos;
	body.offset	= Donya::Vector3::Zero();
	body.size	= wsTileWholeSize * 0.5f;
	body.exist	= true;
	texOffset	= texCoordOffset;
}
void Tile::Uninit() {}
void Tile::Update( float elapsedTime ) {}
void Tile::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else

#if DEBUG_MODE
	Donya::Model::Cube::Constant constant;
	auto &W = constant.matWorld;
	const auto wsPos = body.WorldPosition();
	W._11 = body.size.x * 2.0f;
	W._22 = body.size.y * 2.0f;
	W._33 = body.size.z * 2.0f;
	W._41 = wsPos.x;
	W._42 = wsPos.y;
	W._43 = wsPos.z;
	constant.matViewProj	= matVP;
	constant.drawColor		= { 0.8f, 0.8f, 0.8f, 0.6f };
	constant.lightDirection	= -Donya::Vector3::Up();

	pRenderer->ProcessDrawingCube( constant );
#endif // DEBUG_MODE
}
bool Tile::ShouldRemove() const
{
#if USE_IMGUI
	if ( wantRemove ) { return true; }
	// else
#endif // USE_IMGUI
	return false;
}
#if USE_IMGUI
void Tile::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	wantRemove = ImGui::Button( ( nodeCaption + u8"を削除" ).c_str() );

	ImGui::Helper::ShowAABBNode( u8"体",			&body );
	ImGui::DragInt2( u8"テクスチャオフセット",	&texOffset.x );

	ImGui::TreePop();
}
#endif // USE_IMGUI


namespace
{
#if DEBUG_MODE
	constexpr bool IOFromBinaryFile = false;
#else
	constexpr bool IOFromBinaryFile = true;
#endif // DEBUG_MODE
}

bool Map::Init( int stageNumber )
{
	const bool succeeded = LoadMap( stageNumber, IOFromBinaryFile );

#if DEBUG_MODE
	// Generate safe plane
	if ( tiles.empty() )
	{
		constexpr float				wholeSize = Tile::unitWholeSize;
		constexpr int				tileCount = 5;
		constexpr Donya::Vector3	base  {-wholeSize * tileCount, -wholeSize, 0.0f };
		constexpr Donya::Vector3	offset{ wholeSize, 0.0f, 0.0f };
		constexpr Donya::Int2		texOrigin{ 0, 0 };
		
		for ( int i = 0; i < tileCount; ++i )
		{
			Tile tmp{};
			tmp.Init( base + ( offset * scast<float>( i ) ), wholeSize, texOrigin );
			tiles.emplace_back( std::move( tmp ) );
		}

		return true;
	}
#endif // DEBUG_MODE

	return succeeded;
}
void Map::Uninit()
{
	for ( auto &it : tiles )
	{
		it.Uninit();
	}
}
void Map::Update( float elapsedTime )
{
	for ( auto &it : tiles )
	{
		it.Update( elapsedTime );

		if ( it.ShouldRemove() )
		{
			it.Uninit();
		}
	}

	RemoveTiles();
}
void Map::Draw( RenderingHelper *pRenderer ) const
{
	// TODO: Drawing a stage model
}
void Map::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
{
	for ( const auto &it : tiles )
	{
		it.DrawHitBox( pRenderer, matVP );
	}
}
const std::vector<Tile> &Map::GetTiles() const
{
	return tiles;
}
void Map::RemakeByCSV( const CSVLoader &loadedData )
{
	auto IsIgnoreID	= []( int id )
	{
		switch ( id )
		{
		case StageFormat::StartPoint:	return true;
		case StageFormat::Space:		return true;
		case StageFormat::EmptyValue:	return true;
		default: break;
		}

		return false;
	};
	auto Append		= [&]( int id, size_t row, size_t column )
	{
		if ( IsIgnoreID( id ) ) { return; }
		// else

		// I expect the CSV stage data is screen space, so the Y component must be reverse.(stage's Y+ is down, application's Y+ is up)

		constexpr Donya::Vector3 halfOffset
		{
			Tile::unitWholeSize * 0.5f,
			Tile::unitWholeSize * 0.5f * -1.0f,
			0.0f,
		};
		const Donya::Vector3 generatePos
		{
			Tile::unitWholeSize * column,
			Tile::unitWholeSize * row * -1.0f,
			0.0f,
		};

		Tile tmp;
		tmp.Init( generatePos + halfOffset, Tile::unitWholeSize, 0 );
		tiles.emplace_back( std::move( tmp ) );
	};

	for ( auto &it : tiles ) { it.Uninit(); }
	tiles.clear();

	const auto &data = loadedData.Get();
	const size_t rowCount = data.size();
	for ( size_t r = 0; r < rowCount; ++r )
	{
		const size_t columnCount = data[r].size();
		for ( size_t c = 0; c < columnCount; ++c )
		{
			Append( data[r][c], r, c );
		}
	}
}
void Map::RemoveTiles()
{
	auto itr = std::remove_if
	(
		tiles.begin(), tiles.end(),
		[]( Tile &element )
		{
			return element.ShouldRemove();
		}
	);
	tiles.erase( itr, tiles.end() );
}
bool Map::LoadMap( int stageNumber, bool fromBinary )
{
	const std::string filePath	= ( fromBinary )
								? MakeStageParamPathBinary( ID, stageNumber )
								: MakeStageParamPathJson  ( ID, stageNumber );
	return Donya::Serializer::Load( *this, filePath.c_str(), ID, fromBinary );
}
#if USE_IMGUI
void Map::SaveMap( int stageNumber, bool fromBinary )
{
	const std::string filePath	= ( fromBinary )
								? MakeStageParamPathBinary( ID, stageNumber )
								: MakeStageParamPathJson  ( ID, stageNumber );
	MakeDirectoryIfNotExists( filePath );
	MakeFileIfNotExists( filePath, IOFromBinaryFile );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void Map::ShowImGuiNode( const std::string &nodeCaption, int stageNo )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	if ( ImGui::TreeNode( u8"実体操作" ) )
	{
		// Resize
		{
		constexpr float				wholeSize = Tile::unitWholeSize;
		constexpr Donya::Vector3	wsPos = Donya::Vector3::Zero();
		constexpr Donya::Int2		texOrigin{ 0, 0 };
		Tile ctorArg;
		ctorArg.Init( wsPos, wholeSize, texOrigin );
		ImGui::Helper::ResizeByButton( &tiles, ctorArg );
	}

		std::string caption;
		const size_t count = tiles.size();
		for ( size_t i = 0; i < count; ++i )
		{
			caption =  "[";
			if ( i < 10 ) { caption += "0"; }
			caption += std::to_string( i );
			caption += "]";
			tiles[i].ShowImGuiNode( caption );
			ImGui::SameLine();

			constexpr size_t width		= 6U;
			constexpr size_t decimal	= 2U;
			constexpr char   fill		= '_';
			const Donya::Vector3 itPos	= tiles[i].GetPosition();
			caption =  "[";
			caption += "X:" + Donya::ToString( itPos.x, width, decimal, fill ) + ", ";
			caption += "Y:" + Donya::ToString( itPos.y, width, decimal, fill ) + ", ";
			caption += "Z:" + Donya::ToString( itPos.z, width, decimal, fill );
			caption += "]";
			ImGui::Text( caption.c_str() );
		}

		ImGui::TreePop();
	}

	const auto result = ParameterHelper::ShowIONode();
	using Op = ParameterHelper::IOOperation;
	if ( result == Op::Save )
	{
		SaveMap( stageNo, true  );
		SaveMap( stageNo, false );
	}
	else if ( result == Op::LoadBinary )
	{
		LoadMap( stageNo, true );
	}
	else if ( result == Op::LoadJson )
	{
		LoadMap( stageNo, false );
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI
