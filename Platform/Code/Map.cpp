#include "Map.h"

#include "Donya/Sprite.h"
#include "Donya/Constant.h"	// Use scast macro

#include "Common.h"			// Use IsShowCollision()
#include "FilePath.h"
#include "Parameter.h"		// Use ParameterHelper

void Tile::Init( const Donya::Vector3 &wsTilePos, const Donya::Vector3 &wsTileWholeSize, const Donya::Int2 &texCoordOffset )
{
	pos				= wsTilePos;
	hitBox.pos		= Donya::Vector3::Zero();
	hitBox.size		= wsTileWholeSize * 0.5f;
	hitBox.exist	= true;
	texOffset		= texCoordOffset;
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
	W._11 = hitBox.size.x * 2.0f;
	W._22 = hitBox.size.y * 2.0f;
	W._33 = hitBox.size.z * 2.0f;
	W._41 = pos.x + hitBox.pos.x;
	W._42 = pos.y + hitBox.pos.y;
	W._43 = pos.z + hitBox.pos.z;
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

	ImGui::DragFloat3( u8"ワールド座標",			&pos.x, 0.01f );
	ImGui::Helper::ShowAABBNode( u8"当たり判定",	&hitBox );
	ImGui::DragInt2  ( u8"テクスチャオフセット",	&texOffset.x );

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
	// Generate test solids
	if ( tiles.empty() )
	{
		constexpr float				wholeSize = 1.0f;
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
bool Map::LoadMap( int stageNumber, bool fromBinary )
{
	const std::string filePath = MakeStageParamPathBinary( ID, stageNumber );
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
