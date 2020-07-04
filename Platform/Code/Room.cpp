#include "Room.h"

#include <algorithm>		// Use std::sort, std::find

#include "Donya/Color.h"

#include "FilePath.h"
#include "Map.h"			// Use Map::ToWorldPos()
#include "Parameter.h"
#if USE_IMGUI
#include "StageFormat.h"
#endif // USE_IMGUI


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
const Donya::Collision::Box3F &Room::GetArea() const
{
	return area;
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

	ImGui::Text( u8"ID:%d", id );
	ImGui::InputInt( u8"Ú‘±æ‚ÌID", &connectingRoomID );
	Definition::ShowImGuiNode( u8"‘JˆÚ‰Â”\•ûŒü‚ÌÝ’è", &transition );
	ImGui::Helper::ShowAABBNode( u8"”ÍˆÍÝ’è", &area );

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
int House::CalcBelongRoomID( const Donya::Vector3 &wsSearchPoint ) const
{
	for ( const auto &it : rooms )
	{
		// Note: This method is not support an overlapping some rooms,
		// but I am assuming a stage was not make as that.
		if ( Donya::Collision::IsHit( wsSearchPoint, it.second.GetArea() ) )
		{
			return it.first;
		}
	}

	return -1;
}
bool House::LoadRooms( int stageNo, bool fromBinary )
{
	const std::string filePath	= ( fromBinary )
								? MakeStageParamPathBinary( serializeID, stageNo )
								: MakeStageParamPathJson  ( serializeID, stageNo );
	return Donya::Serializer::Load( *this, filePath.c_str(), serializeID, fromBinary );
}
#if USE_IMGUI
void House::RemakeByCSV( const CSVLoader &loadedData )
{
	for ( auto &it : rooms ) { it.second.Uninit(); }
	rooms.clear();

	class Area
	{
	public:
		Donya::Vector3 min{ +FLT_MAX, +FLT_MAX, +FLT_MAX };
		Donya::Vector3 max{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
	public:
		void Register( const Donya::Vector3 &pos )
		{
			min.x = std::min( pos.x, min.x );
			min.y = std::min( pos.y, min.y );
			min.z = std::min( pos.z, min.z );
			
			max.x = std::max( pos.x, max.x );
			max.y = std::max( pos.y, max.y );
			max.z = std::max( pos.z, max.z );
		}
	};
	std::unordered_map<int, Area> areas;

	auto IsRoomID	= []( int id )
	{
		return ( StageFormat::RoomStart <= id && id <= StageFormat::RoomLast );
	};
	auto Append		= [&IsRoomID, &areas]( int id, size_t row, size_t column )
	{
		if ( !IsRoomID( id ) ) { return; }
		// else

		const Donya::Vector3 wsPos = Map::ToWorldPos( row, column );

		auto found = areas.find( id );
		if ( found == areas.end() )
		{
			Area tmp;
			tmp.Register( wsPos );
			areas.insert( std::make_pair( id, std::move( tmp ) ) );
		}
		else
		{
			found->second.Register( wsPos );
		}

	};

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

	// Append function will register the center pos of tile,
	// But I want the full size, so expand half tile size to outside.
	{
		constexpr Donya::Vector3 halfTileSize
		{
			Tile::unitWholeSize * 0.5f,
			Tile::unitWholeSize * 0.5f,
			0.0f
		};
		for ( auto &it : areas )
		{
			it.second.min -= halfTileSize;
			it.second.max += halfTileSize;
		}
	}

	Room argument;
	for ( const auto &it : areas )
	{
		argument.Init( it.first, it.second.min, it.second.max );
		rooms.insert( std::make_pair( it.first, argument ) );
	}
}
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

	if ( ImGui::TreeNode( u8"ŽÀ‘Ì‚½‚¿" ) )
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
		rooms.clear();
		LoadRooms( stageNo, true );
	}
	else if ( result == Op::LoadJson )
	{
		rooms.clear();
		LoadRooms( stageNo, false );
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI
