#pragma once

#include <unordered_map>
#include <vector>

#undef max
#undef min
#include <cereal\types\unordered_map.hpp>
#include <cereal\types\vector.hpp>

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "CSVLoader.h"
#include "Direction.h"
#include "Renderer.h"


class Room
{
public:
	static constexpr int invalidID = -1;
private:
	int		id				= 0;
	float	hour			= 12.0f;
	Definition::Direction	transition = Definition::Direction::Nil; // Direction that be able to transition
	Donya::Collision::Box3F	area;			// Z component is infinite
	std::vector<int>		connectingRoomIDs;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( id			),
			CEREAL_NVP( transition	),
			CEREAL_NVP( area		)
		);
		if ( 1 <= version )
		{
			archive( CEREAL_NVP( hour ) );
		}
		if ( 2 <= version )
		{
			archive( CEREAL_NVP( connectingRoomIDs ) );
		}
		if ( 3 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Init( int id, const Donya::Vector3 &wsMinPos, const Donya::Vector3 &wsMaxPos );
	void Uninit();
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
public:
	bool  IsConnectTo( int verifyRoomID ) const;
	bool  IsConnectToAny() const;
	int   GetID() const;
	float GetHour() const;
	Definition::Direction GetTransitionableDirection() const;
	const Donya::Collision::Box3F &GetArea() const;
	/// <summary>
	/// Also consider connecting room area.
	/// </summary>
	Donya::Collision::Box3F CalcRoomArea( const std::unordered_map<int, Room> &house, std::vector<int> &ignoreIDs ) const;
	/// <summary>
	/// Also consider connecting room area.
	/// </summary>
	Donya::Collision::Box3F CalcRoomArea( const std::unordered_map<int, Room> &house ) const;
private:
	Donya::Collision::Box3F MakeArea( const Donya::Vector3 &wsMinPos, const Donya::Vector3 &wsMaxPos ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Room, 2 )


/// <summary>
/// Contain many Rooms.
/// </summary>
class House
{
private:
	std::unordered_map<int, Room> rooms; // Key is Room::id
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive( CEREAL_NVP( rooms ) );
		if ( 1 <= version )
		{
			// archive();
		}
	}
	static constexpr const char *serializeID = "House";
public:
	bool Init( int stageNo );
	void Uninit();
	void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
public:
	const Room *FindRoomOrNullptr( int roomID ) const;
	Donya::Collision::Box3F CalcRoomArea( int roomID ) const;
	/// <summary>
	/// Returns Room::invalidID(-1) if the argument is not belongs which rooms.
	/// </summary>
	int CalcBelongRoomID( const Donya::Vector3 &wsSearchPoint ) const;
	bool LoadRooms( int stageNo, bool fromBinary );
#if USE_IMGUI
public:
	void RemakeByCSV( const CSVLoader &loadedData );
	void SaveRooms( int stageNo, bool fromBinary );
public:
	void ShowImGuiNode( const std::string &nodeCaption, int stageNo );
	void ShowInstanceNode( const std::string &nodeCaption, int roomID );
	void ShowIONode( int stageNo );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( House, 0 )
