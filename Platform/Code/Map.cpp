#include "Map.h"

#include <algorithm>		// Use remove_if

#include "Donya/Constant.h"	// Use scast macro
#include "Donya/Mouse.h"
#include "Donya/Sprite.h"

#include "Common.h"			// Use IsShowCollision()
#include "FilePath.h"
#include "Parameter.h"		// Use ParameterHelper
#include "StageFormat.h"
#if USE_IMGUI
#include "CSVLoader.h"
#endif // USE_IMGUI

#if DEBUG_MODE
#pragma comment( lib, "comdlg32.lib" ) // Used for common-dialog
#endif // DEBUG_MODE


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

#if DEBUG_MODE
	constexpr unsigned int maxPathBufferSize = MAX_PATH;
	std::string FetchStageFilePathByCommonDialog()
	{
		char chosenFullPaths[maxPathBufferSize] = { 0 };
		char chosenFileName [maxPathBufferSize] = { 0 };

		OPENFILENAMEA ofn{ 0 };
		ofn.lStructSize		= sizeof( decltype( ofn ) );
		ofn.hwndOwner		= Donya::GetHWnd();
		ofn.lpstrFilter		= "CSV-file(*.csv)\0*.csv\0"
							  "\0";
		ofn.lpstrFile		= chosenFullPaths;
		ofn.nMaxFile		= maxPathBufferSize;
		ofn.lpstrFileTitle	= chosenFileName;
		ofn.nMaxFileTitle	= maxPathBufferSize;
		ofn.Flags			= OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR; // Prevent the current directory of this application will be changed.

		auto  result = GetOpenFileNameA( &ofn );
		if ( !result ) { return std::string{}; }
		// else

		return std::string{ ofn.lpstrFile };
	}
#endif // DEBUG_MODE
}

bool Map::Init( int stageNumber )
{
	const bool succeeded = LoadMap( stageNumber, IOFromBinaryFile );

#if DEBUG_MODE
	// Generate test solids
	if ( tiles.empty() )
	{
		constexpr float				wholeSize = Tile::unitWholeSize;
		constexpr Donya::Vector3	base  {-wholeSize, 0.0f, 0.0f };
		constexpr Donya::Vector3	offset{ wholeSize, 0.0f, 0.0f };
		constexpr Donya::Int2		texOrigin{ 0, 0 };
		constexpr Donya::Vector3	points[]
		{
			base + ( offset * 0.0f ),
			base + ( offset * 1.0f ),
			base + ( offset * 2.0f ),
			base + ( offset * 3.0f ),
			base + ( offset * 4.0f ),
			base + ( offset * 5.0f ),
			base + ( offset * 6.0f ),
		};
		for ( const auto &it : points )
		{
			Tile tmp{};
			tmp.Init( it, wholeSize, texOrigin );
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

	if ( ImGui::Button( u8"CSVファイルからステージを生成" ) )
	{
		const auto filePath = FetchStageFilePathByCommonDialog();
		if ( !filePath.empty() && Donya::IsExistFile( filePath ) )
		{
			CSVLoader loader;
			const bool result = loader.Load( filePath );
			if ( result )
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

				const auto &data = loader.Get();
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
			else
			{
				Donya::ShowMessageBox
				(
					u8"パースに失敗しました。",
					"Loading stage is failed",
					MB_ICONEXCLAMATION | MB_OK
				);
			}
		}
		else
		{
			std::string msg = u8"ファイルロードに失敗しました。\n";
			msg += u8"ファイル：[" + filePath + u8"]";
			Donya::ShowMessageBox
			(
				msg,
				"Loading stage is failed",
				MB_ICONEXCLAMATION | MB_OK
			);
		}
	}

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
			caption = Donya::MakeArraySuffix( i );
			tiles[i].ShowImGuiNode( caption );
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
