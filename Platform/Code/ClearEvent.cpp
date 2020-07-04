#include "ClearEvent.h"

#include "Common.h"
#include "FilePath.h"

#if USE_IMGUI
#include "Map.h"			// Use Map::ToWorldPos()
#include "Parameter.h"
#include "StageFormat.h"
#endif // USE_IMGUI

namespace
{
#if DEBUG_MODE
	constexpr bool IOFromBinaryFile = false;
#else
	constexpr bool IOFromBinaryFile = true;
#endif // DEBUG_MODE
}

bool ClearEvent::Init( int stageNo )
{
	const bool succeeded = LoadEvents( stageNo, IOFromBinaryFile );

#if DEBUG_MODE
	// If a user was changed only a json file, the user wanna apply the changes to binary file also.
	// So save here.
	SaveEvents( stageNo, /* fromBinary = */ true );

	return true;
#endif // DEBUG_MODE

	return succeeded;
}
void ClearEvent::Uninit() {}
void ClearEvent::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else

	Donya::Model::Cube::Constant constant;
	constant.matViewProj		= VP;
	constant.drawColor			= Donya::Vector4{ 1.0f, 0.8f, 0.5f, 0.6f };
	constant.lightDirection		= -Donya::Vector3::Up();

	auto DrawEvent = [&]( const Event &element )
	{
		constexpr float wholeSize	= 0.5f;
		constant.matWorld._11		= wholeSize;
		constant.matWorld._22		= wholeSize;
		constant.matWorld._33		= wholeSize;
		constant.matWorld._41		= element.wsPos.x;
		constant.matWorld._42		= element.wsPos.y;
		constant.matWorld._43		= element.wsPos.z;

		pRenderer->ProcessDrawingCube( constant );
	};
	
	for ( const auto &it : events )
	{
		DrawEvent( it );
	}
}
void ClearEvent::ApplyRoomID( const House &house )
{
	for ( auto &it : events )
	{
		it.roomID = house.CalcBelongRoomID( it.wsPos );
	}
}
bool ClearEvent::LoadEvents( int stageNo, bool fromBinary )
{
	const std::string filePath	= ( fromBinary )
								? MakeStageParamPathBinary( serializeID, stageNo )
								: MakeStageParamPathJson  ( serializeID, stageNo );
	return Donya::Serializer::Load( *this, filePath.c_str(), serializeID, fromBinary );
}
#if USE_IMGUI
void ClearEvent::RemakeByCSV( const CSVLoader &loadedData )
{
	auto IsEventID	= []( int id )
	{
		return ( id == StageFormat::ClearEvent );
	};
	auto Append		= [&]( int id, size_t row, size_t column )
	{
		if ( !IsEventID( id ) ) { return; }
		// else

		Event tmp{};
		tmp.roomID	= Room::invalidID;
		tmp.wsPos	= Map::ToWorldPos( row, column );
		events.emplace_back( std::move( tmp ) );
	};

	events.clear();

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
void ClearEvent::SaveEvents( int stageNo, bool fromBinary )
{
	const std::string filePath	= ( fromBinary )
								? MakeStageParamPathBinary( serializeID, stageNo )
								: MakeStageParamPathJson  ( serializeID, stageNo );
	MakeDirectoryIfNotExists( filePath );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), serializeID, fromBinary );
}
void ClearEvent::ShowImGuiNode( const std::string &nodeCaption, const House &house, int stageNo )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	if ( ImGui::Button( u8"ルーム番号を更新" ) )
	{
		ApplyRoomID( house );
	}

	if ( ImGui::TreeNode( u8"実体たち" ) )
	{
		auto ShowElement = []( const std::string &nodeCaption, Event *p )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
			// else

			ImGui::InputInt  ( u8"ルーム番号",	&p->roomID );
			ImGui::DragFloat3( u8"ワールド座標",	&p->wsPos.x, 0.1f );

			ImGui::TreePop();
			return true;
		};

		const size_t eventCount = events.size();
		std::string caption;
		for ( size_t i = 0; i < eventCount; ++i )
		{
			caption = Donya::MakeArraySuffix( i );
			const bool treeIsOpen = ShowElement( caption, &events[i] );

			if ( !treeIsOpen )
			{
				caption = "[RoomID:";
				caption += std::to_string( events[i].roomID );
				caption += "]";
				ImGui::SameLine();
				ImGui::Text( caption.c_str() );
			}
		}

		ImGui::TreePop();
	}

	const auto result = ParameterHelper::ShowIONode();
	using Op = ParameterHelper::IOOperation;
	if ( result == Op::Save )
	{
		SaveEvents( stageNo, true  );
		SaveEvents( stageNo, false );
	}
	else if ( result == Op::LoadBinary )
	{
		events.clear();
		LoadEvents( stageNo, true );
	}
	else if ( result == Op::LoadJson   )
	{
		events.clear();
		LoadEvents( stageNo, false );
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI
