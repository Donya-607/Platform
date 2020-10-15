#include "Enemy.h"

#include <array>

#if USE_IMGUI
#include "Donya/Useful.h"	// Use ShowMessageBox()
#endif // USE_IMGUI

#include "Common.h"
#include "Effect/EffectAdmin.h"
#include "Enemies/SuperBallMachine.h"
#include "Enemies/Togehero.h"
#include "FilePath.h"
#include "ModelHelper.h"
#if USE_IMGUI
#include "Map.h"			// Use ToWorldPos()
#include "Parameter.h"
#include "StageFormat.h"
#endif // USE_IMGUI

namespace Enemy
{
	namespace
	{
		constexpr size_t kindCount = scast<size_t>( Kind::KindCount );
		constexpr const char *modelFolderName = "Enemy/";
		constexpr std::array<const char *, kindCount> modelNames
		{
			"SuperBallMachine",
			"Togehero",
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
				_ASSERT_EXPR( 0, L"Error: The Enemy's model is not initialized!" );
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
			Impl::LoadTogehero();
			Impl::LoadSuperBallMachine();
		}

	#if USE_IMGUI
		void Update( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			Impl::UpdateTogehero		( u8"Togehero" );
			Impl::UpdateSuperBallMachine( u8"SuperBallMachine" );

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

		ImGui::Text( u8"向き：" );
		auto ShowRadioButton = [&]( const char *caption, LookDirection dir )
		{
			if ( ImGui::RadioButton( caption, lookDirection == dir ) )
			{
				lookDirection = dir;
			}
		};
		ShowRadioButton( u8"自機を見る",	LookDirection::ToTarget	);
		ShowRadioButton( u8"右向き",		LookDirection::Right	);
		ShowRadioButton( u8"左向き",		LookDirection::Left		);

		ImGui::TreePop();
	}
	void BasicParam::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		touchDamage.ShowImGuiNode( u8"接触ダメージ設定" );
		ImGui::DragInt   ( u8"初期ＨＰ",							&hp							);
		ImGui::DragFloat3( u8"当たり判定・オフセット",			&hitBoxOffset.x,	0.01f	);
		ImGui::DragFloat3( u8"当たり判定・サイズ（半分を指定）",	&hitBoxSize.x,		0.01f	);
		ImGui::DragFloat3( u8"喰らい判定・オフセット",			&hurtBoxOffset.x,	0.01f	);
		ImGui::DragFloat3( u8"喰らい判定・サイズ（半分を指定）",	&hurtBoxSize.x,		0.01f	);
		hp				= std::max( 1,		hp				);
		hitBoxSize.x	= std::max( 0.0f,	hitBoxSize.x	);
		hitBoxSize.y	= std::max( 0.0f,	hitBoxSize.y	);
		hitBoxSize.z	= std::max( 0.0f,	hitBoxSize.z	);
		hurtBoxSize.x	= std::max( 0.0f,	hurtBoxSize.x	);
		hurtBoxSize.y	= std::max( 0.0f,	hurtBoxSize.y	);
		hurtBoxSize.z	= std::max( 0.0f,	hurtBoxSize.z	);

		ImGui::TreePop();
	}
#endif // USE_IMGUI

	void Base::Init( const InitializeParam &parameter, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		initializer		= parameter;

		model.Initialize( GetModelPtrOrNullptr( GetKind() ) );
		model.AssignMotion( 0 );
		AssignMyBody( initializer.wsPos );
		body.exist		= true;
		hurtBox.exist	= true;
		hurtBox.id		= Donya::Collision::GetUniqueID();
		hurtBox.ownerID	= Donya::Collision::invalidID;
		hurtBox.ignoreList.clear();
		velocity		= 0.0f;
		hp				= GetInitialHP();
		wantRemove		= false;

		float rotateSign = 1.0f;
		switch ( initializer.lookDirection )
		{
		case InitializeParam::LookDirection::ToTarget:
			{
				const auto dir = ( wsTargetPos - initializer.wsPos ).x;
				rotateSign = ( Donya::SignBit( dir ) < 0 ) ? -1.0f : 1.0f;
			}
			break;
		case InitializeParam::LookDirection::Right:
			rotateSign = +1.0f;
			break;
		case InitializeParam::LookDirection::Left:
			rotateSign = -1.0f;
			break;
		default: break;
		}
		orientation		= Donya::Quaternion::Make
		(
			Donya::Vector3::Up(), ToRadian( 90.0f ) * rotateSign
		);

		pReceivedDamage.reset();

		UpdateOutSideState( wsScreen );
		onOutSidePrevious = onOutSideCurrent;
		if ( onOutSideCurrent )
		{
			BeginWaitIfActive();
		}
	}
	void Base::Uninit()
	{
		// Back to initialize pos for respawn
		body.pos		= initializer.wsPos;
		hurtBox.pos		= initializer.wsPos;
		body.exist		= false;
		hurtBox.exist	= false;
		body.ignoreList.clear();
		hurtBox.ignoreList.clear();

		pReceivedDamage.reset();
	}
	void Base::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		// Update wait/alive state

