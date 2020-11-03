#pragma once

#include <vector>

#include "cereal/types/vector.hpp"

#include "Donya/Collision.h"
#include "Donya/Constant.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "CSVLoader.h"
#include "Renderer.h"

namespace CheckPoint
{
	class Instance
	{
		Donya::Collision::Box3F	area; // The "pos" will be the coordinate that using as the player's initial point
		bool					lookingRight = true;
	private:
		bool					active = true;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( area			),
				CEREAL_NVP( lookingRight	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		/// <summary>
		/// Returns a foot position
		/// </summary>
		Donya::Vector3	GetWorldInitialPos() const;
		bool			ShouldLookingRight() const;
		const Donya::Collision::Box3F &GetArea() const;
	public:
		void AssignParameter( const Donya::Vector3 &wsFootPos, bool lookingRight );
	public:
		void Activate();
		void Deactivate();
		bool NowActive() const;
	#if USE_IMGUI
	public:
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};

	struct Result
	{
		bool			passed = false;
		Donya::Vector3	wsInitialPos; // It is valid when the "passed" is true
	};

	class Container
	{
	private:
		std::vector<Instance> areas;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( areas ) );

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
		static constexpr const char *ID = "CheckPoints";
	public:
	#if DEBUG_MODE
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const;
	#endif // DEBUG_MODE
	public:
		Instance *FetchPassedPointOrNullptr( const Donya::Collision::Box3F &wsVerifyArea );
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
CEREAL_CLASS_VERSION( CheckPoint::Instance,		0 )
CEREAL_CLASS_VERSION( CheckPoint::Container,	0 )
