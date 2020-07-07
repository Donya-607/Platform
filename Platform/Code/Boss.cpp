#include "Boss.h"

#include "Common.h"
#include "Bosses/Skull.h"
#include "FilePath.h"
#include "ModelHelper.h"

#if USE_IMGUI
#include "Map.h"			// Use Map::ToWorldPos()
#include "Parameter.h"
#include "StageFormat.h"
#endif // USE_IMGUI

namespace Boss
{
	namespace
	{
		constexpr size_t kindCount = scast<size_t>( Kind::KindCount );
		constexpr const char *modelFolderName = "Boss/";
		constexpr std::array<const char *, kindCount> modelNames
		{
			"Skull",
		};

		static std::array<std::shared_ptr<ModelHelper::SkinningSet>, kindCount> modelPtrs{ nullptr };

		bool LoadModels()
		{
			const std::string folderName = modelFolderName;
			bool succeeded = true;
			std::string filePath{};
			for ( size_t i = 0; i < kindCount; ++i )
			{
				if ( modelPtrs[i] ) { continue; }
				// else

				filePath = MakeModelPath( folderName + modelNames[i] );

				if ( !Donya::IsExistFile( filePath ) )
				{
					const std::string msg = "Error: File is not found: " + filePath + "\n";
					Donya::OutputDebugStr( msg.c_str() );
					succeeded = false;
					continue;
				}
				// else

				modelPtrs[i] = std::make_shared<ModelHelper::SkinningSet>();
				const bool result = ModelHelper::Load( filePath, modelPtrs[i].get() );
				if ( !result )
				{
					modelPtrs[i].reset(); // Make not loaded state

					const std::string msg = "Failed: Loading failed: " + filePath;
					Donya::OutputDebugStr( msg.c_str() );
					succeeded = false;
					continue;
				}
				// else
			}

			return succeeded;
		}
		constexpr bool IsOutOfRange( Kind kind )
		{
			return ( kindCount <= scast<size_t>( kind ) );
		}
		std::shared_ptr<ModelHelper::SkinningSet> GetModelPtrOrNullptr( Kind kind )
		{
			if ( IsOutOfRange( kind ) ) { return nullptr; }
			// else

			const auto &ptr = modelPtrs[scast<size_t>( kind )];
			if ( !ptr )
			{
				_ASSERT_EXPR( 0, L"Error: The Boss's model is not initialized!" );
				return nullptr;
			}
			// else

			return ptr;
		}
		constexpr const char *GetModelName( Kind kind )
		{
			if ( IsOutOfRange( kind ) ) { return nullptr; }
			// else

			return modelNames[scast<size_t>( kind )];
		}
	}

	namespace Parameter
	{
		void Load()
		{
			Impl::LoadSkull();
		}

	#if USE_IMGUI
		void Update( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			Impl::UpdateSkull( u8"Skull" );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI
	}

	bool LoadResource()
	{
		Parameter::Load();
		return LoadModels();
	}

#if USE_IMGUI
	void InitializeParam::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat3( u8"ワールド座標",	&wsPos.x, 0.01f );
		ImGui::Checkbox  ( u8"右向きか",		&lookingRight );

		ImGui::TreePop();
	}

	std::string Boss::Base::GetStateName( State state )
	{
		switch ( state )
		{
		case Boss::Base::State::Appear:	return u8"登場";
		case Boss::Base::State::Normal:	return u8"通常";
		case Boss::Base::State::Die:	return u8"死亡";
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected state!" );
		return u8"ERROR_STATE";
	}
#endif // USE_IMGUI

	void Base::Init( const InitializeParam &parameter, int belongRoomID, const Donya::Collision::Box3F &wsRoomArea )
	{
		initializer		= parameter;
		roomID			= belongRoomID;
		model.pResource	= GetModelPtrOrNullptr( GetKind() );
		model.animator.ResetTimer();
		AssignMotion( 0 );
		AssignMyBody( parameter.wsPos );
		body.exist		= true;
		hurtBox.exist	= true;
		velocity		= 0.0f;
		hp				= GetInitialHP();
		isDead			= false;
		wantRemove		= false;
		const float rotateSign = ( initializer.lookingRight ) ? 1.0f : -1.0f;
		orientation		= Donya::Quaternion::Make
		(
			Donya::Vector3::Up(), ToRadian( 90.0f ) * rotateSign
		);

		pReceivedDamage.reset();

		AppearInit( wsRoomArea );
	}
	void Base::Uninit()
	{
		pReceivedDamage.reset();
	}
	void Base::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		if ( NowDead() ) { return; }
		// else
		ApplyReceivedDamageIfHas();
	}
	void Base::PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		if ( NowDead() ) { return; }
		// else

		const auto movement = velocity * elapsedTime;
	
		Actor::MoveX( movement.x, solids );
		Actor::MoveZ( movement.z, solids );

		const int collideIndex = Actor::MoveY( movement.y, solids );
		if ( collideIndex != -1 ) // If collided to any
		{
			if ( status == State::Appear )
			{
				TransitionState( State::Normal );
			}

			// Consider as landing
			// if ( velocity.y <= 0.0f )
			// {
			// 	if ( !onGround )
			// 	{
			// 		onGround = true;
			// 	}
			// }

			velocity.y = 0.0f;
		}
		// else
		// {
		// 	onGround = false;
		// }

		hurtBox.pos = body.pos; // We must apply world position to hurt box also.
	}
	void Base::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer			) { return; }
		if ( !model.pResource	) { return; }
		if ( NowDead()			) { return; }
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
	void Base::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
	{
		if ( !pRenderer	) { return; }
		if ( NowDead()	) { return; }
		// else
		
	#if DEBUG_MODE
		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= matVP;
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
	
		constexpr Donya::Vector4 bodyColor{ 0.0f, 1.0f, 0.3f, 0.6f };
		constexpr Donya::Vector4 hurtColor{ 0.0f, 0.3f, 1.0f, 0.6f };
		const auto  &hurt		= hurtBox;
		const float bodyNear	= body.WorldPosition().z - body.size.z;
		const float hurtNear	= hurt.WorldPosition().z - hurt.size.z;
		// Drawing the far box first
		if ( bodyNear < hurtNear )
		{
			DrawProcess( hurt, hurtColor );
			DrawProcess( body, bodyColor );
		}
		else
		{
			DrawProcess( body, bodyColor );
			DrawProcess( hurt, hurtColor );
		}
	#endif // DEBUG_MODE
	}
	bool Base::NowDead() const
	{
		return isDead;
	}
	bool Base::ShouldRemove() const
	{
		return wantRemove;
	}
	int  Base::GetRoomID() const
	{
		return roomID;
	}
	Donya::Collision::Box3F	Base::GetHurtBox() const
	{
		return hurtBox;
	}
	InitializeParam Base::GetInitializer() const
	{
		return initializer;
	}
	void Base::GiveDamage( const Definition::Damage &damage ) const
	{
		if ( !pReceivedDamage )
		{
			pReceivedDamage = std::make_unique<Definition::Damage>();
		}

		pReceivedDamage->Combine( damage );
	}
	void Base::AssignMotion( int motionIndex )
	{
		if ( !model.pResource ) { return; }
		// else

		const int motionCount = scast<int>( model.pResource->motionHolder.GetMotionCount() );
		if ( motionIndex < 0 || motionCount <= motionIndex )
		{
			_ASSERT_EXPR( 0, L"Error: Passed motion index is out of range!" );
			return;
		}
		// else

		const auto &motion = model.pResource->motionHolder.GetMotion( motionIndex );
		model.pose.AssignSkeletal( model.animator.CalcCurrentPose( motion ) );
	}
	void Base::UpdateMotion( float elapsedTime, int motionIndex )
	{
		if ( !model.pResource ) { return; }
		// else

		model.animator.Update( elapsedTime );
		AssignMotion( motionIndex );
	}
	void Base::ApplyReceivedDamageIfHas()
	{
		if ( !pReceivedDamage ) { return; }
		// else

		hp -= pReceivedDamage->amount;
		if ( hp <= 0 )
		{
			isDead = true;
			DieMoment();
		}

		pReceivedDamage.reset();
	}
	void Base::AppearInit( const Donya::Collision::Box3F &wsRoomArea )
	{
		// Make the foot pos places the top of room.
		// The X, Z component is not change.
		Donya::Vector3 topPos = body.pos;
		topPos.y = wsRoomArea.Max().y + body.size.y;
		AssignMyBody( topPos );

		// Deactivate the collision for do not correct to outside room.
		// This will be activated at AppearUpdate().
		body.exist		= false;
		hurtBox.exist	= false;

		// Enter to the room by gravity
		velocity		= 0.0f;
	}
	void Base::AppearUpdate( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsRoomArea )
	{
		// Re-activate the collision
		if ( !body.exist )
		{
			// For now, I regard as the body is there within the room
			// if the top(head) position places under the one block size.

			const float border	= wsRoomArea.Max().y - Tile::unitWholeSize;
			const float topPos	= body.Max().y;
			if ( topPos < border )
			{
				body.exist		= true;
				hurtBox.exist	= true;
			}
		}

		constexpr float lowestGravity = 1.0f;
		const float applyGravity = std::max( lowestGravity, GetGravity() );
		velocity.y -= applyGravity * elapsedTime;
	}
	Donya::Vector4x4 Base::MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const
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
	bool Base::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else
		
		if ( ImGui::Button( ( nodeCaption + u8"を削除する" ).c_str() ) )
		{
			wantRemove = true;
		}
		
		initializer.ShowImGuiNode( u8"初期化パラメータ" );
		ImGui::Text( u8"現在のステート：%s", GetStateName( status ).c_str() );

		ImGui::DragFloat3( u8"ワールド座標", &body.pos.x, 0.01f );
		ImGui::Helper::ShowFrontNode( u8"前方向", &orientation );

		ImGui::Checkbox( u8"死んでいるか", &isDead );

		ImGui::TreePop();
		return true;
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

	bool Container::Init( int stageNumber )
	{
		const bool succeeded = LoadBosses( stageNumber, IOFromBinaryFile );

	#if DEBUG_MODE
		// If a user was changed only a json file, the user wanna apply the changes to binary file also.
		// So save here.
		SaveBosses( stageNumber, /* fromBinary = */ true );

		return true;
	#endif // DEBUG_MODE

		return succeeded;
	}
	void Container::Uninit()
	{
		ClearAllBosses();
	}
	void Container::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos )
	{
		for ( auto &it : bosses )
		{
			if ( it.pBoss ) { it.pBoss->Update( elapsedTime, wsTargetPos, it.roomArea ); }
		}

		RemoveBosses();
	}
	void Container::PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		for ( auto &it : bosses )
		{
			if ( it.pBoss ) { it.pBoss->PhysicUpdate( elapsedTime, solids ); }
		}
	}
	void Container::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer ) { return; }
		// else

		for ( const auto &it : bosses )
		{
			if ( it.pBoss ) { it.pBoss->Draw( pRenderer ); }
		}
	}
	void Container::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

		for ( const auto &it : bosses )
		{
			if ( it.pBoss ) { it.pBoss->DrawHitBox( pRenderer, matVP ); }
		}
	}
	bool Container::IsThereIn( int roomID ) const
	{
		for ( const auto &it : bosses )
		{
			if ( it.roomID == roomID )
			{
				return true;
			}
		}

		return false;
	}
	bool Container::IsAliveIn( int roomID ) const
	{
		// This method requires IsThereIn() is true,
		// but this method also contain that process.

		for ( const auto &it : bosses )
		{
			if ( it.pBoss && it.pBoss->GetRoomID() == roomID ) // == IsThereIn()
			{
				if ( !it.pBoss->NowDead() )
				{
					return true;
				}
			}
		}

		return false;
	}
	void Container::StartupBossIfStandby( int roomID )
	{
		const size_t count = GetBossCount();
		for ( size_t i = 0; i < count; ++i )
		{
			if ( bosses[i].roomID == roomID && !bosses[i].pBoss )
			{
				AppearBoss( i );
				return;
			}
		}
	}
	size_t Container::GetBossCount() const
	{
		return bosses.size();
	}
	bool Container::LoadBosses( int stageNumber, bool fromBinary )
	{
		ClearAllBosses();

		const std::string filePath	= ( fromBinary )
									? MakeStageParamPathBinary( ID, stageNumber )
									: MakeStageParamPathJson  ( ID, stageNumber );
		const bool succeeded = Donya::Serializer::Load( *this, filePath.c_str(), ID, fromBinary );

		// I should call Init()
		for ( auto &it : bosses )
		{
			if ( it.pBoss ) { it.pBoss->Init( it.initializer, it.roomID, it.roomArea ); }
		}

		return succeeded;
	}
	void Container::AppearBoss( size_t appearIndex )
	{
		if ( bosses.size() <= appearIndex ) { return; }
		// else

		auto &set = bosses[appearIndex];
		if ( set.pBoss )
		{
			set.pBoss->Uninit();
			set.pBoss.reset();
		}

		switch ( set.kind )
		{
		case Kind::Skull:	set.pBoss = std::make_shared<Boss::Skull>();	break;
		default: _ASSERT_EXPR( 0, L"Error: That boss kind is invalid!" );	return;
		}

		set.pBoss->Init
		(
			set.initializer,
			set.roomID,
			set.roomArea
		);
	}
	void Container::RemoveBosses()
	{
		// Only reset the shared_ptr.
		for ( auto &it : bosses )
		{
			if ( it.pBoss && it.pBoss->ShouldRemove() )
			{
				it.pBoss->Uninit();
				it.pBoss.reset();
			}
		}
	}
	void Container::ClearAllBosses()
	{
		for ( auto &it : bosses )
		{
			if ( it.pBoss )
			{
				it.pBoss->Uninit();
				it.pBoss.reset();
			}
		}
	}
#if USE_IMGUI
	void Container::AppendBoss( int roomID, const Donya::Collision::Box3F &wsRoomArea, Kind kind, const InitializeParam &parameter )
	{
		const int intKind = scast<int>( kind );
		if ( intKind < 0 || scast<int>( Kind::KindCount ) <= intKind )
		{
			_ASSERT_EXPR( 0, L"Error: That boss kind is invalid!" );
			return;
		}
		// else

		BossSet tmp{};
		tmp.roomID		= roomID;
		tmp.kind		= kind;
		tmp.initializer	= parameter;
		tmp.roomArea	= wsRoomArea;
		tmp.pBoss		= nullptr;
		bosses.emplace_back( std::move( tmp ) );
	}
	Container::BossSet Container::GetBossOrNullptr( size_t instanceIndex )
	{
		if ( GetBossCount() <= instanceIndex ) { return {}; }
		// else

		return bosses[instanceIndex];
	}
	void Container::RemakeByCSV( const CSVLoader &loadedData, const House &house )
	{
		auto IsBossID	= []( int id )
		{
			return ( StageFormat::BossStart <= id && id <= StageFormat::BossLast );
		};
		auto ToKind		= []( int id )
		{
			id -= StageFormat::BossStart;
			if ( id < 0 ) { id = INT_MAX; } // Make invalid number
			return scast<Kind>( id );
		};
		auto IsValidKind= []( Kind kind )
		{
			const int intID = scast<int>( kind );
			return ( 0 <= intID && intID < scast<int>( Kind::KindCount ) );
		};
		auto Append		= [&]( int id, size_t row, size_t column )
		{
			if ( !IsBossID( id ) ) { return; }
			// else

			Kind kind = ToKind( id );

			if ( !IsValidKind( kind ) )
			{
				std::string msg = u8"実装されていないボスの番号を検出しました。\n";
				msg += u8"行:" + std::to_string( row    ) + u8", ";
				msg += u8"列:" + std::to_string( column ) + u8", ";
				msg += u8"ID:" + std::to_string( id     ) + u8".\n";

				Donya::ShowMessageBox
				(
					msg, "Not registered",
					MB_ICONEXCLAMATION | MB_OK
				);
				return;
			}
			// else

			InitializeParam tmp;
			tmp.lookingRight	= false;
			tmp.wsPos			= Map::ToWorldPos( row, column );

			const int roomID	= house.CalcBelongRoomID( tmp.wsPos );
			if ( roomID == Room::invalidID )
			{
				std::string msg = u8"ボスがルーム内に属していません！\n";
				msg += u8"行:" + std::to_string( row    ) + u8", ";
				msg += u8"列:" + std::to_string( column ) + u8", ";
				msg += u8"ID:" + std::to_string( id     ) + u8".\n";

				Donya::ShowMessageBox
				(
					msg, "Not registered",
					MB_ICONEXCLAMATION | MB_OK
				);
				return;
			}
			// else

			AppendBoss( roomID, house.CalcRoomArea( roomID ), kind, tmp );
		};

		ClearAllBosses();

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
	void Container::SaveBosses( int stageNumber, bool fromBinary )
	{
		const std::string filePath	= ( fromBinary )
									? MakeStageParamPathBinary( ID, stageNumber )
									: MakeStageParamPathJson  ( ID, stageNumber );
		MakeDirectoryIfNotExists( filePath );
		MakeFileIfNotExists( filePath, fromBinary );

		Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
	}
	void Container::ShowImGuiNode( const std::string &nodeCaption, int stageNo )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::TreeNode( u8"実体たち" ) )
		{
			const size_t count = GetBossCount();
			for ( size_t i = 0; i < count; ++i )
			{
				ShowInstanceNode( i );
			}

			ImGui::TreePop();
		}

		const auto result = ParameterHelper::ShowIONode();
		using Op = ParameterHelper::IOOperation;
		if ( result == Op::Save )
		{
			SaveBosses( stageNo, true  );
			SaveBosses( stageNo, false );
		}
		else if ( result == Op::LoadBinary )
		{
			LoadBosses( stageNo, true );
		}
		else if ( result == Op::LoadJson )
		{
			LoadBosses( stageNo, false );
		}

		ImGui::TreePop();
	}
	void Container::ShowInstanceNode( size_t instanceIndex )
	{
		auto boss = GetBossOrNullptr( instanceIndex );
		std::string caption = "[RoomID:" + std::to_string( boss.roomID ) + "]";

		if ( !boss.pBoss )
		{
			caption += " is not exist.";
			ImGui::TextDisabled( caption.c_str() );
			return;
		}
		// else

		bool  treeIsOpen = boss.pBoss->ShowImGuiNode( caption );
		if ( !treeIsOpen )
		{
			caption = "[";
			caption += GetModelName( boss.pBoss->GetKind() );
			caption += "]";
			ImGui::SameLine();
			ImGui::Text( caption.c_str() );
		}
	}
#endif // USE_IMGUI
}
