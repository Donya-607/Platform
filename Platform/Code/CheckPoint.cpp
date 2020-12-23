#include "CheckPoint.h"

#include "Donya/Useful.h"

#include "Common.h"
#include "FilePath.h"
#include "Map.h"
#include "Parameter.h"
#include "StageFormat.h"

namespace CheckPoint
{
	Donya::Vector3 Instance::GetWorldInitialPos() const
	{
		return area.pos;
	}
	bool Instance::ShouldLookingRight() const
	{
		return lookingRight;
	}
	const Donya::Collision::Box3F &Instance::GetArea() const
	{
		return area;
	}
	void Instance::AssignParameter( const Donya::Vector3 &wsFootPos, bool lookRight )
	{
		area.pos		= wsFootPos;
		lookingRight	= lookRight;

		// Prevent to forgot
		if ( area.size.IsZero() )
		{
			area.size = Tile::unitWholeSize * 0.5f;
		}
	}
	void Instance::Activate()
	{
		active = true;
	}
	void Instance::Deactivate()
	{
		active = false;
	}
	bool Instance::NowActive() const
	{
		return active;
	}
#if USE_IMGUI
	void Instance::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::Text( u8"「中心座標」が自機の足元となります" );
		ImGui::Helper::ShowAABBNode( u8"判定範囲",	&area			);
		ImGui::Checkbox( u8"右向きか",				&lookingRight	);
		ImGui::Checkbox( u8"今有効である",			&active			);

		ImGui::TreePop();
	}
#endif // USE_IMGUI



#if DEBUG_MODE
	void Container::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

		Donya::Model::Cube::Constant constant;
		constant.matViewProj		= VP;
		constant.drawColor			= Donya::Vector4{ 0.3f, 0.6f, 0.3f, 0.6f };
		constant.lightDirection		= -Donya::Vector3::Up();

		auto Draw = [&]( const Instance &element )
		{
			const auto &area = element.GetArea();

			constant.matWorld._11		= area.size.x * 2.0f;
			constant.matWorld._22		= area.size.y * 2.0f;
			constant.matWorld._33		= area.size.z * 2.0f;
			constant.matWorld._41		= area.pos.x + area.offset.x;
			constant.matWorld._42		= area.pos.y + area.offset.y;
			constant.matWorld._43		= area.pos.z + area.offset.z;

			pRenderer->ProcessDrawingCube( constant );
		};
	
		for ( const auto &it : areas )
		{
			Draw( it );
		}
	}
#endif // DEBUG_MODE
	Instance *Container::FetchPassedPointOrNullptr( const Donya::Collision::Box3F &other )
	{
		for ( auto &it : areas )
		{
			if ( !it.NowActive() ) { continue; }
			// else

			if ( Donya::Collision::IsHit( it.GetArea(), other ) )
			{
				return &it;
			}
		}

		return nullptr;
	}
	void Container::LoadParameter( int stageNo )
	{
	#if DEBUG_MODE
		LoadJson( stageNo );
		// If a user was changed only a json file, the user wanna apply the changes to binary file also.
		// So save here.
		SaveBin( stageNo );
	#else
		LoadBin( stageNo );
	#endif // DEBUG_MODE
	}
	void Container::LoadBin( int stageNo )
	{
		Donya::Serializer tmp;
		tmp.LoadBinary( *this, MakeStageParamPathBinary( ID, stageNo ).c_str(), ID );
	}
	void Container::LoadJson( int stageNo )
	{
		Donya::Serializer tmp;
		tmp.LoadJSON( *this, MakeStageParamPathJson( ID, stageNo ).c_str(), ID );
	}
	#if USE_IMGUI
	void Container::RemakeByCSV( const CSVLoader &loadedData )
	{
		areas.clear();

		auto AppendIfPoint = [&]( int id, size_t r, size_t c )
		{
			if ( id != StageFormat::CheckPoint ) { return; }
			// else

			Donya::Vector3 wsFootPos = Map::ToWorldPos( r, c, /* alignToCenterOfTile = */ true );
			wsFootPos.y -= Tile::unitWholeSize * 0.5f; // To bottom

			Instance tmp;
			tmp.AssignParameter( wsFootPos, /* lookingRight = */ true );
			areas.emplace_back( std::move( tmp ) );
		};

		const auto &data = loadedData.Get();
		const size_t rowCount = data.size();
		for ( size_t r = 0; r < rowCount; ++r )
		{
			const size_t columnCount = data[r].size();
			for ( size_t c = 0; c < columnCount; ++c )
			{
				AppendIfPoint( data[r][c], r, c );
			}
		}
	}
	void Container::SaveBin( int stageNo )
	{
		constexpr bool fromBinary = true;

		const std::string filePath = MakeStageParamPathBinary( ID, stageNo );
		MakeDirectoryIfNotExists( filePath );
		MakeFileIfNotExists( filePath, fromBinary );

		Donya::Serializer tmp;
		tmp.SaveBinary( *this, filePath.c_str(), ID );
	}
	void Container::SaveJson( int stageNo )
	{
		constexpr bool fromBinary = false;

		const std::string filePath = MakeStageParamPathJson( ID, stageNo );
		MakeDirectoryIfNotExists( filePath );
		MakeFileIfNotExists( filePath, fromBinary );

		Donya::Serializer tmp;
		tmp.SaveJSON( *this, filePath.c_str(), ID );
	}
	void Container::ShowImGuiNode( const std::string &nodeCaption, int stageNo, bool allowShowIONode )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::TreeNode( u8"実体たち" ) )
		{
			const size_t count = areas.size();
			for ( size_t i = 0; i < count; ++i )
			{
				areas[i].ShowImGuiNode( Donya::MakeArraySuffix( i ) );
			}

			ImGui::TreePop();
		}

		if ( allowShowIONode )
		{
			const auto result = ParameterHelper::ShowIONode();
			using Op = ParameterHelper::IOOperation;
			if ( result == Op::Save )
			{
				SaveBin ( stageNo );
				SaveJson( stageNo );
			}
			else if ( result == Op::LoadBinary )
			{
				LoadBin( stageNo );
			}
			else if ( result == Op::LoadJson )
			{
				LoadJson( stageNo );
			}
		}

		ImGui::TreePop();
	}
	#endif // USE_IMGUI

}
