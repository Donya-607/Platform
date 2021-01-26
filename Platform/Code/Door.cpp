#include "Door.h"

#include "Donya/Sound.h"
#include "Donya/Useful.h"

#include "Common.h"
#include "DoorParam.h"
#include "FilePath.h"
#include "Map.h"
#include "Music.h"
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
	void Parameter::ShowImGuiNode()
	{
		ImGui::Helper::ShowAABBNode( u8"障害判定",		&hitBox				);
		ImGui::Helper::ShowAABBNode( u8"通過判定範囲",	&triggerAreaBase	);
		// Make to tell as disable
		hitBox.pos			= Donya::Vector3::Zero();
		triggerAreaBase.pos	= Donya::Vector3::Zero();

		ImGui::DragFloat( u8"通過判定の拡大量", &triggerAreaAddSize, 0.01f );
		triggerAreaAddSize = std::max( 0.01f, triggerAreaAddSize );

		ImGui::DragFloat( u8"開・モーション速度", &animeSpeedOpen,	0.01f );
		ImGui::DragFloat( u8"閉・モーション速度", &animeSpeedClose,	0.01f );
		animeSpeedOpen	= std::max( 0.01f, animeSpeedOpen	);
		animeSpeedClose	= std::max( 0.01f, animeSpeedClose	);
	}
#endif // USE_IMGUI

	void Instance::Init()
	{
		nowOpen		= false;
		nowPlaying	= false;
		model.Initialize( GetModelPtrOrNullptr() );
		model.animator.DisableLoop();
		model.AssignMotion( scast<int>( Motion::Open ) );
	}
	void Instance::Update( float elapsedTime )
	{
	#if USE_IMGUI
		// Assign the changes as immediately
		AssignRotatedBodies( passDirection, body.pos );
	#endif // USE_IMGUI

		if ( !nowPlaying ) { return; }
		// else

		const auto  &data = FetchParameter();
		const float &motionSpeed = ( nowMotion == Motion::Open ) ? data.animeSpeedOpen : data.animeSpeedClose;

		model.UpdateMotion( elapsedTime * motionSpeed, scast<int>( nowMotion ) );
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

		pRenderer->Render( model.pResource->model, model.GetCurrentPose() );

		pRenderer->DeactivateConstantModel();
	}
