#include "Map.h"

#include <algorithm>		// Use remove_if

#include "Donya/Constant.h"	// Use scast macro
#include "Donya/Mouse.h"
#include "Donya/Sprite.h"

#include "Common.h"			// Use LargestDeltaTime(), IsShowCollision()
#include "FilePath.h"
#include "Parameter.h"		// Use ParameterHelper
#if USE_IMGUI
#include "StageFormat.h"
#endif // USE_IMGUI


void Tile::Init( StageFormat::ID identifier, const Donya::Vector3 &wsTilePos, const Donya::Vector3 &wsTileWholeSize )
{
	body.pos	= wsTilePos;
	body.offset	= Donya::Vector3::Zero();
	body.size	= wsTileWholeSize * 0.5f;
	body.exist	= true;
	tileID		= identifier;
}
void Tile::Uninit() {}
void Tile::Update( float elapsedTime ) {}
void Tile::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else

#if DEBUG_MODE
	Solid::DrawHitBox( pRenderer, matVP, GetDrawColor() );
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
StageFormat::ID Tile::GetID() const
{
	return tileID;
}
Donya::Vector4 Tile::GetDrawColor() const
{
	constexpr Donya::Vector4 normalColor{ 0.8f, 0.8f, 0.8f, 0.6f };
	constexpr Donya::Vector4 ladderColor{ 0.8f, 0.8f, 0.0f, 0.6f };
	constexpr Donya::Vector4 needleColor{ 0.8f, 0.6f, 0.6f, 0.6f };
	constexpr Donya::Vector4 emptyColor { 0.0f, 0.0f, 0.0f, 0.0f };

	switch ( tileID )
	{
	case StageFormat::Normal: return normalColor;
	case StageFormat::Ladder: return ladderColor;
	case StageFormat::Needle: return needleColor;
	default: break;
	}

	return emptyColor;
}
#if USE_IMGUI
bool Tile::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
	// else

	wantRemove = ImGui::Button( ( nodeCaption + u8"を削除" ).c_str() );

	ImGui::Helper::ShowAABBNode( u8"体",			&body );
	const bool treeIsOpen = StageFormat::ShowImGuiNode( u8"種類", &tileID );
	if ( !treeIsOpen )
	{
		// Show as: "種類：[%s]"
		ImGui::SameLine();
		ImGui::Text( "：[%s]", StageFormat::MakeIDName( tileID ).c_str() );
	}

	ImGui::TreePop();
	return true;
}
#endif // USE_IMGUI


namespace
{
#if DEBUG_MODE
	constexpr bool IOFromBinaryFile = true;
#else
	constexpr bool IOFromBinaryFile = true;
#endif // DEBUG_MODE

	constexpr Donya::Vector3 halfTileSize
	{
		Tile::unitWholeSize * 0.5f,
		Tile::unitWholeSize * 0.5f * -1.0f,
		0.0f,
	};
}

