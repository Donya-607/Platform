#include "Enemy.h"

#include <array>

#include "Common.h"
#include "ModelHelper.h"
#include "Terry.h"
#if USE_IMGUI
#include "Parameter.h"
#endif // USE_IMGUI

namespace Enemy
{
	namespace
	{
		constexpr size_t kindCount = scast<size_t>( Kind::KindCount );
		constexpr const char *modelFolderName = "Enemy/";
		constexpr std::array<const char *, kindCount> modelNames
		{
			"Terry",
		};

		static std::array<std::unique_ptr<ModelHelper::StaticSet>, kindCount> modelPtrs{ nullptr };

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

				modelPtrs[i] = std::make_unique<ModelHelper::StaticSet>();
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
		const ModelHelper::StaticSet *GetModelPtrOrNullptr( Kind kind )
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

			return ptr.get();
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
			Impl::LoadTerry();
		}

	#if USE_IMGUI
		void Update( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			Impl::UpdateTerry( u8"Terry" );

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
		AssignMyBody( parameter.wsPos );
		body.exist		= true;
		hurtBox.exist	= true;
		velocity		= 0.0f;
		hp				= GetInitialHP();
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
		// Back to initialize pos for respawn
		body.pos		= initializer.wsPos;
		hurtBox.pos		= initializer.wsPos;
		body.exist		= false;
		hurtBox.exist	= false;
	}
	void Base::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		ApplyReceivedDamageIfHas();
	}
	void Base::PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		if ( NowWaiting() ) { return; }
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
		if ( !pRenderer   ) { return; }
		if ( NowWaiting() ) { return; }
		// else

		const ModelHelper::StaticSet *pModelSet = GetModelPtrOrNullptr( GetKind() );
		if ( !pModelSet ) { return; }
		// else

		const auto model = *pModelSet;

		const Donya::Vector3 &drawPos = body.pos; // I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
		const Donya::Vector4x4 W = MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );

		Donya::Model::Constants::PerModel::Common modelConstant{};
		modelConstant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		modelConstant.worldMatrix	= W;;
		pRenderer->UpdateConstant( modelConstant );
		pRenderer->ActivateConstantModel();

		pRenderer->Render( model.model, model.pose );

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
	}
	void Base::RespawnIfSpawnable()
	{
		if ( !NowWaiting() ) { return; }
		if ( onOutSideCurrent || !onOutSidePrevious ) { return;  }
		// else
		waitForRespawn = false;
		Init( GetInitializer() );
	}
	void Base::ApplyReceivedDamageIfHas()
	{
		if ( !pReceivedDamage ) { return; }
		// else

		hp -= pReceivedDamage->amount;
		if ( hp <= 0 )
		{
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
		ImGui::DragFloat3( u8"ワールド座標", &body.pos.x, 0.01f );
		ImGui::Helper::ShowFrontNode( "", &orientation );
		ImGui::Text( u8"画面内にいる：%d", ( onOutSideCurrent ) ? 0 : 1 );
		ImGui::Text( u8"リスポーン待ち：%d", ( waitForRespawn ) ? 1 : 0 );
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
	}
	void Admin::PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		for ( auto &pIt : enemyPtrs )
		{
			if ( pIt ) { pIt->PhysicUpdate( elapsedTime, solids ); }
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
	bool Admin::LoadEnemies( int stageNumber, bool fromBinary )
	{
		ClearInstances();

		const std::string filePath	= ( fromBinary )
									? MakeStageParamPathBinary( ID, stageNumber )
									: MakeStageParamPathJson  ( ID, stageNumber );
		const bool succeeded = Donya::Serializer::Load( *this, filePath.c_str(), ID, fromBinary );

		// I should call Init()
		for ( auto &pIt : enemyPtrs )
		{
			if ( pIt ) { pIt->Init( pIt->GetInitializer() ); }
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
	void Admin::RemoveEnemies()
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
	void Admin::RemakeByCSV( const CSVLoader &loadedData )
	{

	}
	void Admin::AppendEnemy( Kind kind, const InitializeParam &parameter )
	{
		std::shared_ptr<Base> instance = nullptr;

		switch ( kind )
		{
		case Kind::Terry:	instance = std::make_shared<Enemy::Terry>();	break;
		default: break;
		}

		if ( !instance )
		{
			_ASSERT_EXPR( 0, L"Error: That enemy kind is invalid!" );
			return;
		}
		// else

		instance->Init( parameter );
		enemyPtrs.emplace_back( std::move( instance ) );
	}
	void Admin::SaveEnemies( int stageNumber, bool fromBinary )
	{
		const std::string filePath	= ( fromBinary )
									? MakeStageParamPathBinary( ID, stageNumber )
									: MakeStageParamPathJson  ( ID, stageNumber );
		MakeDirectoryIfNotExists( filePath );
		MakeFileIfNotExists( filePath, fromBinary );

		Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
	}
	void Admin::ShowImGuiNode( const std::string &nodeCaption, int stageNo )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::Button( u8"Terryを追加" ) )
		{
			InitializeParam tmp;
			tmp.lookingRight = true;
			tmp.wsPos = Donya::Vector3::Zero();
			AppendEnemy( Kind::Terry, tmp );
		}

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
			LoadEnemies( stageNo, true );
		}
		else if ( result == Op::LoadJson )
		{
			LoadEnemies( stageNo, false );
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
