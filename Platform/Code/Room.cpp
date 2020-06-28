#include "Room.h"

#include <algorithm>	// Use std::sort, std::find

#include "Donya/Color.h"

#include "FilePath.h"
#include "Parameter.h"


void Room::Init( int assignID, const Donya::Vector3 &wsMin, const Donya::Vector3 &wsMax )
{
	id		= assignID;
	area	= MakeArea( wsMin, wsMax );
}
void Room::Uninit() {}
void Room::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
{
	constexpr float maxSize = 1.0f;
	Donya::Vector4x4 W;
	W._11 = area.size.x * 2.0f;
	W._22 = area.size.y * 2.0f;
	W._33 = maxSize;
	W._41 = area.pos.x;
	W._42 = area.pos.y;
	W._43 = area.pos.z;

	Donya::Model::Cube::Constant constant;
	constant.matWorld			= W;
	constant.matViewProj		= VP;
	constant.drawColor			= Donya::Vector4{ Donya::Color::MakeColor( Donya::Color::Code::ORANGE ), 0.2f };
	constant.lightDirection		= -Donya::Vector3::Up();
	pRenderer->ProcessDrawingCube( constant );
}
int  Room::GetID() const
{
	return id;
}
Donya::Collision::Box3F Room::CalcRoomArea( const std::unordered_map<int, Room> &house, std::vector<int> &ignoreIDs ) const
{
	if ( connectingRoomID == invalidID ) { return area; }
	if ( connectingRoomID == this->id  ) { return area; }
	// else

	const auto found = house.find( connectingRoomID );
	if ( found == house.end() ) { return area; }
	// else

	const auto &other = found->second;

	// Flags that prevent circular reference
	const auto containMyID		= std::find( ignoreIDs.begin(), ignoreIDs.end(), this->id );
	const bool ignoreMe			= ( containMyID != ignoreIDs.end() );
	const bool connectToOther	= ( other.connectingRoomID != invalidID ) && ( other.connectingRoomID != this->id );

	Donya::Collision::Box3F otherArea;
	if ( connectToOther && !ignoreMe )
	{
		// Prevent an other->connecting->connecting->...->me
		ignoreIDs.emplace_back( this->id );

		otherArea = other.CalcRoomArea( house, ignoreIDs );
	}
	else
	{
		otherArea = other.area;
	}

	const auto myMin = area.Min();
	const auto myMax = area.Max();
	const auto otherMin = otherArea.Min();
	const auto otherMax = otherArea.Max();

	Donya::Vector3 min;
	min.x = std::min( myMin.x, otherMin.x );
	min.y = std::min( myMin.y, otherMin.y );
	min.z = std::min( myMin.z, otherMin.z );
	Donya::Vector3 max;
	max.x = std::max( myMax.x, otherMax.x );
	max.y = std::max( myMax.y, otherMax.y );
	max.z = std::max( myMax.z, otherMax.z );
	return MakeArea( min, max );
}
Donya::Collision::Box3F Room::CalcRoomArea( const std::unordered_map<int, Room> &house ) const
{
	std::vector<int> empty; // Make long scope for reference argument
	return CalcRoomArea( house, empty );
}
Donya::Collision::Box3F Room::MakeArea( const Donya::Vector3 &min, const Donya::Vector3 &max ) const
{
	const float halfWidth  = ( max.x - min.x ) * 0.5f;
	const float halfHeight = ( max.y - min.y ) * 0.5f;
	Donya::Collision::Box3F tmp{};
	tmp.pos.x	= min.x + halfWidth;
	tmp.pos.y	= min.y + halfHeight;
	tmp.pos.z	= 0.0f;
	tmp.size.x	= fabsf( halfWidth  );
	tmp.size.y	= fabsf( halfHeight );
	tmp.size.z	= FLT_MAX;
	tmp.exist	= true;
	return tmp;
}
#if USE_IMGUI
void Room::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::Text( u8"Myself__ID:%d", id );
	ImGui::Text( u8"Connect_ID:%d", connectingRoomID );
	Definition::ShowImGuiNode( u8"遷移可能方向の設定", &transition, /* useTreeNode = */ false );
	ImGui::Helper::ShowAABBNode( u8"範囲設定", &area );

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