Donya::Vector3 Map::ToWorldPos( size_t row, size_t column, bool alignToCenter )
{
	// I expect the CSV stage data is screen space, so the Y component must be reverse.(stage's Y+ is down, application's Y+ is up)

	const Donya::Vector3 generatePos
	{
		Tile::unitWholeSize * column,
		Tile::unitWholeSize * row * -1.0f,
		0.0f,
	};

	return	( alignToCenter )
			? generatePos + halfTileSize
			: generatePos;
}
Donya::Vector2 Map::ToTilePosF( const Donya::Vector3 &wsPos, bool alignToLeftTop )
{
	constexpr Donya::Vector2 ssHalfTileSize2
	{
		halfTileSize.x,
		halfTileSize.y * 1.0f,
	};
	const Donya::Vector2 screenPos
	{
		wsPos.x / Tile::unitWholeSize,
		wsPos.y / Tile::unitWholeSize * -1.0f,
	};
	return	( alignToLeftTop )
			? screenPos - ssHalfTileSize2
			: screenPos;
}
Donya::Int2 Map::ToTilePos( const Donya::Vector3 &wsPos, bool alignToLeftTop )
{
	const auto ssPosF = ToTilePosF( wsPos );
	return Donya::Int2
	{	// Discard under the decimal point
		scast<int>( ssPosF.x ),
		scast<int>( ssPosF.y )
	};
}
std::vector<Donya::Collision::Box3F> Map::ToAABBSolids( const std::vector<std::shared_ptr<const Tile>> &tilePtrs, const Map &terrain, const Donya::Collision::Box3F &otherBody, bool removeEmpties )
{
	const float otherFoot = otherBody.Min().y;
	auto CanRideOnLadder = [&]( const std::shared_ptr<const Tile> &pLadder )
	{
		if ( !pLadder ) { return false; }
		// else
		
		// TODO: Also consider the object's horizontal area is inside in ladder area

		const float ladderTop = pLadder->GetHitBox().Max().y;
		if ( otherFoot < ladderTop ) { return false; }
		// else

		constexpr Donya::Vector3 oneAboveOffset{ 0.0f, Tile::unitWholeSize, 0.0f };
		const auto pOneAboveTile = terrain.GetPlaceTileOrNullptr( pLadder->GetPosition() + oneAboveOffset );
		if ( pOneAboveTile && pOneAboveTile->GetID() == StageFormat::Ladder ) { return false; }
		// else

		return true;
	};

	std::vector<Donya::Collision::Box3F> results;
	auto Skip = [&]()
	{
		if ( !removeEmpties )
		{
			// Fill by Nil() for align the index
			results.emplace_back( Donya::Collision::Box3F::Nil() );
		}
	};

	for ( const auto &pIt : tilePtrs )
	{
		if ( pIt )
		{
			if ( pIt->GetID() == StageFormat::Needle )
			{
				Skip();
				continue;
			}
			if ( pIt->GetID() == StageFormat::Ladder && !CanRideOnLadder( pIt ) )
			{
				Skip();
				continue;
			}
			// else

			results.emplace_back( pIt->GetHitBox() );
		}
		else
		{
			Skip();
		}
	}

	return results;
}
std::vector<Donya::Collision::Box3F> Map::ToAABBKillAreas( const std::vector<std::shared_ptr<const Tile>> &tilePtrs, const Map &terrain, const Donya::Collision::Box3F &unused, bool removeEmpties )
{
	std::vector<Donya::Collision::Box3F> results;
	auto Skip = [&]()
	{
		if ( !removeEmpties )
		{
			// Fill by Nil() for align the index
			results.emplace_back( Donya::Collision::Box3F::Nil() );
		}
	};

	for ( const auto &pIt : tilePtrs )
	{
		if ( pIt )
		{
			if ( pIt->GetID() != StageFormat::Needle )
			{
				Skip();
				continue;
			}
			// else

			results.emplace_back( pIt->GetHitBox() );
		}
		else
		{
			Skip();
		}
	}

	return results;
}
namespace
{
	constexpr const char *modelPrefix	= "Map/Stage";
	constexpr const char *modelName		= "World";
	bool LoadStageModel( std::unique_ptr<ModelHelper::StaticSet> *pTarget, int stageNumber )
	{
		if ( !pTarget ) { return false; }
		// else

		auto &ptr = *pTarget;
		ptr.reset();

		const std::string folderName	= modelPrefix + Donya::MakeArraySuffix( stageNumber ) + "/";
		const std::string filePath		= MakeModelPath( folderName + modelName );
		if ( !Donya::IsExistFile( filePath ) )
		{
			const std::string msg = "Error: Model of Stage[" + std::to_string( stageNumber ) + "] is not found.\n";
			Donya::OutputDebugStr( msg.c_str() );
			return false;
		}
		// else

		ptr = std::make_unique<ModelHelper::StaticSet>();
		const bool result = ModelHelper::Load( filePath, ptr.get() );;
		if ( !result )
		{
			ptr.reset(); // Make not loaded state

			const std::string msg = "Failed: Loading Map model: " + filePath;
			Donya::OutputDebugStr( msg.c_str() );
			return false;
		}
		// else

		return true;
	}
}
bool Map::Init( int stageNumber )
{
	bool succeeded = true, result = true;

	result = LoadMap( stageNumber, IOFromBinaryFile );
	if ( !result ) { succeeded = false; }

	result = LoadStageModel( &pModel, stageNumber );
	if ( !result ) { succeeded = false; }

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
		
		Donya::Vector3 offset;
		std::vector<ElementType> row;
		for ( int y = 0; y < tileCount; ++y )
		{
			row.clear();
			for ( int x = 0; x < tileCount; ++x )
			{
				offset = ( offsetX * scast<float>( x ) ) + ( offsetY * scast<float>( y ) );
				
				ElementType tmp = std::make_shared<Tile>();
				tmp->Init( StageFormat::ID::Normal, base + offset, wholeSize );
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
	pModel.reset();

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
	if ( !pModel || !pRenderer ) { return; }
	// else

	Donya::Model::Constants::PerModel::Common modelConstant{};
	modelConstant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	modelConstant.worldMatrix	= Donya::Vector4x4::Identity();
	pRenderer->UpdateConstant( modelConstant );
	pRenderer->ActivateConstantModel();

	pRenderer->Render( pModel->model, pModel->pose );

	pRenderer->DeactivateConstantModel();
}
void Map::DrawHitBoxes( const Donya::Collision::Box3F &wsScreen, RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
{
	ForEach
	(
		[&]( const ElementType &pElement )
		{
			if ( !pElement ) { return; }
			// else

			if ( !Donya::Collision::IsHit( pElement->GetHitBox(), wsScreen ) ) { return; }
			// else

			pElement->DrawHitBox( pRenderer, matVP );
		}
	);
}
const std::vector<std::vector<Map::ElementType>> &Map::GetTiles() const
{
	return tilePtrs;
}
std::shared_ptr<const Tile> Map::GetPlaceTileOrNullptr( const Donya::Vector3 &wsPos ) const
{
	const auto ssPos = ToTilePos( wsPos );

	const int rowCount = scast<int>( tilePtrs.size() );
	if ( ssPos.y < 0 || rowCount <= ssPos.y ) { return nullptr; }
	// else

	const auto &row = tilePtrs[ssPos.y];

	const int columnCount = scast<int>( row.size() );
	if ( ssPos.x < 0 || columnCount <= ssPos.x ) { return nullptr; }
	// else

	return row[ssPos.x];
}
std::vector<std::shared_ptr<const Tile>> Map::GetPlaceTiles( const std::vector<Donya::Vector3> &wsPositions ) const
{
	const size_t count = wsPositions.size();
	
	std::vector<std::shared_ptr<const Tile>> results{ count };
	for ( size_t i = 0; i < count; ++i )
	{
		results[i] = GetPlaceTileOrNullptr( wsPositions[i] );
	}
	return std::move( results );
}
std::vector<std::shared_ptr<const Tile>> Map::GetPlaceTiles( const Donya::Collision::Box3F &wsArea, const Donya::Vector3 &wsVelocity ) const
{
	// Note: Currently, all Z component of the tiles is zero. So it only considers X and Y axis.

	// Make the hit box that covers an area of at least moving amount.
	Donya::Collision::Box3F extArea = wsArea;
	if ( !wsVelocity.IsZero() )
	{
		constexpr float margin = 0.1f;
		extArea.size.x += fabsf( wsVelocity.x ) + margin;
		extArea.size.y += fabsf( wsVelocity.y ) + margin;
//		extArea.size.z += fabsf( wsVelocity.z ) + margin;
	}

	const auto  areaCenter	= extArea.WorldPosition();
	const auto  areaMax		= extArea.Max();
	const auto  areaMin		= extArea.Min();
	const float &halfWidth	= extArea.size.x;
	const float &halfHeight	= extArea.size.y;

	std::vector<std::shared_ptr<const Tile>> results{};
	auto AppendByCalc = [&]( const Donya::Vector3 &wsPos )
	{
		// HACK: It can optimize by: Cache the tile-space position of "wsPos", then ignore same pos.

		results.emplace_back( GetPlaceTileOrNullptr( wsPos ) );
	};

	AppendByCalc( areaCenter ); // Center

	Donya::Vector3 offset{ 0.0f, 0.0f, 0.0f };
	for ( float offsetY = halfHeight; 0.0f <= offsetY; offsetY -= Tile::unitWholeSize )
	{
		for ( float offsetX = halfWidth; 0.0f <= offsetX; offsetX -= Tile::unitWholeSize )
		{
			offset.y = 0.0f;
			offset.x = offsetX;		AppendByCalc( areaCenter + offset ); // Right
			offset.x = -offsetX;	AppendByCalc( areaCenter + offset ); // Left

			offset.y = offsetY;
			offset.x = 0.0f;		AppendByCalc( areaCenter + offset ); // Top
			offset.x = offsetX;		AppendByCalc( areaCenter + offset ); // Right-Top
			offset.x = -offsetX;	AppendByCalc( areaCenter + offset ); // Left-Top
			
			offset.y = -offsetY;
			offset.x = 0.0f;		AppendByCalc( areaCenter + offset ); // Bottom
			offset.x = offsetX;		AppendByCalc( areaCenter + offset ); // Right-Bottom
			offset.x = -offsetX;	AppendByCalc( areaCenter + offset ); // Left-Bottom
		}
	}

	return std::move( results );
}
bool Map::LoadMap( int stageNumber, bool fromBinary )
{
	const std::string filePath	= ( fromBinary )
								? MakeStageParamPathBinary( ID, stageNumber )
								: MakeStageParamPathJson  ( ID, stageNumber );
	Donya::Serializer tmp;
	return	( fromBinary )
			? tmp.LoadBinary( *this, filePath.c_str(), ID )
			: tmp.LoadJSON	( *this, filePath.c_str(), ID );
}
#if USE_IMGUI
void Map::ReloadModel( int loadStageNumber )
{
	const bool loadResult = LoadStageModel( &pModel, loadStageNumber );
	if ( !loadResult )
	{
		std::string msg = u8"マップの読み込みに失敗しました。\n";
		msg += u8"ステージ番号：[" + std::to_string( loadStageNumber ) + u8"]";
		Donya::ShowMessageBox
		(
			msg,
			"Loading stage is failed",
			MB_ICONEXCLAMATION | MB_OK
		);
	}
}
void Map::RemakeByCSV( const CSVLoader &loadedData )
{
	auto IsTileID	= []( int id )
	{
		switch ( id )
		{
		case StageFormat::Normal:	return true;
		case StageFormat::Ladder:	return true;
		case StageFormat::Needle:	return true;
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
		tmp->Init( scast<StageFormat::ID>( id ), ToWorldPos( row, column ), Tile::unitWholeSize );
		pOutput->emplace_back( std::move( tmp ) );
	};

	ForEach( []( ElementType &pElement ) { if ( pElement ) { pElement->Uninit(); } } );
	tilePtrs.clear();

	const auto &data = loadedData.Get();
	const size_t rowCount = data.size();

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

	Donya::Serializer tmp;
	( fromBinary )
	? tmp.SaveBinary( *this, filePath.c_str(), ID )
	: tmp.SaveJSON	( *this, filePath.c_str(), ID );
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
					continue;
				}
				// else

				const bool treeIsOpen = pTile->ShowImGuiNode( caption );
				if ( !treeIsOpen )
				{
					caption = MakeCoordinateStr( pTile->GetPosition() );
					ImGui::SameLine();
					ImGui::Text( caption.c_str() );
				}
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