		UpdateOutSideState( wsScreen );
		const bool nowWaiting = NowWaiting();
		const bool onOutSide  = OnOutSide();
		if ( nowWaiting != onOutSide )
		{
			if ( !nowWaiting && onOutSide )
			{
				BeginWaitIfActive();
			}
			else // if ( nowWaiting && !onOutSide )
			{
				RespawnIfSpawnable( wsTargetPos );
			}

			return;
		}
		// else


		// Normal update processes

		hurtBox.UpdateIgnoreList( elapsedTime );

		ApplyReceivedDamageIfHas();
	}
	void Base::PhysicUpdate( float elapsedTime, const Map &terrain )
	{
		if ( NowWaiting() ) { return; }
		// else

		const auto myBody		= GetHitBox();
		const auto movement		= velocity * elapsedTime;
		const auto aroundTiles	= terrain.GetPlaceTiles( myBody, movement );
		const auto aroundSolids	= Map::ToAABBSolids( aroundTiles, terrain, myBody );
		Actor::MoveX( movement.x, aroundSolids );
		Actor::MoveZ( movement.z, aroundSolids );

		const int collideIndex = Actor::MoveY( movement.y, aroundSolids );
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
		if ( NowWaiting()		) { return; }
		// else

		const Donya::Vector3 &drawPos = body.pos; // I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
		const Donya::Vector4x4 W = MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );

		Donya::Model::Constants::PerModel::Common modelConstant{};
		modelConstant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		modelConstant.worldMatrix	= W;
		pRenderer->UpdateConstant( modelConstant );
		pRenderer->ActivateConstantModel();

		pRenderer->Render( model.pResource->model, model.pose );

		pRenderer->DeactivateConstantModel();
	}
	void Base::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
	{
		if ( NowWaiting() ) { return; }
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
		const auto  body		= GetHitBox();
		const auto  hurt		= GetHurtBox();
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
			pReceivedDamage = std::make_shared<Definition::Damage>();
		}

		pReceivedDamage->Combine( damage );
	}
	bool Base::WillDie() const
	{
		if ( !pReceivedDamage ) { return false; }

		return ( hp - pReceivedDamage->amount <= 0 );
	}
	void Base::UpdateMotionIfCan( float elapsedTime, int motionIndex )
	{
		if ( !model.IsAssignableIndex( motionIndex ) ) { return; }
		// else

		model.UpdateMotion( elapsedTime, motionIndex );
	}
	void Base::UpdateOutSideState( const Donya::Collision::Box3F &wsScreen )
	{
		onOutSidePrevious = onOutSideCurrent;
		onOutSideCurrent  = ( !Donya::Collision::IsHit( body, wsScreen, /* considerExistFlag = */ false ) );
	}
	bool Base::OnOutSide() const
	{
		return onOutSideCurrent;
	}
	bool Base::NowWaiting() const
	{
		return waitForRespawn;
	}
	void Base::BeginWaitIfActive()
	{
		if ( NowWaiting() ) { return; }
		// else
		waitForRespawn = true;
		Uninit();

		// Prevent doing respawn as immediately if the waiting is occurred by leave outside
		onOutSidePrevious = false;
		onOutSideCurrent  = false;
	}
	void Base::RespawnIfSpawnable( const Donya::Vector3 &wsTargetPos )
	{
		if ( !NowWaiting() ) { return; }
		if ( onOutSideCurrent || !onOutSidePrevious ) { return; }
		// else
		waitForRespawn = false;

		// The "wsScreenHitBox" will be used for consideration to go to wait.
		// So I make to do not wait as certainly by passing my body.
		Init( GetInitializer(), wsTargetPos, GetHitBox() );
	}
	void Base::ApplyReceivedDamageIfHas()
	{
		if ( !pReceivedDamage ) { return; }
		// else

		hp -= pReceivedDamage->amount;
		if ( hp <= 0 )
		{
			// Generate the effect before resetting the position
			Effect::Admin::Get().GenerateInstance( Effect::Kind::DefeatEnemy_Small, GetPosition() );

			BeginWaitIfActive();
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
		ImGui::DragInt		( u8"現在ＨＰ",				&hp );
		ImGui::DragFloat3	( u8"ワールド座標",			&body.pos.x, 0.01f );
		ImGui::Helper::ShowFrontNode( "", &orientation );
		ImGui::Text			( u8"画面内にいる：%d",		( onOutSideCurrent	) ? 0 : 1 );
		ImGui::Text			( u8"リスポーン待ち：%d",		( waitForRespawn	) ? 1 : 0 );
		if ( ImGui::Button( u8"リスポーン待ち状態にする" ) )
		{
			BeginWaitIfActive();
		}

		ImGui::TreePop();
		return true;
	}
#endif // USE_IMGUI


	void Admin::Uninit()
	{
		for ( auto &pIt : enemyPtrs )
		{
			if ( pIt ) { pIt->Uninit(); }
		}
	}
	void Admin::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		for ( auto &pIt : enemyPtrs )
		{
			if ( pIt ) { pIt->Update( elapsedTime, wsTargetPos, wsScreen ); }
		}

		RemoveEnemiesIfNeeded();
	}
	void Admin::PhysicUpdate( float elapsedTime, const Map &terrain )
	{
		for ( auto &pIt : enemyPtrs )
		{
			if ( pIt ) { pIt->PhysicUpdate( elapsedTime, terrain ); }
		}
	}
	void Admin::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer ) { return; }
		// else

		for ( const auto &pIt : enemyPtrs )
		{
			if ( pIt ) { pIt->Draw( pRenderer ); }
		}
	}
	void Admin::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

		for ( const auto &pIt : enemyPtrs )
		{
			if ( pIt ) { pIt->DrawHitBox( pRenderer, matVP ); }
		}
	}
	void Admin::ClearInstances()
	{
		for ( auto &pIt : enemyPtrs )
		{
			if ( pIt ) { pIt->Uninit(); }
		}
		enemyPtrs.clear();
	}
	bool Admin::LoadEnemies( int stageNumber, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen, bool fromBinary )
	{
		ClearInstances();

		const std::string filePath	= ( fromBinary )
									? MakeStageParamPathBinary( ID, stageNumber )
									: MakeStageParamPathJson  ( ID, stageNumber );
		Donya::Serializer tmp;
		const bool succeeded		= ( fromBinary )
									? tmp.LoadBinary( *this, filePath.c_str(), ID )
									: tmp.LoadJSON	( *this, filePath.c_str(), ID );

		// I should call Init()
		for ( auto &pIt : enemyPtrs )
		{
			if ( pIt ) { pIt->Init( pIt->GetInitializer(), wsTargetPos, wsScreen ); }
		}

		return succeeded;
	}
	size_t Admin::GetInstanceCount() const
	{
		return enemyPtrs.size();
	}
	bool Admin::IsOutOfRange( size_t instanceIndex ) const
	{
		return ( GetInstanceCount() <= instanceIndex ) ? true : false;
	}
	std::shared_ptr<const Base> Admin::GetInstanceOrNullptr( size_t instanceIndex ) const
	{
		if ( IsOutOfRange( instanceIndex ) ) { return nullptr; }
		// else
		return enemyPtrs[instanceIndex];
	}
	void Admin::RemoveEnemiesIfNeeded()
	{
		auto itr = std::remove_if
		(
			enemyPtrs.begin(), enemyPtrs.end(),
			[]( std::shared_ptr<Base> &element )
			{
				return ( element ) ? element->ShouldRemove() : true;
			}
		);
		enemyPtrs.erase( itr, enemyPtrs.end() );
	}
