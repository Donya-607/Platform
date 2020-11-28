#pragma once

#include <vector>

#undef max
#undef min
#include "cereal/types/vector.hpp"

#include "Donya/Collision.h"
#include "Donya/Constant.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "CSVLoader.h"
#include "Direction.h"
#include "ModelHelper.h"
#include "Renderer.h"

namespace Door
{
	bool LoadResource();
#if USE_IMGUI
	void UpdateParameter( const std::string &nodeCaption );
#endif // USE_IMGUI

	class Instance
	{
	private: // Serialize parameters
		Donya::Collision::Box3F	body;
		Donya::Collision::Box3F	triggerArea; // The "pos" is same as the "body.pos"
		Definition::Direction	passDirection = Definition::Direction::Right; // The direction that a player can pass-through. e.g., the "Right" specification only allows the path-throughing that from left to right.
		Donya::Quaternion		orientation;
	private: // State parameters
		enum class Motion { Open = 0, Close };
		Motion							nowMotion	= Motion::Open;
		bool							nowOpen		= false;
		bool							nowPlaying	= false;
		ModelHelper::SkinningOperator	model;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( body			),
				CEREAL_NVP( triggerArea		),
				CEREAL_NVP( passDirection	),
				CEREAL_NVP( orientation		)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		void Init();
		void Update( float elapsedTime );
		void Draw( RenderingHelper *pRenderer ) const;
	#if DEBUG_MODE
		void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const;
	#endif // DEBUG_MODE
	public:
		const Donya::Collision::Box3F &GetBody()		const;
		const Donya::Collision::Box3F &GetTriggerArea()	const;
	public:
		void AssignParameter( const Donya::Vector3 &wsBaseFootPos );
	public:
		bool NowOpen() const;
		void Open();
		void Close();
	private:
		void AssignRotatedBodies( Definition::Direction enablePassDirection, const Donya::Vector3 &wsBaseFootPos );
		void StartMotion( Motion motionKind );
		Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const;
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};

	class Container
	{
	private:
		std::vector<Instance> doors;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( doors ) );

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
		static constexpr const char *ID = "Doors";
	public:
	#if DEBUG_MODE
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const;
	#endif // DEBUG_MODE
	public:
		Instance *FetchDetectedDoorOrNullptr( const Donya::Collision::Box3F &wsVerifyArea );
		std::vector<Donya::Collision::Box3F> GetDoorBodies() const;
	public:
		void LoadParameter( int stageNo );
	private:
		void LoadBin( int stageNo );
		void LoadJson( int stageNo );
	#if USE_IMGUI
	public:
		void RemakeByCSV( const CSVLoader &loadedData );
		void SaveBin( int stageNo );
		void SaveJson( int stageNo );
	public:
		void ShowImGuiNode( const std::string &nodeCaption, int stageNo, bool allowShowIONode = true );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Door::Instance,	0 )
CEREAL_CLASS_VERSION( Door::Container,	0 )
