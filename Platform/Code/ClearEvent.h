#pragma once

#include <vector>

#include <cereal/types/vector.hpp>

#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "CSVLoader.h"
#include "Room.h"

/// <summary>
/// The container of trigger that event of clear game.
/// </summary>
class ClearEvent
{
public:
	struct Event
	{
		int				roomID = Room::invalidID;
		Donya::Vector3	wsPos;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( roomID	),
				CEREAL_NVP( wsPos	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	std::vector<Event> events;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive( CEREAL_NVP( events ) );
		if ( 1 <= version )
		{
			// archive();
		}
	}
	static constexpr const char *serializeID = "ClearEvent";
public:
	bool Init( int stageNo );
	void Uninit();
	void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
public:
	bool IsThereEvent( int roomID ) const;
public:
	void ApplyRoomID( const House &house );
	bool LoadEvents( int stageNo, bool fromBinary );
#if USE_IMGUI
public:
	void RemakeByCSV( const CSVLoader &loadedData );
	void SaveEvents( int stageNo, bool fromBinary );
public:
	void ShowImGuiNode( const std::string &nodeCaption, const House &house, int stageNo );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( ClearEvent,			0 )
CEREAL_CLASS_VERSION( ClearEvent::Event,	0 )
