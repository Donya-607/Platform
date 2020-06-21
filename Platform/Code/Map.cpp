#include "Map.h"

#include "Donya/Sprite.h"
#include "Donya/Constant.h"	// Use scast macro

#include "Common.h"			// Use IsShowCollision()
#include "FilePath.h"
#include "Parameter.h"		// Use ParameterHelper


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
#if USE_IMGUI
void Tile::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

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


#if DEBUG_MODE
void Map::EditOperator::Init( int stageNumber )
{
	mode = Mode::NotEnabled;

	gridline.Init();
	// My prefer initial settings.
	gridline.SetDrawOrigin  ( { 0.0f,	0.0f,	0.0f } );
	gridline.SetDrawLength  ( { 16.0f,	64.0f } );
	gridline.SetDrawInterval( {  1.0f,	 1.0f } );
}
void Map::EditOperator::Uninit()
{
	gridline.Uninit();
}
void Map::EditOperator::Activate()
{
	mode = Mode::Placement;
}
void Map::EditOperator::Deactivate()
{
	mode = Mode::NotEnabled;
}
void Map::EditOperator::Update( float elapsedTime, const Donya::Int2 &mousePos, const Donya::Vector4x4 &matVP )
{

}
void Map::EditOperator::Draw( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP )
{
	gridline.Draw( matVP );
}
Donya::Vector4x4 Map::EditOperator::MakeScreenTransformMatrix( const Donya::Vector4x4 &matViewProjection )
{
	const Donya::Vector4x4 matViewport = Donya::Vector4x4::MakeViewport( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
	return matViewProjection * matViewport;
}
#endif // DEBUG_MODE


bool Map::Init( int stageNumber )
{
	const bool succeeded = LoadMap( stageNumber, IOFromBinaryFile );

#if DEBUG_MODE
	editOperator.Init( stageNumber );

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
	}
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
#if DEBUG_MODE
void Map::ActivateEditorMode()
{
	editOperator.Activate();
}
void Map::DeactivateEditorMode()
{
	editOperator.Deactivate();
}
void Map::EditorUpdate( float elapsedTime, const Donya::Int2 &mousePos, const Donya::Vector4x4 &matVP )
{
	editOperator.Update( elapsedTime, mousePos, matVP );
}
void Map::EditorDraw( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP )
{
	editOperator.Draw( pRenderer, matVP );
}
#endif // DEBUG_MODE
const std::vector<Tile> &Map::GetTiles() const
{
	return tiles;
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
