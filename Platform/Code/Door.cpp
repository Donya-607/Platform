#include "Door.h"

#include "Donya/Useful.h"

#include "Common.h"
#include "DoorParam.h"
#include "FilePath.h"
#include "Map.h"
#include "Parameter.h"
#include "StageFormat.h"

namespace Door
{
	namespace
	{
		constexpr const char *modelName = "Map/Door";
		static std::shared_ptr<ModelHelper::SkinningSet> pModel{ nullptr };
		static ParamOperator<Parameter> parameter{ "Door" };

		bool LoadModel()
		{
			// Already loaded
			if ( pModel ) { return true; }
			// else

			const std::string filePath = MakeModelPath( modelName );

			if ( !Donya::IsExistFile( filePath ) )
			{
				const std::string msg = "Error: File is not found: " + filePath + "\n";
				Donya::OutputDebugStr( msg.c_str() );
				return false;
			}
			// else

			pModel = std::make_shared<ModelHelper::SkinningSet>();
			const bool result = ModelHelper::Load( filePath, pModel.get() );
			if ( !result )
			{
				pModel.reset(); // Make not loaded state

				const std::string msg = "Failed: Loading failed: " + filePath;
				Donya::OutputDebugStr( msg.c_str() );
				return false;
			}
			// else

			return true;
		}
		std::shared_ptr<ModelHelper::SkinningSet> GetModelPtrOrNullptr()
		{
			if ( !pModel )
			{
				_ASSERT_EXPR( 0, L"Error: The Door's model is not initialized!" );
				return nullptr;
			}
			// else

			return pModel;
		}
	}

	bool LoadResource()
	{
		parameter.LoadParameter();
		return LoadModel();
	}
	const Parameter &FetchParameter()
	{
		return parameter.Get();
	}
#if USE_IMGUI
	void UpdateParameter( const std::string &nodeCaption )
	{
		parameter.ShowImGuiNode( nodeCaption );
	}
#endif // USE_IMGUI

	void Instance::Init()
	{
		nowOpen		= false;
		nowPlaying	= false;
		model.Initialize( GetModelPtrOrNullptr() );
		model.animator.DisableLoop();
	}
	void Instance::Update( float elapsedTime )
	{
		if ( !nowPlaying ) { return; }
		// else

		model.UpdateMotion( elapsedTime, scast<int>( nowMotion ) );
		if ( model.animator.WasEnded() )
		{
			nowPlaying = false;
			if ( nowMotion != Motion::Open )
			{
				nowOpen = false;
			}
		}
	}
	void Instance::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer			) { return; }
		if ( !model.pResource	) { return; }
		// else

		const Donya::Vector3 &drawPos = body.pos; // I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
		const Donya::Vector4x4 W = MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );

		Donya::Model::Constants::PerModel::Common modelConstant{};
		modelConstant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		modelConstant.worldMatrix	= W;;
		pRenderer->UpdateConstant( modelConstant );
		pRenderer->ActivateConstantModel();

		pRenderer->Render( model.pResource->model, model.pose );

		pRenderer->DeactivateConstantModel();
	}
#if DEBUG_MODE
	void Instance::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !Common::IsShowCollision() ) { return; }
		// else
		
	#if DEBUG_MODE
		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= VP;
		constant.lightDirection	= -Donya::Vector3::Up();

		auto DrawProcess = [&]( const Donya::Collision::Box3F &box, const Donya::Vector4 &color )
		{
			const Donya::Vector4x4 W = MakeWorldMatrix
			(
				box.size * 2.0f,
				/* enableRotation = */ false,
				box.WorldPosition()
			);

			constant.matWorld  = W;
			constant.drawColor = color;
			pRenderer->ProcessDrawingCube( constant );
		};
	
		constexpr Donya::Vector4 bodyColor{ 0.5f, 0.2f, 1.0f, 0.6f };
		constexpr Donya::Vector4 areaColor{ 0.0f, 1.0f, 0.3f, 0.6f };
		const auto  &area		= triggerArea;
		const float bodyNear	= body.WorldPosition().z - body.size.z;
		const float areaNear	= area.WorldPosition().z - area.size.z;
		// Drawing the far box first
		if ( bodyNear < areaNear )
		{
			DrawProcess( area, areaColor );
			DrawProcess( body, bodyColor );
		}
		else
		{
			DrawProcess( body, bodyColor );
			DrawProcess( area, areaColor );
		}
	#endif // DEBUG_MODE
	}