#if USE_IMGUI
	void Admin::AppendEnemy( Kind kind, const InitializeParam &parameter, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		std::shared_ptr<Base> instance = nullptr;

		switch ( kind )
		{
		case Kind::SuperBallMachine:instance = std::make_shared<Enemy::SuperBallMachine>();	break;
		case Kind::Togehero:		instance = std::make_shared<Enemy::Togehero>();			break;
		// case Kind::SkeletonJoe:		instance = std::make_shared<Enemy::>();	break;
		// case Kind::ShieldAttacker:	instance = std::make_shared<Enemy::>();	break;
		// case Kind::Battonton:		instance = std::make_shared<Enemy::>();	break;
		// case Kind::SkullMet:		instance = std::make_shared<Enemy::>();	break;
		// case Kind::Imorm:			instance = std::make_shared<Enemy::>();	break;
		default: break;
		}

		if ( !instance )
		{
			_ASSERT_EXPR( 0, L"Error: That enemy kind is invalid!" );
			return;
		}
		// else

		instance->Init( parameter, wsTargetPos, wsScreen );
		enemyPtrs.emplace_back( std::move( instance ) );
	}
	void Admin::RemakeByCSV( const CSVLoader &loadedData, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		auto IsEnemyID	= []( int id )
		{
			return ( StageFormat::EnemyStart <= id && id <= StageFormat::EnemyLast );
		};
		auto IsValidID	= []( int id )
		{
			id -= StageFormat::EnemyStart;
			return ( 0 <= id && id < scast<int>( Kind::KindCount ) );
		};
		auto Append		= [&]( int id, size_t row, size_t column )
		{
			if ( !IsEnemyID( id ) ) { return; }
			// else

			if ( !IsValidID( id ) )
			{
				std::string msg = u8"実装されていない敵の番号を検出しました。\n";
				msg += u8"行:" + std::to_string( row    ) + u8", ";
				msg += u8"列:" + std::to_string( column ) + u8", ";
				msg += u8"ID:" + std::to_string( id     ) + u8".\n";

				Donya::ShowMessageBox
				(
					msg, "Announce",
					MB_ICONEXCLAMATION | MB_OK
				);
				return;
			}
			// else

			InitializeParam tmp;
			tmp.lookDirection	= InitializeParam::LookDirection::ToTarget;
			tmp.wsPos			= Map::ToWorldPos( row, column );
			const Kind kind = scast<Kind>( id - StageFormat::EnemyStart );
			AppendEnemy( kind, tmp, wsTargetPos, wsScreen );
		};

		for ( auto &pIt : enemyPtrs )
		{
			if ( pIt ) { pIt->Uninit(); }
		}
		enemyPtrs.clear();

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
	void Admin::SaveEnemies( int stageNumber, bool fromBinary )
	{
		const std::string filePath	= ( fromBinary )
									? MakeStageParamPathBinary( ID, stageNumber )
									: MakeStageParamPathJson  ( ID, stageNumber );
		MakeDirectoryIfNotExists( filePath );
		MakeFileIfNotExists( filePath, fromBinary );

		Donya::Serializer tmp;
		( fromBinary )
		? tmp.SaveBinary( *this, filePath.c_str(), ID )
		: tmp.SaveJSON	( *this, filePath.c_str(), ID );
	}
	void Admin::ShowImGuiNode( const std::string &nodeCaption, int stageNo, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::TreeNode( u8"実体たち" ) )
		{
			const size_t enemyCount = enemyPtrs.size();

			for ( size_t i = 0; i < enemyCount; ++i )
			{
				ShowInstanceNode( i );
			}

			ImGui::TreePop();
		}

		const auto result = ParameterHelper::ShowIONode();
		using Op = ParameterHelper::IOOperation;
		if ( result == Op::Save )
		{
			SaveEnemies( stageNo, true  );
			SaveEnemies( stageNo, false );
		}
		else if ( result == Op::LoadBinary )
		{
			LoadEnemies( stageNo, wsTargetPos, wsScreen, true );
		}
		else if ( result == Op::LoadJson )
		{
			LoadEnemies( stageNo, wsTargetPos, wsScreen, false );
		}

		ImGui::TreePop();
	}
	void Admin::ShowInstanceNode( size_t index )
	{
		if ( IsOutOfRange( index ) ) { return; }
		// else

		auto &pEnemy = enemyPtrs[index];
		if ( !pEnemy ) { return; }
		// else

		std::string caption = Donya::MakeArraySuffix( index );
		bool  treeIsOpen = pEnemy->ShowImGuiNode( caption );
		if ( !treeIsOpen )
		{
			caption = "[";
			caption += GetModelName( pEnemy->GetKind() );
			caption += "]";
			ImGui::SameLine();
			ImGui::Text( caption.c_str() );
		}
	}
#endif // USE_IMGUI
}