bool House::Init( int stageNo )
{
	const bool succeeded = LoadRooms( stageNo, IOFromBinaryFile );

#if DEBUG_MODE
	// If a user was changed only a json file, the user wanna apply the changes to binary file also.
	// So save here.
	SaveRooms( stageNo, /* fromBinary = */ true );

	return true;

#endif // DEBUG_MODE

	return succeeded;
}
void House::Uninit()
{
	for ( auto &it : rooms )
	{
		it.second.Uninit();
	}
}
void House::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
{
	for ( const auto &it : rooms )
	{
		it.second.DrawHitBox( pRenderer, VP );
	}
}
const Room *House::FindRoomOrNullptr( int roomID ) const
{
	const auto found = rooms.find( roomID );
	if ( found == rooms.end() ) { return nullptr; }
	// else

	return &found->second;
}
Donya::Collision::Box3F House::CalcRoomArea( int roomID ) const
{
	const auto p = FindRoomOrNullptr( roomID );
	return ( p ) ? p->CalcRoomArea( rooms ) : Donya::Collision::Box3F::Nil();
}
void House::RemakeByCSV( const CSVLoader &loadedData )
{

}
bool House::LoadRooms( int stageNo, bool fromBinary )
{
	const std::string filePath	= ( fromBinary )
								? MakeStageParamPathBinary( serializeID, stageNo )
								: MakeStageParamPathJson  ( serializeID, stageNo );
	return Donya::Serializer::Load( *this, filePath.c_str(), serializeID, fromBinary );
}
#if USE_IMGUI
void House::SaveRooms( int stageNo, bool fromBinary )
{
	const std::string filePath	= ( fromBinary )
								? MakeStageParamPathBinary( serializeID, stageNo )
								: MakeStageParamPathJson  ( serializeID, stageNo );
	MakeDirectoryIfNotExists( filePath );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), serializeID, fromBinary );
}
void House::ShowImGuiNode( const std::string &nodeCaption, int stageNo )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	if ( ImGui::Button( u8"Tempを追加" ) )
	{
		static int id = -1;
		static Room argument;
		++id;
		constexpr float size = 10.0f;
		const Donya::Vector3 pos{ size * id, 0.0f, 0.0f };
		argument.Init( id, pos, pos + Donya::Vector3{ size, -size, 0.0f } );
		rooms.insert( std::make_pair( id, argument ) );
	}

	if ( ImGui::TreeNode( u8"実体たち" ) )
	{
		using ElementType = Room *;
		const size_t roomCount = rooms.size();
		std::vector<ElementType> roomPtrs{};
		for ( auto &it : rooms )
		{
			roomPtrs.emplace_back( &it.second );
		}

		auto AscendingCompare = []( const ElementType &lhs, const ElementType &rhs )
		{
			return ( lhs->GetID() < rhs->GetID() );
		};
		std::sort( roomPtrs.begin(), roomPtrs.end(), AscendingCompare );

		std::string caption;
		for ( size_t i = 0; i < roomCount; ++i )
		{
			auto &pRoom = roomPtrs[i];

			if ( !pRoom ) { continue; }
			// else

			caption = Donya::MakeArraySuffix( i );
			pRoom->ShowImGuiNode( caption );

			caption = "[ID:";
			caption += std::to_string( pRoom->GetID() );
			caption += "]";
			ImGui::SameLine();
			ImGui::Text( caption.c_str() );
		}

		ImGui::TreePop();
	}

	const auto result = ParameterHelper::ShowIONode();
	using Op = ParameterHelper::IOOperation;
	if ( result == Op::Save )
	{
		SaveRooms( stageNo, true  );
		SaveRooms( stageNo, false );
	}
	else if ( result == Op::LoadBinary )
	{
		LoadRooms( stageNo, true );
	}
	else if ( result == Op::LoadJson )
	{
		LoadRooms( stageNo, false );
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI
