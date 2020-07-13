#include "Map.h"

#include <algorithm>		// Use remove_if

#include "Donya/Constant.h"	// Use scast macro
#include "Donya/Mouse.h"
#include "Donya/Sprite.h"

#include "Common.h"			// Use IsShowCollision()
#include "FilePath.h"
#include "Parameter.h"		// Use ParameterHelper
#if USE_IMGUI
#include "StageFormat.h"
#endif // USE_IMGUI


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
	constexpr Donya::Vector4 tileColor{ 0.8f, 0.8f, 0.8f, 0.6f };
	Solid::DrawHitBox( pRenderer, matVP, tileColor );
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

Donya::Vector3 Map::ToWorldPos( size_t row, size_t column, bool alignToCenter )
{
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

	return	( alignToCenter )
			? generatePos + halfOffset
			: generatePos;
}
bool Map::Init( int stageNumber )
{
	const bool succeeded = LoadMap( stageNumber, IOFromBinaryFile );

#if DEBUG_MODE
	// If a user was changed only a json file, the user wanna apply the changes to binary file also.
	// So save here.
	SaveMap( stageNumber, /* fromBinary = */ true );

	// Generate safe plane
	if ( tilePtrs.empty() )
	{
		constexpr float				wholeSize = Tile::unitWholeSize;
		constexpr int				tileCount = 5;
		constexpr Donya::Vector3	base   {-wholeSize * ( tileCount >> 1 ), -wholeSize, 0.0f };
		constexpr Donya::Vector3	offsetX{ wholeSize, 0.0f,  0.0f };
		constexpr Donya::Vector3	offsetY{ 0.0f, -wholeSize, 0.0f };
		constexpr Donya::Int2		texOrigin{ 0, 0 };
		
		Donya::Vector3 offset;
		std::vector<ElementType> row;
		for ( int y = 0; y < tileCount; ++y )
		{
			row.clear();
			for ( int x = 0; x < tileCount; ++x )
			{
				offset = ( offsetX * scast<float>( x ) ) + ( offsetY * scast<float>( y ) );
				
				ElementType tmp = std::make_shared<Tile>();
				tmp->Init( base + offset, wholeSize, texOrigin );
				row.emplace_back( std::move( tmp ) );
			}
			tilePtrs.emplace_back( row );
		}

		return true;
	}
#endif // DEBUG_MODE

	return succeeded;
}
void Map::Uninit()
{
	ForEach
	(
		[]( ElementType &pElement )
		{
			if ( pElement ) { pElement->Uninit(); }
		}
	);
}
void Map::Update( float elapsedTime )
{
	ForEach
	(
		[&]( const ElementType &pElement )
		{
			if ( pElement )
			{
				pElement->Update( elapsedTime );

				//if ( pElement->ShouldRemove() )
				//{
				//	pElement->Uninit();
				//}
			}
		}
	);
}
void Map::Draw( RenderingHelper *pRenderer ) const
{
	// TODO: Drawing a stage model
}
void Map::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
{
	ForEach
	(
		[&]( const ElementType &pElement )
		{
			if ( pElement ) { pElement->DrawHitBox( pRenderer, matVP ); }
		}
	);
}
const std::vector<std::vector<Map::ElementType>> &Map::GetTiles() const
{
	return tilePtrs;
}
bool Map::LoadMap( int stageNumber, bool fromBinary )
{
	const std::string filePath	= ( fromBinary )
								? MakeStageParamPathBinary( ID, stageNumber )
								: MakeStageParamPathJson  ( ID, stageNumber );
	return Donya::Serializer::Load( *this, filePath.c_str(), ID, fromBinary );
}
#if USE_IMGUI
void Map::RemakeByCSV( const CSVLoader &loadedData )
{
	auto IsTileID	= []( int id )
	{
		switch ( id )
		{
		case StageFormat::Normal:	return true;
		default: break;
		}

		return false;
	};
	auto Append		= [&]( int id, size_t row, size_t column, std::vector<ElementType> *pOutput )
	{
		if ( !pOutput ) { return; }
		// else

		ElementType tmp = nullptr;

		if ( !IsTileID( id ) )
		{
			pOutput->emplace_back( std::move( tmp ) );
			return;
		}
		// else

		tmp = std::make_shared<Tile>();
		tmp->Init( ToWorldPos( row, column ), Tile::unitWholeSize, 0 );
		pOutput->emplace_back( std::move( tmp ) );
	};

	ForEach( []( ElementType &pElement ) { if ( pElement ) { pElement->Uninit(); } } );
	tilePtrs.clear();

	const auto &data = loadedData.Get();
	const size_t rowCount = data.size();
	tilePtrs.resize( rowCount );

	std::vector<ElementType> row;
	for ( size_t r = 0; r < rowCount; ++r )
	{
		row.clear();
		const size_t columnCount = data[r].size();
		for ( size_t c = 0; c < columnCount; ++c )
		{
			Append( data[r][c], r, c, &row );
		}
		tilePtrs.emplace_back( row );
	}
}
void Map::SaveMap( int stageNumber, bool fromBinary )
{
	const std::string filePath	= ( fromBinary )
								? MakeStageParamPathBinary( ID, stageNumber )
								: MakeStageParamPathJson  ( ID, stageNumber );
	MakeDirectoryIfNotExists( filePath );
	MakeFileIfNotExists( filePath, fromBinary );

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
			std::vector<ElementType> argument;
			ImGui::Helper::ResizeByButton( &tilePtrs, argument );
		}

		auto MakeIndexStr		= []( char elementName, size_t v )
		{
			std::string
				caption = "[" + std::string{ elementName } +":";
			if ( v < 100 ) { caption += "_"; } // Align
			if ( v < 10  ) { caption += "_"; } // Align
			caption += std::to_string( v );
			caption += "]";
			return caption;
		};
		auto MakeCoordinateStr	= []( const Donya::Vector3 &wsPos )
		{
			constexpr size_t	width	= 6U;
			constexpr size_t	decimal	= 2U;
			constexpr char		fill	= '_';
			std::string
			caption =  "[";
			caption += "X:" + Donya::ToString( wsPos.x, width, decimal, fill ) + ", ";
			caption += "Y:" + Donya::ToString( wsPos.y, width, decimal, fill ) + ", ";
			caption += "Z:" + Donya::ToString( wsPos.z, width, decimal, fill );
			caption += "]";
			return caption;
		};

		std::string caption;
		const size_t rowCount = tilePtrs.size();
		for ( size_t y = 0; y < rowCount; ++y )
		{
			auto &row = tilePtrs[y];
			const size_t columnCount = row.size();
			for ( size_t x = 0; x < columnCount; ++x )
			{
				auto &pTile = row[x];

				caption =  MakeIndexStr( 'Y', y );
				caption += MakeIndexStr( 'X', x );

				if ( !pTile )
				{
					caption += ", " + MakeCoordinateStr( ToWorldPos( y, x ) );
					ImGui::TextDisabled( caption.c_str() );
					return;
				}
				// else

				pTile->ShowImGuiNode( caption );
				ImGui::SameLine();

				caption = MakeCoordinateStr( pTile->GetPosition() );
				ImGui::Text( caption.c_str() );
			}
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
		ForEach( []( ElementType &pElement ) { if ( pElement ) { pElement->Uninit(); } } );
		tilePtrs.clear();
		LoadMap( stageNo, true );
	}
	else if ( result == Op::LoadJson )
	{
		ForEach( []( ElementType &pElement ) { if ( pElement ) { pElement->Uninit(); } } );
		tilePtrs.clear();
		LoadMap( stageNo, false );
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI
