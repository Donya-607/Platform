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
#endif // USE_IMGUI

	void Base::Init( const InitializeParam &parameter )
	{
		initializer		= parameter;
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
	}
	void Base::Uninit()
	{
		pReceivedDamage.reset();
	}
	void Base::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos )
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
		ImGui::DragFloat3( u8"ワールド座標", &body.pos.x, 0.01f );
		ImGui::Helper::ShowFrontNode( "", &orientation );
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
		for ( auto &it : bossPtrs )
		{
			if ( it.second ) { it.second->Update( elapsedTime, wsTargetPos ); }
		}
	}
	void Container::PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		for ( auto &it : bossPtrs )
		{
			if ( it.second ) { it.second->PhysicUpdate( elapsedTime, solids ); }
		}
	}
	void Container::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer ) { return; }
		// else

		for ( const auto &it : bossPtrs )
		{
			if ( it.second ) { it.second->Draw( pRenderer ); }
		}
	}
	void Container::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

		for ( const auto &it : bossPtrs )
		{
			if ( it.second ) { it.second->DrawHitBox( pRenderer, matVP ); }
		}
	}
	bool Container::LoadBosses( int stageNumber, bool fromBinary )
	{
		ClearAllBosses();

		const std::string filePath	= ( fromBinary )
									? MakeStageParamPathBinary( ID, stageNumber )
									: MakeStageParamPathJson  ( ID, stageNumber );
		const bool succeeded = Donya::Serializer::Load( *this, filePath.c_str(), ID, fromBinary );

		// I should call Init()
		for ( auto &it : bossPtrs )
		{
			if ( it.second ) { it.second->Init( it.second->GetInitializer() ); }
		}

		return succeeded;
	}
	void Container::RemoveBosses()
	{
		// An erase-remove idiom is can not use for unordered_map.
		// So I do simple loop.
		for ( auto it = bossPtrs.begin(); it != bossPtrs.end(); )
		{
			if ( !it->second || it->second->ShouldRemove() )
			{
				it = bossPtrs.erase( it );
			}
			else
			{
				++it;
			}
		}
	}
	void Container::ClearAllBosses()
	{
		for ( auto &it : bossPtrs )
		{
			if ( it.second )
			{
				it.second->Uninit();
				it.second.reset();
			}
		}
	}
#if USE_IMGUI
	void Container::AddBoss( Kind kind, const InitializeParam &parameter, int roomID )
	{
		std::unique_ptr<Base> instance = nullptr;

		switch ( kind )
		{
		case Kind::Skull:	instance = std::make_unique<Boss::Skull>();	break;
		default: break;
		}

		if ( !instance )
		{
			_ASSERT_EXPR( 0, L"Error: That boss kind is invalid!" );
			return;
		}
		// else

		instance->Init( parameter );
		bossPtrs.insert
		(
			std::make_pair( roomID, std::move( instance ) )
		);
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

			AddBoss( kind, tmp, roomID );
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
			for ( auto &it : bossPtrs )
			{
				if ( !it.second ) { continue; }
				// else
				ShowInstanceNode( it.first );
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
	void Container::ShowInstanceNode( int roomID )
	{
		auto find =  bossPtrs.find( roomID );
		if ( find == bossPtrs.end() ) { return; }
		// else

		auto &pBoss = find->second;
		if ( !pBoss ) { return; }
		// else

		std::string caption = "[RoomID:" + std::to_string( roomID ) + "]";
		bool  treeIsOpen = pBoss->ShowImGuiNode( caption );
		if ( !treeIsOpen )
		{
			caption = "[";
			caption += GetModelName( pBoss->GetKind() );
			caption += "]";
			ImGui::SameLine();
			ImGui::Text( caption.c_str() );
		}
	}
#endif // USE_IMGUI
}