#endif // DEBUG_MODE
	const Donya::Collision::Box3F &Instance::GetBody() const
	{
		return body;
	}
	const Donya::Collision::Box3F &Instance::GetTriggerArea() const
	{
		return triggerArea;
	}
	void Instance::AssignParameter( const Donya::Vector3 &wsBasePos )
	{
		AssignRotatedBodies( passDirection, wsBasePos );
	}
	bool Instance::NowOpen() const
	{
		return nowOpen;
	}
	void Instance::Open()
	{
		nowOpen = true;
		StartMotion( Motion::Open );
	}
	void Instance::Close()
	{
		StartMotion( Motion::Close );
	}
	void Instance::AssignRotatedBodies( Definition::Direction enablePassDir, const Donya::Vector3 &wsBasePos )
	{
		constexpr Donya::Quaternion rotRoll90degCW{ 0.0f, 0.0f, 0.7071f, 0.7071f };
		constexpr Donya::Quaternion rotYaw180degCW{ 0.0f, 1.0f, 0.0000f, 0.0000f };
		
		// Assign the default(Right direction) body data
		const auto &data = FetchParameter();
		passDirection	= enablePassDir;
		body			= data.hitBox;
		body.pos		= wsBasePos;
		triggerArea		= data.triggerAreaBase;
		triggerArea.pos	= wsBasePos;

		auto Abs = []( const Donya::Vector3 &v )
		{
			return Donya::Vector3
			{
				fabsf( v.x ),
				fabsf( v.y ),
				fabsf( v.z )
			};
		};
		auto RotateBodies = [&]( const Donya::Quaternion &rotation )
		{
			body.offset	= rotation.RotateVector( body.offset	);
			body.size	= rotation.RotateVector( body.size		);
			triggerArea.offset	= rotation.RotateVector( triggerArea.offset	);
			triggerArea.size	= rotation.RotateVector( triggerArea.size	);

			body.size			= Abs( body.size		);
			triggerArea.size	= Abs( triggerArea.size	);
		};

		using Dir = Definition::Direction;

		// Rotate the bodies if the direction is not Right

		if ( passDirection == Dir::Left || passDirection == Dir::Up )
		{
			if ( passDirection == Dir::Up )
			{
				RotateBodies( rotRoll90degCW );
			}

			RotateBodies( rotYaw180degCW );
		}
		else
		if ( passDirection == Dir::Down )
		{
			RotateBodies( rotRoll90degCW );
		}
		else
		// Also come here when the "passDirection" has multiple direction
		{
			// Enable only uni-direction
			passDirection = Dir::Right;
		}
	}
	void Instance::StartMotion( Motion motionKind )
	{
		nowMotion	= motionKind;
		nowPlaying	= true;
		model.AssignMotion( scast<int>( nowMotion ) );
	}
	Donya::Vector4x4 Instance::MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const
	{
		Donya::Vector4x4 W{};
		W._11 = scale.x;
		W._22 = scale.y;
		W._33 = scale.z;
		if ( enableRotation ) { W *= orientation.MakeRotationMatrix(); }
		W._41 = translation.x;
		W._42 = translation.y;
		W._43 = translation.z;
		return W;
	}
#if USE_IMGUI
	void Instance::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::Text( u8"「中心座標」が自機の足元となります" );
		ImGui::Helper::ShowAABBNode( u8"障害判定",		&body			);
		ImGui::Helper::ShowAABBNode( u8"通過判定範囲",	&triggerArea	);

		const auto oldDirection = passDirection;
		Definition::ShowImGuiNode( u8"通過可能方向",		&passDirection,	/* allowMultipleDirection = */ false );
		if ( passDirection != oldDirection )
		{
			AssignRotatedBodies( passDirection, body.pos );
		}

		ImGui::Helper::ShowFrontNode( u8"現在の前方向",	&orientation	);

		ImGui::TreePop();
	}
#endif // USE_IMGUI



#if DEBUG_MODE
	void Container::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

		for ( const auto &it : doors )
		{
			it.Draw( pRenderer );
		}
	}
#endif // DEBUG_MODE
	Instance *Container::FetchDetectedDoorOrNullptr( const Donya::Collision::Box3F &other )
	{
		for ( auto &it : doors )
		{
			if ( !it.NowOpen() ) { continue; }
			// else

			if ( Donya::Collision::IsHit( it.GetTriggerArea(), other ) )
			{
				return &it;
			}
		}

		return nullptr;
	}
	std::vector<Donya::Collision::Box3F> Container::GetDoorBodies() const
	{
		std::vector<Donya::Collision::Box3F> bodies;
		bodies.reserve( doors.size() );

		for ( const auto &it : doors )
		{
			if ( !it.NowOpen() ) { continue; }
			// else

			bodies.emplace_back( it.GetBody() );
		}

		return std::move( bodies );
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
		auto AppendIfPoint = [&]( int id, size_t r, size_t c )
		{
			if ( id != StageFormat::Door ) { return; }
			// else

			Donya::Vector3 wsFootPos = Map::ToWorldPos( r, c, /* alignToCenterOfTile = */ true );
			wsFootPos.y -= Tile::unitWholeSize * 0.5f; // To bottom

			Instance tmp;
			tmp.AssignParameter( wsFootPos );
			doors.emplace_back( std::move( tmp ) );
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
			const size_t count = doors.size();
			for ( size_t i = 0; i < count; ++i )
			{
				doors[i].ShowImGuiNode( Donya::MakeArraySuffix( i ) );
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