#if DEBUG_MODE
	void Instance::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
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
	Definition::Direction Instance::GetThroughDirection() const
	{
		return passDirection;
	}
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
	bool Instance::NowOpenMotion() const
	{
		return nowMotion == Motion::Open;
	}
	bool Instance::NowPlayingAnimation() const
	{
		return nowPlaying;
	}
	void Instance::Open()
	{
		nowOpen = true;
		StartMotion( Motion::Open );
		Donya::Sound::Play( Music::Door_OpenClose );
	}
	void Instance::Close()
	{
		StartMotion( Motion::Close );
		Donya::Sound::Play( Music::Door_OpenClose );
	}
	void Instance::AssignRotatedBodies( Definition::Direction enablePassDir, Donya::Vector3 wsBasePos )
	{
		// The quaternion is made by half radian, so 90deg is cos(45deg).
		constexpr float cos90deg = 0.70710678118655f;
		constexpr float sin90deg = cos90deg;
		constexpr Donya::Quaternion rotPitch270deg{ -sin90deg, +0.00000f, +0.00000f, cos90deg };
		constexpr Donya::Quaternion rotYaw180deg  { +0.00000f, +1.00000f, +0.00000f, 0.00000f };
		constexpr Donya::Quaternion rotRoll90deg  { +0.00000f, +0.00000f, -sin90deg, cos90deg };

		// Assign the default(Right direction) body data
		const auto &data = FetchParameter();
		passDirection	= enablePassDir;
		orientation		= Donya::Quaternion::Identity();
		body			= data.hitBox;
		body.pos		= wsBasePos;
		triggerArea		= data.triggerAreaBase;
		triggerArea.pos	= wsBasePos;
		// Magnify to coming side
		triggerArea.size.x		+= data.triggerAreaAddSize;
		triggerArea.offset.x	-= data.triggerAreaAddSize * 0.5f;

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

			orientation.RotateBy( rotation );
		};

		using Dir = Definition::Direction;

		// Rotate the bodies if the direction is not Right

		if ( passDirection == Dir::Left || passDirection == Dir::Up )
		{
			RotateBodies( rotYaw180deg );

			if ( passDirection == Dir::Up )
			{
				RotateBodies( rotRoll90deg );
			}
		}
		else
		if ( passDirection == Dir::Down )
		{
			RotateBodies( rotRoll90deg );
		}
		else
		// Come here the Right and other(the Nil or a multiple direction)
		// for guarantee the direction has unit direction
		{
			// Enable only uni-direction
			passDirection = Dir::Right;
		}
	}
	void Instance::StartMotion( Motion motionKind )
	{
		nowMotion	= motionKind;
		nowPlaying	= true;
		model.animator.ResetTimer();
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

		const auto oldDirection = passDirection;
		Definition::ShowImGuiNode( u8"通過可能方向",		&passDirection,	/* allowMultipleDirection = */ false );
		if ( passDirection != oldDirection )
		{
			AssignRotatedBodies( passDirection, body.pos );
		}

		ImGui::Helper::ShowFrontNode( u8"現在の前方向",	&orientation	);

		auto GetMotionName = []( Motion kind )->const char *
		{
			switch ( kind )
			{
			case Motion::Open:  return u8"Open";
			case Motion::Close: return u8"Close";
			default: break;
			}

			_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
			return u8"ERROR";
		};
		ImGui::Text( u8"現在のモーション：%s", GetMotionName( nowMotion ) );
		ImGui::Checkbox( u8"モーション再生中か",	&nowPlaying	);
		ImGui::Checkbox( u8"開いているか",		&nowOpen	);

		if ( ImGui::Button( u8"開ける" ) ) { Open(); }
		ImGui::SameLine();
		if ( ImGui::Button( u8"閉める" ) ) { Close(); }


		ImGui::Text( u8"「中心座標」が自機の足元となります" );
		ImGui::Helper::ShowAABBNode( u8"障害判定",		&body			);
		ImGui::Helper::ShowAABBNode( u8"通過判定範囲",	&triggerArea	);

		ImGui::TreePop();
	}
#endif // USE_IMGUI



	void Container::Init( int stageNo )
	{
		LoadParameter( stageNo );
	}
	void Container::Update( float elapsedTime )
	{
		for ( auto &it : doors )
		{
			it.Update( elapsedTime );
		}
	}
	void Container::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer ) { return; }
		// else

		for ( const auto &it : doors )
		{
			it.Draw( pRenderer );
		}
	}
#if DEBUG_MODE
	void Container::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

		for ( const auto &it : doors )
		{
			it.DrawHitBox( pRenderer, VP );
		}
	}
#endif // DEBUG_MODE
	Instance *Container::FetchDetectedDoorOrNullptr( const Donya::Collision::Box3F &other )
	{
		for ( auto &it : doors )
		{
			if ( it.NowOpen() ) { continue; }
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
			if ( it.NowOpen() ) { continue; }
			// else

			bodies.emplace_back( it.GetBody() );
		}

		return std::move( bodies );
	}
	void Container::LoadParameter( int stageNo )
	{
		doors.clear();

	#if DEBUG_MODE
		LoadJson( stageNo );
		// If a user was changed only a json file, the user wanna apply the changes to binary file also.
		// So save here.
		SaveBin( stageNo );
	#else
		LoadBin( stageNo );
	#endif // DEBUG_MODE

		for ( auto &it : doors )
		{
			it.Init();
		}
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
		doors.clear();

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
	size_t Container::GetDoorCount() const
	{
		return doors.size();
	}
	Instance *Container::GetInstanceOrNullptr( size_t index )
	{
		if ( GetDoorCount() <= index ) { return nullptr; }
		// else

		return &doors[index];
	}
	#endif // USE_IMGUI

}
