#include "SceneGame.h"

#include <algorithm>				// Use std::find
#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Blend.h"
#include "Donya/Color.h"			// Use ClearBackGround(), StartFade().
#include "Donya/Keyboard.h"			// Make an input of player.
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Template.h"			// Use Clamp
#include "Donya/Useful.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/GeometricPrimitive.h"
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Bloom.h"
#include "Bosses/Skull.h"			// Use SkullParam
#include "Bullet.h"
#include "Common.h"
#include "Enemy.h"
#include "Effect/EffectAdmin.h"
#include "Effect/EffectKind.h"
#include "Fader.h"
#include "FilePath.h"
#include "FontHelper.h"
#include "Input.h"
#include "Item.h"
#include "ItemParam.h"				// Use a recovery amount
#include "Meter.h"
#include "ModelHelper.h"			// Use serialize methods
#include "Music.h"
#include "Parameter.h"
#include "PlayerParam.h"
#include "PointLightStorage.h"
#include "RenderingStuff.h"
#include "SaveData.h"
#include "StageNumber.h"

#if DEBUG_MODE
#include "CSVLoader.h"
#pragma comment( lib, "comdlg32.lib" ) // Used for common-dialog
#endif // DEBUG_MODE

namespace
{
#if DEBUG_MODE
	constexpr bool  wantSilence  = false;
	constexpr float silentSecond = 0.5f;
	constexpr float silentVolume = 0.2f;

	constexpr bool IOFromBinary = false;
#else
	constexpr bool IOFromBinary = true;
#endif // DEBUG_MODE

#if USE_IMGUI
	static bool dontFinishLoadState	= false;
	static bool projectLightCamera	= false;
	static bool drawLightSources	= false;
#endif // USE_IMGUI
}

namespace
{
	struct SceneParam
	{
		struct
		{
			float slerpFactor	= 0.2f;
			float fovDegree		= 30.0f;
			Donya::Vector3 offsetPos{ 0.0f, 5.0f, -10.0f };	// The offset of position from the player position
			Donya::Vector3 offsetFocus;						// The offset of focus from the player position
		}
		camera;
		Donya::Vector2 projectScreenAreaMin{ 0.0f,						0.0f					};
		Donya::Vector2 projectScreenAreaMax{ Common::ScreenWidthF(),	Common::ScreenHeightF()	};
		
		Donya::Model::Constants::PerScene::DirectionalLight directionalLight;

		float scrollTakeSecond	= 1.0f;	// The second required for scrolling
		float waitSecondRetry	= 1.0f; // Waiting second between Miss ~ Re-try
		float waitSecondToOver	= 2.0f; // Waiting second between Miss ~ GameOver

		struct ShadowMap
		{
			Donya::Vector3	color;			// RGB
			float			bias = 0.03f;	// Ease an acne

			float			offsetDistance	= 10.0f;				// From the player position
			Donya::Vector3	projectDirection{  0.0f,  0.0f,  1.0f };
			Donya::Vector3	projectDistance { 10.0f, 10.0f, 50.0f };// [m]
			float			nearDistance	= 1.0f;					// Z near is this. Z far is projectDistance.z.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( color			),
					CEREAL_NVP( bias			),
					CEREAL_NVP( offsetDistance	),
					CEREAL_NVP( projectDirection),
					CEREAL_NVP( projectDistance	),
					CEREAL_NVP( nearDistance	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		ShadowMap shadowMap;

		RenderingHelper::VoxelizeConstant voxelize;

		BloomApplier::Parameter bloomParam;

		Donya::Vector2 deadZone{ 0.3f, 0.3f }; // The stick input is valid if the value greater than this

		struct MeterParam
		{
			Donya::Vector2	hpDrawPos;		// Left-Top, Screen space
			Donya::Vector3	hpDrawColor{ 1.0f, 1.0f, 1.0f };
			float			hpDrawScale = 1.0f;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( hpDrawPos	),
					CEREAL_NVP( hpDrawColor	),
					CEREAL_NVP( hpDrawScale	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		MeterParam playerMeter;
		MeterParam skullMeter;

		float readyFxBeginDelaySecond	= 1.0f;
		Donya::Vector3 readyFxPosOffset; // Offset from the focus point of camera

		float fadeOutSecondOfBGM		= 1.0f;

		float waitSec_PlayAppearBossSE	= 1.0f;
		float waitSec_PlayClearSE		= 1.0f; // From the leave timing
		float waitSecBet_ClearLeave		= 1.0f; // From the clear timing to leave
		float waitSecBet_LeaveFade		= 1.0f; // From the leave timing to fade out

		float bossRoomInitialPosOffset	= 3.0f; // It uses only when the boss's appearing

		Donya::Vector2 ssLoadingDrawPos;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( camera.slerpFactor			),
				CEREAL_NVP( camera.fovDegree			),
				CEREAL_NVP( camera.offsetPos			),
				CEREAL_NVP( camera.offsetFocus			)
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( waitSecondRetry ) );
			}
			if ( 2 <= version )
			{
				archive( CEREAL_NVP( scrollTakeSecond ) );
			}
			if ( 3 <= version )
			{
				archive( CEREAL_NVP( directionalLight ) );
			}
			if ( 4 <= version )
			{
				archive( CEREAL_NVP( shadowMap ) );
			}
			if ( 5 <= version )
			{
				archive( CEREAL_NVP( bloomParam ) );
			}
			if ( 6 <= version )
			{
				archive( CEREAL_NVP( deadZone ) );
			}
			if ( 7 <= version )
			{
				archive
				(
					CEREAL_NVP( projectScreenAreaMin ),
					CEREAL_NVP( projectScreenAreaMax )
				);
			}
			if ( 8 <= version )
			{
				archive
				(
					CEREAL_NVP( playerMeter	),
					CEREAL_NVP( skullMeter	)
				);
			}
			if ( 9 <= version )
			{
				archive( CEREAL_NVP( readyFxPosOffset ) );
			}
			if ( 10 <= version )
			{
				archive
				(
					CEREAL_NVP( readyFxBeginDelaySecond	),
					CEREAL_NVP( fadeOutSecondOfBGM		)
				);
			}
			if ( 11 <= version )
			{
				archive
				(
					CEREAL_NVP( waitSecBet_ClearLeave	),
					CEREAL_NVP( waitSecBet_LeaveFade	)
				);
			}
			if ( 12 <= version )
			{
				archive
				(
					CEREAL_NVP( waitSec_PlayAppearBossSE	),
					CEREAL_NVP( waitSec_PlayClearSE			)
				);
			}
			if ( 13 <= version )
			{
				archive( CEREAL_NVP( bossRoomInitialPosOffset ) );
			}
			if ( 14 <= version )
			{
				archive( CEREAL_NVP( ssLoadingDrawPos ) );
			}
			if ( 15 <= version )
			{
				archive( CEREAL_NVP( waitSecondToOver ) );
			}
			if ( 16 <= version )
			{
				archive( CEREAL_NVP( voxelize ) );
			}
			if ( 17 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode()
		{
			if ( ImGui::TreeNode( u8"カメラ" ) )
			{
				ImGui::DragFloat ( u8"補間倍率",						&camera.slerpFactor,	0.01f );
				ImGui::DragFloat ( u8"画角（Degree）",				&camera.fovDegree,		0.1f  );
				ImGui::DragFloat3( u8"自身の座標（自機からの相対）",	&camera.offsetPos.x,	0.01f );
				ImGui::DragFloat3( u8"注視点の座標（自機からの相対）",	&camera.offsetFocus.x,	0.01f );

				ImGui::TreePop();
			}
			if ( ImGui::TreeNode( u8"スクリーン範囲" ) )
			{
				ImGui::DragFloat2( u8"基準座標・左上",	&projectScreenAreaMin.x );
				ImGui::DragFloat2( u8"基準座標・右下",	&projectScreenAreaMax.x );

				ImGui::TreePop();
			}
			
			ImGui::Helper::ShowDirectionalLightNode( u8"平行光", &directionalLight );

			if ( ImGui::TreeNode( u8"シャドウマップ関連" ) )
			{
				ImGui::ColorEdit3( u8"影の色",					&shadowMap.color.x );
				ImGui::DragFloat ( u8"アクネ用のバイアス",		&shadowMap.bias,				0.01f );
				ImGui::DragFloat ( u8"自機からの距離",			&shadowMap.offsetDistance,		0.1f  );
				ImGui::DragFloat3( u8"写す方向（単位ベクトル）",	&shadowMap.projectDirection.x,	0.01f );
				ImGui::DragFloat3( u8"写す範囲ＸＹＺ",			&shadowMap.projectDistance.x,	1.0f  );
				ImGui::DragFloat ( u8"Z-Near",					&shadowMap.nearDistance,		1.0f  );

				shadowMap.offsetDistance	= std::max( 0.01f, shadowMap.offsetDistance );
				shadowMap.projectDistance.x	= std::max( 0.01f, shadowMap.projectDistance.x );
				shadowMap.projectDistance.y	= std::max( 0.01f, shadowMap.projectDistance.y );
				shadowMap.projectDistance.z	= std::max( shadowMap.nearDistance + 1.0f, shadowMap.projectDistance.z );

				static bool alwaysNormalize = false;
				if ( ImGui::Button( u8"写す方向を正規化" ) || alwaysNormalize )
				{
					shadowMap.projectDirection.Normalize();
				}
				ImGui::Checkbox( u8"常に正規化する", &alwaysNormalize );

				ImGui::TreePop();
			}

			bloomParam.ShowImGuiNode( u8"ブルーム関連" );

			voxelize.ShowImGuiNode( u8"ボクセライズ関連" );

			if ( ImGui::TreeNode( u8"秒数関連" ) )
			{
				ImGui::DragFloat( u8"READY表示までの遅延",				&readyFxBeginDelaySecond,	0.01f );
				ImGui::DragFloat( u8"ＢＧＭのフェードアウトにかける秒数",	&fadeOutSecondOfBGM,		0.01f );
				ImGui::DragFloat( u8"スクロールに要する秒数",				&scrollTakeSecond,			0.01f );
				ImGui::DragFloat( u8"ミスからリトライまでの待機秒数",		&waitSecondRetry,			0.01f );
				ImGui::DragFloat( u8"ミスからゲームオーバーまでの待機秒数",	&waitSecondToOver,			0.01f );
				ImGui::DragFloat( u8"ボス登場時ＳＥ再生までの待機秒数",	&waitSec_PlayAppearBossSE,	0.01f );
				ImGui::DragFloat( u8"クリア時ＳＥ再生までの待機秒数",		&waitSec_PlayClearSE,		0.01f );
				ImGui::DragFloat( u8"クリアから退場までの待機秒数",		&waitSecBet_ClearLeave,		0.01f );
				ImGui::DragFloat( u8"退場からフェードまでの待機秒数",		&waitSecBet_LeaveFade,		0.01f );
				readyFxBeginDelaySecond	= std::max( 0.0f,  readyFxBeginDelaySecond	);
				fadeOutSecondOfBGM		= std::max( 0.0f,  fadeOutSecondOfBGM		);
				scrollTakeSecond		= std::max( 0.01f, scrollTakeSecond			);
				waitSecondRetry			= std::max( 0.01f, waitSecondRetry			);
				waitSecondToOver		= std::max( 0.01f, waitSecondToOver			);
				waitSec_PlayAppearBossSE= std::max( 0.0f,  waitSec_PlayAppearBossSE	);
				waitSec_PlayClearSE		= std::max( 0.0f,  waitSec_PlayClearSE		);
				waitSecBet_ClearLeave	= std::max( 0.0f,  waitSecBet_ClearLeave	);
				waitSecBet_LeaveFade	= std::max( 0.0f,  waitSecBet_LeaveFade		);

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"ボス登場関連" ) )
			{
				ImGui::DragFloat( u8"自機の定位置へのボス部屋端からの距離", &bossRoomInitialPosOffset, 0.01f );
				bossRoomInitialPosOffset = std::max( 0.0f, bossRoomInitialPosOffset );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"メータ関連" ) )
			{
				auto ShowPart = []( const char *caption, MeterParam *p )
				{
					if ( !ImGui::TreeNode( caption ) ) { return; }
					// else

					ImGui::DragFloat2( u8"ＨＰ描画位置（左上座標）",	&p->hpDrawPos.x		);
					ImGui::ColorEdit3( u8"ＨＰ描画色",				&p->hpDrawColor.x	);
					ImGui::DragFloat ( u8"ＨＰ描画スケール",			&p->hpDrawScale, 0.01f );

					ImGui::TreePop();
				};

				ShowPart( u8"自機",		&playerMeter	);
				ShowPart( u8"スカル",	&skullMeter		);

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"その他" ) )
			{
				ImGui::DragFloat2( u8"Loading：表示座標", &ssLoadingDrawPos.x );
				ImGui::DragFloat3( u8"READY：表示座標（注視点からの相対）", &readyFxPosOffset.x, 0.01f );
				ImGui::SliderFloat2( u8"スティックのデッドゾーン", &deadZone.x, 0.0f, 1.0f );
				ImGui::TreePop();
			}
		}
	#endif // USE_IMGUI
	};

	static ParamOperator<SceneParam> sceneParam{ "SceneGame" };
	const SceneParam &FetchParameter()
	{
		return sceneParam.Get();
	}

#if DEBUG_MODE
	constexpr unsigned int maxPathBufferSize = MAX_PATH;
	std::string FetchStageFilePathByCommonDialog()
	{
		char chosenFullPaths[maxPathBufferSize] = { 0 };
		char chosenFileName [maxPathBufferSize] = { 0 };

		OPENFILENAMEA ofn{ 0 };
		ofn.lStructSize		= sizeof( decltype( ofn ) );
		ofn.hwndOwner		= Donya::GetHWnd();
		ofn.lpstrFilter		= "CSV-file(*.csv)\0*.csv\0"
							  "\0";
		ofn.lpstrFile		= chosenFullPaths;
		ofn.nMaxFile		= maxPathBufferSize;
		ofn.lpstrFileTitle	= chosenFileName;
		ofn.nMaxFileTitle	= maxPathBufferSize;
		ofn.Flags			= OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR; // Prevent the current directory of this application will be changed.

		auto  result = GetOpenFileNameA( &ofn );
		if ( !result ) { return std::string{}; }
		// else

		return std::string{ ofn.lpstrFile };
	}
#endif // DEBUG_MODE
}
CEREAL_CLASS_VERSION( SceneParam,				16 )
CEREAL_CLASS_VERSION( SceneParam::ShadowMap,	0  )

void SceneGame::Init()
{
	sceneParam.LoadParameter();

	status		= State::FirstInitialize;
	stageNumber	= Definition::StageNumber::Game();

	loadPerformer.Init();
	loadPerformer.Start( FetchParameter().ssLoadingDrawPos, Donya::Color::Code::BLACK );

	constexpr auto coInitValue = COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE;
	auto InitObjects	= [coInitValue]( SceneGame *pScene, Thread::Result *pResult )
	{
		if ( !pScene || !pResult ) { assert( !"HUMAN ERROR" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, coInitValue );
		if ( FAILED( hr ) )
		{
			pResult->WriteResult( /* wasSucceeded = */ false );
			return;
		}
		// else

		bool succeeded	= true;
		bool result		= true;

		PauseProcessor::LoadParameter();
		Performer::LoadPart::LoadParameter();
		pScene->playerIniter.LoadParameter( pScene->stageNumber );

					pScene->pSky = std::make_unique<Sky>();
		result =	pScene->pSky->Init();
		if ( !result ) { succeeded = false; }

		pScene->pMap = std::make_unique<Map>();

		constexpr auto stayFirstState = State::FirstInitialize;
		pScene->InitStage( pScene->currentPlayingBGM, pScene->stageNumber, /* reloadModel = */ true, stayFirstState );

		// The item depends on the map for detect to be buried.
		// So must initialize after the map's initialize.
		auto &itemAdmin = Item::Admin::Get();
		const Map emptyMap{}; // Used for empty argument. Fali safe.
		const Map &mapRef = ( pScene->pMap ) ? *pScene->pMap : emptyMap;
		itemAdmin.ClearInstances();
		itemAdmin.LoadItems( pScene->stageNumber, mapRef, IOFromBinary );
	#if DEBUG_MODE
		itemAdmin.SaveItems( pScene->stageNumber, true );
	#endif // DEBUG_MODE

		pResult->WriteResult( succeeded );
		CoUninitialize();
	};
	thObjects.pThread	= std::make_unique<std::thread>( InitObjects, this, &thObjects.result );
	
	auto &renderer = RenderingStuffInstance::Get();
	renderer.AssignBloomParameter( FetchParameter().bloomParam );
	renderer.ClearBuffers();

	auto &effectAdmin = Effect::Admin::Get();
	effectAdmin.SetLightColorAmbient( Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } );
	effectAdmin.SetLightColorDiffuse( Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } );
	effectAdmin.SetLightDirection	( FetchParameter().directionalLight.direction.XYZ() );
}
void SceneGame::Uninit()
{
	UninitStage();

	if ( pMap ) { pMap->ReleaseModel(); }
	pMap.reset();

	loadPerformer.Uninit();

	Effect::Admin::Get().ClearInstances();

	Donya::Sound::Stop( currentPlayingBGM, /* isEnableForAll = */ true );
}

Scene::Result SceneGame::Update( float elapsedTime )
{
#if DEBUG_MODE
	if ( status != State::FirstInitialize )
	{
		if ( Donya::Keyboard::Trigger( VK_F2 ) && !Fader::Get().IsExist() )
		{
			Donya::Sound::Play( Music::DEBUG_Strong );

			const bool ctrled  = Donya::Keyboard::Press( VK_CONTROL	);
			const bool shifted = Donya::Keyboard::Press( VK_SHIFT	);

			Scene::Type next = Scene::Type::Title;
			if ( shifted && ctrled )
			{ next = Scene::Type::Over; }
			else if ( ctrled )
			{ next = Scene::Type::Result; }
			else if ( shifted )
			{
				next = Scene::Type::Game;
				// Completely reset
				playerIniter.LoadParameter( stageNumber );
			}

			StartFade( next );
		}
		if ( Donya::Keyboard::Trigger( VK_F5 ) )
		{
			nowDebugMode = !nowDebugMode;

			if ( nowDebugMode )
			{
				iCamera.ChangeMode( Donya::ICamera::Mode::Satellite );
			}
			else
			{
				iCamera.SetOrientation( Donya::Quaternion::Identity() );
				lightCamera.SetOrientation( Donya::Quaternion::Identity() );
				CameraInit();
			}
		}
	}
#endif // DEBUG_MODE

#if USE_IMGUI
	UseImGui( elapsedTime );

	// Apply for be able to see an adjustment immediately
	if ( status != State::FirstInitialize )
	{
		RenderingStuffInstance::Get().AssignBloomParameter( FetchParameter().bloomParam );

		const auto &data = FetchParameter();
		if ( pPlayerMeter ) { pPlayerMeter->SetDrawOption( data.playerMeter.hpDrawPos, data.playerMeter.hpDrawColor, data.playerMeter.hpDrawScale ); }
		if ( pSkullMeter  ) { pSkullMeter->SetDrawOption( data.skullMeter.hpDrawPos, data.skullMeter.hpDrawColor, data.skullMeter.hpDrawScale ); }
	}
#endif // USE_IMGUI

	loadPerformer.UpdateIfActive( elapsedTime );

	if ( status == State::FirstInitialize )
	{
		FirstInitStateUpdate( elapsedTime );
		Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
		return noop;
	}
	// else


	const auto &data = FetchParameter();
	PointLightStorage::Get().Clear();


	// Re-try the same stage
	if ( Fader::Get().IsClosed() && nextScene == Scene::Type::Game )
	{
		UninitStage();
		InitStage( Music::BGM_Game, stageNumber, /* reloadModel = */ false );
	}

	
	controller.Update();
	AssignCurrentInput();

	const float updateSpeedFactor = PauseUpdate( elapsedTime );
	elapsedTime *= updateSpeedFactor;

	stageTimer += elapsedTime;
	ReadyPlayer();

	const float deltaTimeForMove  = ( scroll.active ) ? 0.0f : elapsedTime;
	const float deltaTimeForAnime = ( scroll.active ) ? 0.0f : elapsedTime;


	if ( pSky	) { pSky->Update( elapsedTime );		}
	if ( pMap	) { pMap->Update( deltaTimeForMove );	}
	if ( pDoors	)
	{
		pDoors->Update( elapsedTime );
		if ( pMap )
		{
			pMap->ClearExtraSolids();
			pMap->RegisterExtraSolids( pDoors->GetDoorBodies() );
		}
	}
	const Map emptyMap{}; // Used for empty argument. Fali safe.
	const Map &mapRef = ( pMap ) ? *pMap : emptyMap;

	DoorUpdate();


	const int oldRoomID = currentRoomID;
	UpdateCurrentRoomID();
	const Room *pCurrentRoom = pHouse->FindRoomOrNullptr( currentRoomID );
	if ( oldRoomID != currentRoomID )
	{
		if ( pSky )
		{
			pSky->AdvanceHourTo( pCurrentRoom->GetHour(), data.scrollTakeSecond );
		}

		isThereClearEvent	= ( pClearEvent		&& pClearEvent->IsThereIn( currentRoomID ) );
		isThereBoss			= ( pBossContainer	&& pBossContainer->IsThereIn( currentRoomID ) );
	}


	StageStateUpdate( elapsedTime );
	AppearBossStateUpdate( elapsedTime );
	VSBossStateUpdate( elapsedTime );
	ClearStateUpdate( elapsedTime );

	
	PlayerUpdate( deltaTimeForMove, mapRef );
	const bool  nextIsGameOver		= Player::Remaining::Get() <= 0;
	const float &waitSecondForMiss	= ( nextIsGameOver ) ? data.waitSecondToOver : data.waitSecondRetry;
	if ( waitSecondForMiss <= elapsedSecondsAfterMiss && !Fader::Get().IsExist() )
	{
		const int remaining = Player::Remaining::Get();
		if ( remaining <= 0 )
		{
			StartFade( Scene::Type::Over );
			// The remaining count will be re-set at transitioned scene
		}
		else
		{
			// Re-try the game
			StartFade( Scene::Type::Game );

			Player::Remaining::Decrement();
		}
	}

	const Donya::Vector3 playerPos = GetPlayerPosition();
	Bullet::Admin::Get().Update( deltaTimeForMove, currentScreen );
	Enemy::Admin::Get().Update( deltaTimeForMove, playerPos, currentScreen );
	BossUpdate( deltaTimeForMove, playerPos );
	Item::Admin::Get().Update( deltaTimeForMove, currentScreen, mapRef );



	PlayerPhysicUpdate( deltaTimeForMove, mapRef );
	Bullet::Admin::Get().PhysicUpdate( deltaTimeForMove, mapRef );
	Enemy::Admin::Get().PhysicUpdate( deltaTimeForMove, mapRef );
	Item::Admin::Get().PhysicUpdate( deltaTimeForMove, mapRef );
	if ( pBossContainer ) { pBossContainer->PhysicUpdate( deltaTimeForMove, mapRef ); }
	

	// AssignCameraPos() (will called in CameraUpdate()) depends the currentScreen, so I should update that before CameraUpdate().
	currentScreen = CalcCurrentScreenPlane();
	CameraUpdate( elapsedTime );

	UpdatePlayerIniter();

	// Kill the player if fall out from current room
	if ( pPlayer && !pPlayer->NowMiss() )
	{
		// If the transition to downward is allowed,
		// the bottom of current room will scroll faster than the player's falling.
		// But if that transition is not allowed,
		// the current room does not scroll, so the player will fall out from current room, then die.

		const float playerTop  = pPlayer->GetHitBox().Max().y;
		const float roomBottom = ( pCurrentRoom ) ? pCurrentRoom->GetArea().Min().y : currentScreen.Min().y;
		if ( playerTop < roomBottom )
		{
			pPlayer->KillMe();
		}
	}


	Collision_PlayerVSItem();
	Collision_BulletVSBullet();
	Collision_BulletVSBoss();
	Collision_BulletVSEnemy();
	Collision_BulletVSPlayer();
	Collision_BossVSPlayer();
	Collision_EnemyVSPlayer();

	// Needle VS Player.
	// By call it after VS Boss/Enemy, it makes to priority the VS Boss/Enemy than VS Needle if these collisions occur at the same time.
	if ( pPlayer && !pPlayer->NowMiss() )
	{
		pPlayer->KillMeIfCollideToKillAreas( deltaTimeForMove, mapRef );
	}

	if ( pPlayerMeter ) { pPlayerMeter->Update( elapsedTime ); }
	if ( pSkullMeter  ) { pSkullMeter->Update( elapsedTime );  }

	return ReturnResult();
}

namespace
{
	enum class DrawTarget
	{
		Map		= 1 << 0,
		Door	= 1 << 1,
		Bullet	= 1 << 2,
		Player	= 1 << 3,
		Boss	= 1 << 4,
		Enemy	= 1 << 5,
		Item	= 1 << 6,

		All		= Map | Door | Bullet | Player | Boss | Enemy | Item
	};
	DEFINE_ENUM_FLAG_OPERATORS( DrawTarget )
}
void SceneGame::Draw( float elapsedTime )
{
	ClearBackGround();

	if ( status == State::FirstInitialize )
	{
		loadPerformer.DrawIfActive( 0.0f );
		return;
	}
	// else

	RenderingStuff *p = RenderingStuffInstance::Get().Ptr();
	if ( !p ) { return; }
	// else

	auto UpdateSceneConstant	= [&]( const Donya::Model::Constants::PerScene::DirectionalLight &directionalLight, const Donya::Vector4 &eyePos, const Donya::Vector4x4 &viewMatrix, const Donya::Vector4x4 &viewProjectionMatrix, bool applyToEffect )
	{
		Donya::Model::Constants::PerScene::Common constant{};
		constant.directionalLight	= directionalLight;
		constant.eyePosition		= eyePos;
		constant.viewMatrix			= viewMatrix;
		constant.viewProjMatrix		= viewProjectionMatrix;
		p->renderer.UpdateConstant( constant );

		if ( applyToEffect )
		{
			auto &effectAdmin = Effect::Admin::Get();
			effectAdmin.SetViewMatrix( viewMatrix );
			// Currently I judge to it is not necessary
			// effectAdmin.SetLightColorAmbient( directionalLight.light.ambientColor );
			// effectAdmin.SetLightColorDiffuse( directionalLight.light.diffuseColor );
			effectAdmin.SetLightDirection	( directionalLight.direction.XYZ() );
		}
	};
	auto DrawObjects			= [&]( DrawTarget option, bool castShadow )
	{
		using Kind = DrawTarget;
		auto Drawable = [&option]( Kind verify )
		{
			return scast<int>( option & verify ) != 0;
		};

		// The drawing priority is determined by the priority of the information.

		( castShadow )
		? p->renderer.ActivateShaderShadowStatic()
		: p->renderer.ActivateShaderNormalStatic();

		if ( Drawable( Kind::Map ) && pMap ) { pMap->Draw( &p->renderer ); }

		( castShadow )
		? p->renderer.DeactivateShaderShadowStatic()
		: p->renderer.DeactivateShaderNormalStatic();


		( castShadow )
		? p->renderer.ActivateShaderShadowSkinning()
		: p->renderer.ActivateShaderNormalSkinning();

		if ( Drawable( Kind::Player	) && pPlayer		)	{ pPlayer->				Draw( &p->renderer ); }
		if ( Drawable( Kind::Boss	) && pBossContainer	)	{ pBossContainer->		Draw( &p->renderer ); }
		if ( Drawable( Kind::Enemy	) )						{ Enemy::Admin::Get().	Draw( &p->renderer ); }
		if ( Drawable( Kind::Door	) && pDoors			)	{ pDoors->				Draw( &p->renderer ); }
		if ( Drawable( Kind::Item	) )						{ Item::Admin::Get().	Draw( &p->renderer ); }
		if ( Drawable( Kind::Bullet	) )						{ Bullet::Admin::Get().	Draw( &p->renderer ); }

		( castShadow )
		? p->renderer.DeactivateShaderShadowSkinning()
		: p->renderer.DeactivateShaderNormalSkinning();
	};
	
#if DEBUG_MODE
		  Donya::Vector4   cameraPos = Donya::Vector4{ iCamera.GetPosition(), 1.0f };
		  Donya::Vector4x4 V	= iCamera.CalcViewMatrix();
		  Donya::Vector4x4 VP	= V * iCamera.GetProjectionMatrix();
	if ( projectLightCamera )
	{
		cameraPos = Donya::Vector4{ lightCamera.GetPosition(), 1.0f };
		V	= CalcLightViewMatrix();
		VP	= V * lightCamera.GetProjectionMatrix();
	}
#else
	const Donya::Vector4   cameraPos = Donya::Vector4{ iCamera.GetPosition(), 1.0f };
	const Donya::Vector4x4 V   = iCamera.CalcViewMatrix();
	const Donya::Vector4x4 VP  = V * iCamera.GetProjectionMatrix();
#endif // DEBUG_MODE

	const Donya::Vector4   lightPos = Donya::Vector4{ lightCamera.GetPosition(), 1.0f };
	const Donya::Vector4x4 LV  = CalcLightViewMatrix();
	const Donya::Vector4x4 LVP = LV * lightCamera.GetProjectionMatrix();
	const auto &data = FetchParameter();

	// Draw the back-ground
	if ( pSky )
	{
		p->screenSurface.SetRenderTarget();
		p->screenSurface.SetViewport();
		
		pSky->Draw( cameraPos.XYZ(), VP );

		Donya::Surface::ResetRenderTarget();
	}

	Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLess );
	Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullBack_CCW );
	p->renderer.ActivateSamplerModel( Donya::Sampler::Defined::Aniso_Wrap );
	p->renderer.ActivateSamplerNormal( Donya::Sampler::Defined::Point_Wrap );

	p->shadowMap.SetRenderTarget();
	p->shadowMap.SetViewport();
	// Make the shadow map
	{
		// Update scene constant as light source
		{
			Donya::Model::Constants::PerScene::DirectionalLight tmpDirLight{};
			tmpDirLight.direction = Donya::Vector4{ data.shadowMap.projectDirection.Unit(), 0.0f };
			UpdateSceneConstant( tmpDirLight, lightPos, LV, LVP, /* applyToEffect = */ false );
		}
		p->renderer.ActivateConstantScene();

		DrawObjects( DrawTarget::All, /* castShadow = */ true );

		p->renderer.DeactivateConstantScene();
	}
	Donya::Surface::ResetRenderTarget();

	p->screenSurface.SetRenderTarget();
	p->screenSurface.SetViewport();
	// Draw normal scene with shadow map
	{
		RenderingHelper::ShadowConstant shadowConstant{};

		// Update scene and shadow constants
		{
			const auto skyColor = ( pSky ) ? Donya::Vector4{ pSky->GetCurrentColor(), 1.0f } : Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
			auto dir = data.directionalLight;
			dir.light.ambientColor = Donya::Vector4::Product( dir.light.ambientColor, skyColor );
			dir.light.diffuseColor = Donya::Vector4::Product( dir.light.diffuseColor, skyColor );
			UpdateSceneConstant( dir, cameraPos, V, VP, /* applyToEffect = */ true );

			shadowConstant.lightProjMatrix	= LVP;
			shadowConstant.shadowColor		= data.shadowMap.color;
			shadowConstant.shadowBias		= data.shadowMap.bias;
			p->renderer.UpdateConstant( shadowConstant );
		}
		// Update voxelize constant
		{
			p->renderer.UpdateConstant( data.voxelize );
		}
		// Update point light constant
		{
			p->renderer.UpdateConstant( PointLightStorage::Get().GetStorage() );
		}

		p->renderer.ActivateConstantScene();
		p->renderer.ActivateConstantPointLight();
		p->renderer.ActivateConstantShadow();
		p->renderer.ActivateConstantVoxelize();
		p->renderer.ActivateSamplerShadow( Donya::Sampler::Defined::Point_Border_White );
		p->renderer.ActivateShadowMap( p->shadowMap );

		constexpr DrawTarget option = DrawTarget::All ^ DrawTarget::Bullet ^ DrawTarget::Item;
		DrawObjects( option, /* castShadow = */ false );

		// Disable shadow
		{
			p->renderer.DeactivateConstantShadow();
			shadowConstant.shadowBias = 1.0f; // Make the pixel to nearest
			p->renderer.UpdateConstant( shadowConstant );
			p->renderer.ActivateConstantShadow();
		}

		DrawObjects( DrawTarget::Item, /* castShadow = */ false );

		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::NoTest_Write );
		DrawObjects( DrawTarget::Bullet, /* castShadow = */ false );
		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLess );

		p->renderer.DeactivateShadowMap( p->shadowMap );
		p->renderer.DeactivateSamplerShadow();
		p->renderer.DeactivateConstantVoxelize();
		p->renderer.DeactivateConstantShadow();
		p->renderer.DeactivateConstantPointLight();
		p->renderer.DeactivateConstantScene();
	}
	Donya::Surface::ResetRenderTarget();

	p->renderer.DeactivateSamplerModel();
	Donya::Rasterizer::Deactivate();
	Donya::DepthStencil::Deactivate();

	// Generate the buffers of bloom
	{
		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLessEq );
		Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullNone );

		const float oldDepth = Donya::Sprite::GetDrawDepth();
		Donya::Sprite::SetDrawDepth( 0.0f );
		Donya::Sampler::SetPS( Donya::Sampler::Defined::Linear_Border_Black, 0 );
		p->bloomer.WriteLuminance( p->screenSurface );
		Donya::Sampler::ResetPS( 0 );
		Donya::Sprite::SetDrawDepth( oldDepth );

		Donya::Sampler::SetPS( Donya::Sampler::Defined::Aniso_Wrap, 0 );
		p->bloomer.WriteBlur();
		Donya::Sampler::ResetPS( 0 );

		Donya::Rasterizer::Deactivate();
		Donya::DepthStencil::Deactivate();
	}

	Donya::SetDefaultRenderTargets();

	const Donya::Vector2 screenSurfaceSize = p->screenSurface.GetSurfaceSizeF();

	Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullNone );
	Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLessEq );
	// Draw the scene to screen
	{
		Donya::Sampler::SetPS( Donya::Sampler::Defined::Aniso_Wrap, 0 );
		Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );

		p->quadShader.VS.Activate();
		p->quadShader.PS.Activate();

		p->screenSurface.SetRenderTargetShaderResourcePS( 0U );

		p->displayer.Draw
		(
			screenSurfaceSize,
			Donya::Vector2::Zero()
		);

		p->screenSurface.ResetShaderResourcePS( 0U );

		p->quadShader.PS.Deactivate();
		p->quadShader.VS.Deactivate();

		Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );
		Donya::Sampler::ResetPS( 0 );
	}
	Donya::DepthStencil::Deactivate();

	Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::NoTest_NoWrite );
	// Add the bloom buffers
	{
		const float oldDepth = Donya::Sprite::GetDrawDepth();
		Donya::Sprite::SetDrawDepth( 0.0f );

		Donya::Blend::Activate( Donya::Blend::Mode::ADD_NO_ATC );
		p->bloomer.DrawBlurBuffers( screenSurfaceSize );
		Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );

		Donya::Sprite::SetDrawDepth( oldDepth );
	}
	Donya::DepthStencil::Deactivate();

	Donya::Rasterizer::Deactivate();

#if DEBUG_MODE
	// Object's hit/hurt boxes
	if ( Common::IsShowCollision() )
	{
		if ( pPlayer		) { pPlayer->		DrawHitBox	( &p->renderer, VP ); }
		if ( pBossContainer	) { pBossContainer->DrawHitBoxes( &p->renderer, VP ); }
		if ( pClearEvent	) { pClearEvent->	DrawHitBoxes( &p->renderer, VP ); }
		if ( pMap			) { pMap->			DrawHitBoxes( currentScreen, &p->renderer, VP ); }
		Bullet::Admin::Get().DrawHitBoxes( &p->renderer, VP );
		Enemy ::Admin::Get().DrawHitBoxes( &p->renderer, VP );
		Item  ::Admin::Get().DrawHitBoxes( &p->renderer, VP );
		checkPoint.DrawHitBoxes( &p->renderer, VP );
		if ( pDoors			) { pDoors->		DrawHitBoxes( &p->renderer, VP ); }
		if ( pHouse			) { pHouse->		DrawHitBoxes( &p->renderer, VP ); }
	}
#endif // DEBUG_MODE

	// Draw HP meters
	{
		// DrawRemains() will change the sprite from the meter's,
		// So I prioritize the boss's meter that doesn't use DrawRemains().

		if ( isThereBoss && pSkullMeter )
		{
			pSkullMeter->Draw();
			pSkullMeter->DrawIcon( Meter::Icon::Skull );
		}
		if ( pPlayer && pPlayerMeter )
		{
			pPlayerMeter->Draw();
			pPlayerMeter->DrawIcon( Meter::Icon::Player );
			pPlayerMeter->DrawRemains( FontAttribute::Main, Player::Remaining::Get() );
		}
	}

#if DEBUG_MODE
	// Some visualizing
	{
		static Donya::Geometric::Line line{ 512U };
		static bool shouldInitializeLine = true;
		if ( shouldInitializeLine )
		{
			shouldInitializeLine = false;
			line.Init();
		}

		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= VP;
		constant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 0.6f };
		constant.lightDirection	= data.directionalLight.direction.XYZ();
		
		auto DrawCube = [&]( const Donya::Vector3 &pos, const Donya::Vector3 &scale = { 1.0f, 1.0f, 1.0f }, const Donya::Quaternion &rotation = Donya::Quaternion::Identity() )
		{
			constant.matWorld = Donya::Vector4x4::Identity();
			constant.matWorld._11 = scale.x * 2.0f;
			constant.matWorld._22 = scale.y * 2.0f;
			constant.matWorld._33 = scale.z * 2.0f;
			constant.matWorld *= rotation.MakeRotationMatrix();
			constant.matWorld._41 = pos.x;
			constant.matWorld._42 = pos.y;
			constant.matWorld._43 = pos.z;
			p->renderer.ProcessDrawingCube( constant );
		};

		// Screen box
		if ( 0 )
		{
			const auto hWidth  = currentScreen.size.x;
			const auto hHeight = currentScreen.size.y;
			const Donya::Vector2 scale{ hWidth * 2.0f / 0.5f, hHeight * 2.0f / 0.5f };

			constexpr auto  color = Donya::Color::Code::TEAL;
			constexpr float alpha = 0.6f;
			constant.drawColor = Donya::Vector4{ Donya::Color::MakeColor( color ), alpha };
			DrawCube( currentScreen.WorldPosition(), Donya::Vector3{ scale, 0.2f } );
		}

		// Light source for shadow map
		if ( drawLightSources )
		{
			constexpr float lineLength = 24.0f;
			const auto lookDirection = data.shadowMap.projectDirection.Unit();
			
			auto DrawAxisBox = [&]( const Donya::Quaternion &orientation, Donya::Color::Code frontLineColor, Donya::Color::Code otherLineColor )
			{
				struct Bundle { Donya::Vector3 dir; Donya::Color::Code color; };
				const Bundle directions[]
				{
					{ orientation.LocalRight(),	otherLineColor	},
					{ orientation.LocalUp(),	otherLineColor	},
					{ orientation.LocalFront(),	frontLineColor	},
				};
				const Donya::Vector3 start = lightPos.XYZ();
				
				for ( const auto &it : directions )
				{
					line.Reserve
					(
						start, start + ( it.dir * lineLength ),
						Donya::Vector4{ Donya::Color::MakeColor( it.color ), 1.0f }
					);
				}

				constant.drawColor = Donya::Vector4{ Donya::Color::MakeColor( frontLineColor ), 0.8f };

				constexpr Donya::Vector3 scale{ 1.0f, 1.0f, 1.0f };
				DrawCube( start, scale, orientation );
			};
			
			const Donya::Quaternion rawViewRot = lightCamera.GetOrientation();
			DrawAxisBox( rawViewRot, Donya::Color::Code::MAGENTA, Donya::Color::Code::DARK_GRAY );
		}

		// PointLight source
		if ( drawLightSources )
		{
			const auto &plr = PointLightStorage::Get().GetStorage();

			constexpr Donya::Vector3 scale{ 0.05f, 0.05f, 0.05f };
			for ( unsigned int i = 0; i < plr.enableLightCount; ++i )
			{
				constant.drawColor.x = plr.lights[i].light.diffuseColor.x;
				constant.drawColor.y = plr.lights[i].light.diffuseColor.y;
				constant.drawColor.z = plr.lights[i].light.diffuseColor.z;
				constant.drawColor.w = 1.0f;
				DrawCube( plr.lights[i].wsPos, scale );
			}
		}

		line.Flush( VP );
	}
#endif // DEBUG_MODE



	if ( pPauser )
	{
		pPauser->Draw( elapsedTime );
	}

	loadPerformer.DrawIfActive( 0.0f );
}

void SceneGame::PlayBGM( Music::ID kind )
{
	Donya::Sound::Stop( currentPlayingBGM, /* isEnableForAll = */ true );
	currentPlayingBGM = kind;
	Donya::Sound::Play( currentPlayingBGM );

#if DEBUG_MODE
	if ( wantSilence )
	{
		Donya::Sound::AppendFadePoint( currentPlayingBGM, silentSecond, silentVolume, /* isEnableForAll = */ true );
	}
#endif // DEBUG_MODE
}
void SceneGame::FadeOutBGM() const
{
	Donya::Sound::AppendFadePoint( currentPlayingBGM, FetchParameter().fadeOutSecondOfBGM, 0.0f, /* isEnableForAll = */ true );
}

Donya::Vector4x4 SceneGame::MakeScreenTransform() const
{
	constexpr Donya::Vector4x4 matViewport = Donya::Vector4x4::MakeViewport( { Common::ScreenWidthF(), Common::ScreenHeightF() } );

	const Donya::Vector4x4 matViewProj = iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix();
	return matViewProj * matViewport;
}
Donya::Collision::Box3F SceneGame::CalcCurrentScreenPlane() const
{
	const Donya::Vector4x4 toWorld = MakeScreenTransform().Inverse();

	Donya::Plane xyPlane;
	xyPlane.distance	= 0.0f;
	xyPlane.normal		= -Donya::Vector3::Front();

	auto Transform		= [&]( const Donya::Vector3 &v, float fourthParam, const Donya::Vector4x4 &m )
	{
		Donya::Vector4 tmp = m.Mul( v, fourthParam );
		tmp /= tmp.w;
		return tmp.XYZ();
	};
	auto CalcWorldPos	= [&]( const Donya::Vector2 &ssPos, const Donya::Vector3 &oldPos )
	{
		const Donya::Vector3 ssRayStart{ ssPos, 0.0f };
		const Donya::Vector3 ssRayEnd  { ssPos, 1.0f };

		const Donya::Vector3 wsRayStart	= Transform( ssRayStart,	1.0f, toWorld );
		const Donya::Vector3 wsRayEnd	= Transform( ssRayEnd,		1.0f, toWorld );

		const auto result = Donya::CalcIntersectionPoint( wsRayStart, wsRayEnd, xyPlane );
		return ( result.isIntersect ) ? result.intersection : oldPos;
	};

	Donya::Vector3 nowMin{ currentScreen.Min().XY(), 0.0f };
	Donya::Vector3 nowMax{ currentScreen.Max().XY(), 0.0f };

	const auto &data = FetchParameter();
	nowMin = CalcWorldPos( data.projectScreenAreaMin,	nowMin	);
	nowMax = CalcWorldPos( data.projectScreenAreaMax,	nowMax	);

	Donya::Vector2 wsHalfSize
	{
		fabsf( nowMax.x - nowMin.x ) * 0.5f,
		fabsf( nowMax.y - nowMin.y ) * 0.5f,
	};

	const auto roomArea = pHouse->CalcRoomArea( currentRoomID );
	if ( roomArea != Donya::Collision::Box3F::Nil() )
	{
		// Clamp to room area
		wsHalfSize.x = std::min( roomArea.size.x, wsHalfSize.x );
		wsHalfSize.y = std::min( roomArea.size.y, wsHalfSize.y );
	}

	Donya::Collision::Box3F nowScreen;
	nowScreen.pos.x  = nowMin.x + wsHalfSize.x;	// Specify center
	nowScreen.pos.y  = nowMin.y - wsHalfSize.y;	// Minus for resolve Y-plus direction between screen space and world space
	nowScreen.pos.z  = 0.0f;
	nowScreen.size.x = wsHalfSize.x;
	nowScreen.size.y = wsHalfSize.y;
	nowScreen.size.z = FLT_MAX;

	return nowScreen;
}

void SceneGame::InitStage( Music::ID nextBGM, int stageNo, bool reloadMapModel, State destState )
{
	status = destState;
	stageTimer = 0.0f;
	elapsedSecondsAfterMiss = 0.0f;

	if ( currentPlayingBGM != nextBGM )
	{
		PlayBGM( nextBGM );
	}

	Effect::Admin::Get().ClearInstances();

	// Remove the player relates
	if ( pPlayer )
	{
		pPlayer->Uninit();
		pPlayer.reset();
	}
	if ( pPlayerMeter )
	{
		pPlayerMeter.reset();
	}
	if ( pFxReady )
	{
		pFxReady->Stop();
		pFxReady.reset();
	}
	// But the PlayerInitializer must use the same data
	const Donya::Vector3 playerPos = playerIniter.GetWorldInitialPos();

	const auto &data = FetchParameter();

	/*
	The dependencies of initializations:
	Enemies:		[CurrentScreen, PlayerPos]	- depends on the player position as target, and current screen for decide the initial state
	CurrentScreen:	[Camera]					- depends on the view and projection matrix of the camera
	Camera:			[PlayerPos, House]			- depends on the player position(camera), and room position(light camera)
	CurrentRoomID	[PlayerPos, House]			- the current room indicates what the player belongs map
	House		- it is free
	PlayerPos	- it is free("pPlayerIniter" will be in charge)
	Map			- it is free
	*/


	// Initialize a dependent objects


	pHouse = std::make_unique<House>();
	pHouse->Init( stageNo );

	if ( pMap ) { pMap->Init( stageNo, reloadMapModel ); }

	currentRoomID = CalcCurrentRoomID();
	const Room  *pCurrentRoom = pHouse->FindRoomOrNullptr( currentRoomID );
	if ( pSky && pCurrentRoom )
	{
		pSky->AdvanceHourTo( pCurrentRoom->GetHour(), 0.0f ); // Immediately
	}

	CameraInit();
	currentScreen = CalcCurrentScreenPlane();

	auto &enemyAdmin = Enemy::Admin::Get();
	enemyAdmin.ClearInstances();
	enemyAdmin.LoadEnemies( stageNo, playerPos, currentScreen, IOFromBinary );
#if DEBUG_MODE
	enemyAdmin.SaveEnemies( stageNo, true );
#endif // DEBUG_MODE


	// Initialize a non-dependent objects

	checkPoint.LoadParameter( stageNumber );

	pDoors = std::make_unique<Door::Container>();
	pDoors->Init( stageNo );
#if DEBUG_MODE
	pDoors->SaveBin( stageNo );
	pDoors->SaveJson( stageNo );
#endif // DEBUG_MODE
	pThroughingDoor = nullptr;

	pClearEvent = std::make_unique<ClearEvent>();
	pClearEvent->Init( stageNo );
	isThereClearEvent = pClearEvent->IsThereIn( currentRoomID );

	pBossContainer = std::make_unique<Boss::Container>();
	pBossContainer->Init( stageNo );
#if DEBUG_MODE
	pBossContainer->SaveBosses( stageNo, true );
#endif // DEBUG_MODE
	isThereBoss = pBossContainer->IsThereIn( currentRoomID );
	pSkullMeter.reset(); // If there a boss, it will be initialized at BossUpdate()

	willUnlockWeapon = Definition::WeaponKind::Buster; // Reset

	Bullet::Admin::Get().ClearInstances();

	// Don't touch the static generated instance
	Item::Admin::Get().RemoveDynamicInstances();
}
void SceneGame::UninitStage()
{
	if ( pMap			) { pMap->Uninit();				}
	if ( pHouse			) { pHouse->Uninit();			}
	if ( pBossContainer	) { pBossContainer->Uninit();	}
	if ( pPlayer		) { pPlayer->Uninit();			}
	pClearEvent.reset();
	pBossContainer.reset();
	pHouse.reset();
	pPlayer.reset();

	pSkullMeter.reset();

	// Don't reset the "pMap" because for holding the model of Map

	Bullet::Admin::Get().ClearInstances();
	Enemy::Admin::Get().ClearInstances();
	
	// Don't touch the static generated instance
	Item::Admin::Get().RemoveDynamicInstances();
}

void SceneGame::AssignCurrentInput()
{
	const auto &deadZone = FetchParameter().deadZone;
	currentInput = Input::MakeCurrentInput( controller, deadZone );
}

float SceneGame::PauseUpdate( float elapsedTime )
{
	float updateSpeedFactor = 1.0f;

	if ( !pPauser )
	{
		const bool pausable =
			pPlayer && !pPlayer->NowMiss() &&
			!Fader::Get().IsExist() &&
			status != State::Clear && status != State::WaitToFade
			;
		if ( Input::IsPauseRequested( controller ) && pausable )
		{
			BeginPause();
			updateSpeedFactor = 0.0f;
		}
		
		/*
		I must skip the frame that beginning timing of pause.
		The beginning and resume trigger(IsPauseRequested()) is shared between here and the PauseProcessor,
		so if did not skip here, the below process will gets the shared trigger,  and resume the pause.
		*/

		return updateSpeedFactor;
	}
	// else

	updateSpeedFactor = 0.0f;

	using Cmd = PauseProcessor::Result::Command;

	const auto result = pPauser->Update( elapsedTime, controller );
	if ( result.command == Cmd::Noop ) { return updateSpeedFactor; }
	// else

	switch ( result.command )
	{
	case Cmd::Resume:
		{
			EndPause();
			updateSpeedFactor = 1.0f;
		}
		break;
	case Cmd::ChangeScene:
		{
			EndPause();
			StartFade( result.nextScene );
			updateSpeedFactor = 1.0f;

			if ( pPlayer ) { pPlayer->PerformLeaving(); }
		}
	default:
		break;
	}

	return updateSpeedFactor;
}
void  SceneGame::BeginPause()
{
	pPauser = std::make_unique<PauseProcessor>();
	pPauser->Init( currentPlayingBGM );

	Donya::Sound::Play( Music::UI_Decide );
}
void  SceneGame::EndPause()
{
	if ( !pPauser ) { return; }
	// else

	pPauser->Uninit();
	pPauser.reset();

	Donya::Sound::Play( Music::UI_Decide );
}

bool SceneGame::IsPlayingStatus( State verify ) const
{
	return ( status == State::Stage || status == State::VSBoss );
}

void SceneGame::FirstInitStateUpdate( float elapsedTime )
{
	if ( status != State::FirstInitialize ) { return; }
	// else

	if ( !thObjects.result.Finished() ) { return; }
	// else

	thObjects.JoinThenRelease();

#if USE_IMGUI
	if ( dontFinishLoadState ) { return; }
	// else
#endif // USE_IMGUI

	status = State::Stage;
	loadPerformer.Stop();

	PlayBGM( currentPlayingBGM );
}

void SceneGame::StageStateUpdate( float elapsedTime )
{
	if ( status != State::Stage ) { return; }
	// else

	// If touch to a door when the player do not pass-through-ing a door.
	// It process is not have the relation to an appearance of boss, so it is ok if the next room has a boss.
	if ( !pThroughingDoor && pDoors && pPlayer )
	{
		const auto pl = pPlayer->GetHitBox();
		pThroughingDoor = pDoors->FetchDetectedDoorOrNullptr( pl );
		if ( pThroughingDoor )
		{
			pThroughingDoor->Open();

			const auto throughDir	= pThroughingDoor->GetThroughDirection();
			const auto throughVec	= Donya::Vector3{ Definition::ToUnitVector( throughDir ), 0.0f };

			// Save the destination to far as same as the distance between current player position and the door position
			const auto doorBody = pThroughingDoor->GetBody();
			// The difference direction is same as throughing direction
			const auto diff		= doorBody.pos - pPlayer->GetPosition();
			const auto projDiff	= Donya::Vector3::Projection( diff, throughVec );
			const auto projSize = Donya::Vector3::Projection( doorBody.size, throughVec );

			const auto toBackVec	= projDiff + projSize;
			doorPassedPlayerPos		= doorBody.pos + toBackVec;
		}
	}

	if ( isThereBoss && pBossContainer )
	{
		AppearBossStateInit();
	}
}

void SceneGame::AppearBossStateInit()
{
	status				= State::AppearBoss;
	willUnlockWeapon	= Definition::WeaponKind::Buster; // Init
	FadeOutBGM();
}
void SceneGame::AppearBossStateUpdate( float elapsedTime )
{
	if ( status != State::AppearBoss	) { return; }
	if ( scroll.active					) { return; }
	// else

	if ( !pBossContainer )
	{
		// Fail safe.
		// The pBossContainer's StartupBossIfStandby() will make the meter, the meter will be condition of change the state.
		// So if the pBossContainer is not exist, the game will stop.

		_ASSERT_EXPR( 0, L"Error: The boss admin is invalid!" );
		VSBossStateInit( Definition::WeaponKind::Buster );
		return;
	}
	// else

	const Donya::Vector3 playerPos   = pPlayer->GetPosition();
	const Donya::Vector3 destination = MakeBossRoomInitialPosOf( currentRoomID );

	const auto prevSign = Donya::SignBit( destination.x - prevPlayerPos.x );
	const auto currSign = Donya::SignBit( destination.x - playerPos.x );

	// Arrive to destination
	if ( currSign != prevSign )
	{
		pBossContainer->StartupBossIfStandby( currentRoomID );
		Donya::Sound::Play( Music::Performance_AppearBoss );
		
		// Only generate the meter
		pSkullMeter = std::make_unique<Meter::Drawer>();
		pSkullMeter->Init( 0.0f, 0.0f, 0.0f );

		const auto &data = FetchParameter();
		pSkullMeter->SetDrawOption( data.skullMeter.hpDrawPos, data.skullMeter.hpDrawColor, data.skullMeter.hpDrawScale );
	}

	const auto pBoss = pBossContainer->GetBossOrNullptr( currentRoomID );
	if ( pBoss )
	{
		if ( pBoss->NowRecoverHPTiming() )
		{
			if ( !pSkullMeter ) { pSkullMeter = std::make_unique<Meter::Drawer>(); }

			// TODO: If create another boss kind, fix this
			const float maxHP = scast<float>( Boss::Parameter::GetSkull().hp );
			pSkullMeter->Init( maxHP, 0.0f, maxHP );

			const auto &data = FetchParameter();
			pSkullMeter->SetDrawOption( data.skullMeter.hpDrawPos, data.skullMeter.hpDrawColor, data.skullMeter.hpDrawScale );
		}

		if ( !pBoss->NowAppearing() )
		{
			VSBossStateInit( pBoss->GetUsingWeapon() );
		}
	}
}

void SceneGame::VSBossStateInit( Definition::WeaponKind bossWeapon )
{
	status				= State::VSBoss;
	willUnlockWeapon	= bossWeapon;
	PlayBGM( Music::BGM_Boss );
}
void SceneGame::VSBossStateUpdate( float elapsedTime )
{
	if ( status != State::VSBoss	) { return; }
	if ( !isThereBoss				) { return; }
	// else

	if ( !pBossContainer )
	{
		// Fail safe.
		// The pBossContainer's StartupBossIfStandby() will make the meter, the meter will be condition of change the state.
		// So if the pBossContainer is not exist, the game will stop.

		_ASSERT_EXPR( 0, L"Error: The boss admin is invalid!" );
		if ( isThereClearEvent )
		{
			ClearStateInit();
		}
		else
		{
			status = State::Stage;
			FadeOutBGM();
		}

		return;
	}
	// else

	if ( !pBossContainer->IsAliveIn( currentRoomID ) )
	{
		isThereBoss = false;

		if ( pPlayer )
		{
			pPlayer->ApplyAvailableWeapon( willUnlockWeapon );
		}
		auto &saveData = SaveData::Admin::Get();
		saveData.Write( willUnlockWeapon );
		saveData.Save();
		willUnlockWeapon = Definition::WeaponKind::Buster; // Reset

		if ( isThereClearEvent )
		{
			ClearStateInit();
		}
		else
		{
			status = State::Stage;
			FadeOutBGM();
		}

		return;
	}
	// else
}

void SceneGame::ClearStateInit()
{
	status		= State::Clear;
	clearTimer	= 0.0f;
	wantLeave	= false;

	FadeOutBGM();
}
void SceneGame::ClearStateUpdate( float elapsedTime )
{
	if ( status != State::Clear && status != State::WaitToFade ) { return; }
	// else

	const auto  &data		= FetchParameter();
	const float secPlaySE	= data.waitSec_PlayClearSE;
	const float secFirst	= data.waitSecBet_ClearLeave;
	const float secSecond	= data.waitSecBet_LeaveFade + secFirst;

	const float prevTimer = clearTimer;
	clearTimer += elapsedTime;

	if ( secPlaySE <= clearTimer )
	{
		const int currSign = Donya::SignBit( secPlaySE - clearTimer );
		const int prevSign = Donya::SignBit( secPlaySE - prevTimer  );
		if ( currSign != prevSign )
		{
			Donya::Sound::Play( Music::Performance_ClearStage );
		}
	}
	
	if ( !wantLeave && secFirst <= clearTimer )
	{
		wantLeave = true;
	}

	if ( !Fader::Get().IsExist() && secSecond <= clearTimer )
	{
		StartFade( Scene::Type::Result );
	}
}

void SceneGame::CameraInit()
{
	const auto &data = FetchParameter();

	constexpr Donya::Vector2 screenSize{ Common::ScreenWidthF(), Common::ScreenHeightF() };
	constexpr Donya::Vector2 defaultZRange{ 0.1f, 500.0f };

	iCamera.Init				( Donya::ICamera::Mode::Look );
	iCamera.SetZRange			( defaultZRange.x, defaultZRange.y );
	iCamera.SetFOV				( ToRadian( data.camera.fovDegree ) );
	iCamera.SetScreenSize		( screenSize );

	lightCamera.Init			( Donya::ICamera::Mode::Look );
	lightCamera.SetZRange		( data.shadowMap.nearDistance, data.shadowMap.projectDistance.z );
	lightCamera.SetScreenSize	( data.shadowMap.projectDistance.XY() );

	AssignCameraPos();

	iCamera.SetProjectionPerspective();
	lightCamera.SetProjectionOrthographic();

	Effect::Admin::Get().SetProjectionMatrix( iCamera.GetProjectionMatrix() );

	// I can setting a configuration,
	// but current data is not changed immediately.
	// So update here.
	Donya::ICamera::Controller moveInitPoint{};
	moveInitPoint.SetNoOperation();
	moveInitPoint.slerpPercent = 1.0f;
	iCamera.Update( moveInitPoint );
	lightCamera.Update( moveInitPoint );

	scroll.active			= false;
	scroll.elapsedSecond	= 0.0f;
}
Donya::Vector3 SceneGame::ClampFocusPoint( const Donya::Vector3 &focusPoint, int roomID )
{
	if ( !pHouse ) { return focusPoint; }
	// else
		
	const auto area = pHouse->CalcRoomArea( roomID );
	if ( area == Donya::Collision::Box3F::Nil() ) { return focusPoint; }
	// else

	const auto &data = FetchParameter();

	const auto min = area.Min();
	const auto max = area.Max();

	const float halfAreaWidth		= ( max.x - min.x ) * 0.5f;
	const float halfAreaHeight		= ( max.y - min.y ) * 0.5f;
	const float halfScreenWidth		= ( currentScreen.Max().x - currentScreen.Min().x ) * 0.5f;
	const float halfScreenHeight	= ( currentScreen.Max().y - currentScreen.Min().y ) * 0.5f;
	const float halfWidth			= std::min( halfAreaWidth,  halfScreenWidth  ); // If area size smaller than screen, set to center the focus point
	const float halfHeight			= std::min( halfAreaHeight, halfScreenHeight ); // If area size smaller than screen, set to center the focus point

	Donya::Vector3 clamped = focusPoint;
	clamped += data.camera.offsetFocus; // Clamp the center pos in offseted value
	clamped.x = Donya::Clamp( clamped.x, min.x + halfWidth,  max.x - halfWidth  );
	clamped.y = Donya::Clamp( clamped.y, min.y + halfHeight, max.y - halfHeight );
	clamped.z = Donya::Clamp( clamped.z, min.z, max.z );
	clamped -= data.camera.offsetFocus; // Back to before offset pos because below setting process expects that value
	return clamped;
}
void SceneGame::PrepareScrollIfNotActive( int oldRoomID, int newRoomID )
{
	if ( scroll.active ) { return; }
	// else

	scroll.active			= true;
	scroll.elapsedSecond	= 0.0f;

	const Donya::Vector3 baseFocus = GetPlayerPosition();
	scroll.cameraFocusStart = ClampFocusPoint( baseFocus, oldRoomID );
	scroll.cameraFocusDest  = ClampFocusPoint( baseFocus, newRoomID );
}
void SceneGame::AssignCameraPos()
{
	const auto &data = FetchParameter();
	const Donya::Vector3 playerPos = GetPlayerPosition();
	
	Donya::Vector3 focusPos;
	if ( scroll.active )
	{
		const float percent = scroll.elapsedSecond / ( data.scrollTakeSecond + EPSILON );
		const Donya::Vector3 diff = scroll.cameraFocusDest - scroll.cameraFocusStart;

		focusPos = scroll.cameraFocusStart + ( diff * percent );
	}
	else
	{
		focusPos = playerPos;
		focusPos = ClampFocusPoint( focusPos, currentRoomID );
	}

	iCamera.SetPosition  ( focusPos + data.camera.offsetPos   );
	iCamera.SetFocusPoint( focusPos + data.camera.offsetFocus );
	
	const Donya::Vector3 offset = -data.shadowMap.projectDirection * data.shadowMap.offsetDistance;
	lightCamera.SetPosition  ( playerPos + offset );
	lightCamera.SetFocusPoint( playerPos );
}
void SceneGame::CameraUpdate( float elapsedTime )
{
	const auto &data = FetchParameter();

#if USE_IMGUI
	// Apply for be able to see an adjustment immediately
	{
		iCamera.SetFOV( ToRadian( data.camera.fovDegree ) );
		iCamera.SetProjectionPerspective();
		
		if ( nowDebugMode ) // Don't call AssignCameraPos() when debug-mode
		{
			const Donya::Vector3 playerPos	= GetPlayerPosition();
			const Donya::Vector3 offset		= -data.shadowMap.projectDirection * data.shadowMap.offsetDistance;
			lightCamera.SetPosition  ( playerPos + offset );
			lightCamera.SetFocusPoint( playerPos );
		}

		lightCamera.SetZRange( data.shadowMap.nearDistance, data.shadowMap.projectDistance.z );
		lightCamera.SetScreenSize( data.shadowMap.projectDistance.XY() );
		lightCamera.SetProjectionOrthographic();
	}
#endif // USE_IMGUI

	if ( scroll.active )
	{
		scroll.elapsedSecond += elapsedTime;
		if ( data.scrollTakeSecond <= scroll.elapsedSecond )
		{
			scroll.active = false;

			if ( pThroughingDoor )
			{
				pThroughingDoor->Close();
			}
		}
	}

	Donya::ICamera::Controller input{};
	input.SetNoOperation();
	input.slerpPercent = data.camera.slerpFactor;

	lightCamera.Update( input );

#if !DEBUG_MODE
	AssignCameraPos();
	iCamera.Update( input );
#else
	if ( !nowDebugMode )
	{
		AssignCameraPos();
		iCamera.Update( input );
		previousCameraPos = iCamera.GetPosition();
		return;
	}
	// else

	static Donya::Int2 prevMouse{};
	static Donya::Int2 currMouse{};

	prevMouse = currMouse;

	auto nowMouse = Donya::Mouse::Coordinate();
	currMouse.x = scast<int>( nowMouse.x );
	currMouse.y = scast<int>( nowMouse.y );

	bool  isInputMouseButton = Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) || Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) || Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT );
	bool  isDriveMouse = ( prevMouse != currMouse ) || Donya::Mouse::WheelRot() || isInputMouseButton;
	if ( !isDriveMouse )
	{
		input.SetNoOperation();
		iCamera.Update( input );
		return;
	}
	// else

	const Donya::Vector2 diff = ( currMouse - prevMouse ).Float();
	
	Donya::Vector3 movement{};
	Donya::Vector3 rotation{};

	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		if ( Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) )
		{
			constexpr float ROT_AMOUNT = ToRadian( 0.5f );
			rotation.x = diff.x * ROT_AMOUNT;
			rotation.y = diff.y * ROT_AMOUNT;

			if ( isReverseCameraRotX ) { rotation.x *= -1.0f; }
			if ( isReverseCameraRotY ) { rotation.y *= -1.0f; }
		}
	}

	// Operation that ALT key is not needed 
	{
		if ( Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) )
		{
			constexpr float MOVE_SPEED = 0.05f;
			movement.x = diff.x * MOVE_SPEED;
			movement.y = diff.y * MOVE_SPEED;

			if ( isReverseCameraMoveX ) { movement.x *= -1.0f; }
			if ( isReverseCameraMoveY ) { movement.y *= -1.0f; }
		}

		constexpr float FRONT_SPEED = 3.0f;
		movement.z = FRONT_SPEED * scast<float>( Donya::Mouse::WheelRot() );
	}

	input.moveVelocity		= movement;
	input.yaw				= rotation.x;
	input.pitch				= rotation.y;
	input.roll				= 0.0f;
	input.moveInLocalSpace	= true;

	iCamera.Update( input );

	input.SetNoOperation();

#endif // !DEBUG_MODE
}

void SceneGame::UpdateCurrentRoomID()
{
	if ( !pPlayer ) { return; }
	// else

	const int nextRoomID = CalcCurrentRoomID();
	if ( nextRoomID == currentRoomID ) { return; }
	// else


	const Room *pCurrentRoom	= pHouse->FindRoomOrNullptr( currentRoomID	);
	const Room *pNextRoom		= pHouse->FindRoomOrNullptr( nextRoomID		);
	if ( !pCurrentRoom || !pNextRoom ) { return; }
	// else


	// If allow the scroll to a connecting room, the scroll waiting will occur as strange.
	if ( pCurrentRoom->IsConnectTo( nextRoomID ) )
	{
		currentRoomID = nextRoomID;
		return;
	}
	// else


	// Here condition is "There in other room" however "I don't know can I transiton from old room to now room".
	// So check the transition-able then scroll if transiton-able.

	using Dir = Definition::Direction;
	
	const auto currentRoomArea	= pCurrentRoom->GetArea();
	const auto nextRoomArea		= pNextRoom->GetArea();
	const Donya::Vector3 delta	= nextRoomArea.pos - currentRoomArea.pos;
	const Donya::Int2 deltaSign
	{
		Donya::SignBit( delta.x ),
		Donya::SignBit( delta.y ),
	};
	Dir adjoinDir = Dir::Nil;
	if ( deltaSign.x == +1 ) { adjoinDir |= Dir::Right;	}
	if ( deltaSign.x == -1 ) { adjoinDir |= Dir::Left;	}
	if ( deltaSign.y == +1 ) { adjoinDir |= Dir::Up;	}
	if ( deltaSign.y == -1 ) { adjoinDir |= Dir::Down;	}

	const Dir ableDir = pCurrentRoom->GetTransitionableDirection();
	
	// Verify the consistency between the adjoin direction and transition-able direction
	bool hasConsistency = false;
	constexpr Dir directions[]{ Dir::Right, Dir::Left, Dir::Up, Dir::Down };
	for ( const auto &dir : directions )
	{
		if ( Contain( ableDir, dir ) )
		{
			if ( Contain( adjoinDir, dir ) )
			{
				hasConsistency = true;
			}
		}
	}
	if ( !hasConsistency ) { return; }
	// else

	// Up direction is only able to when grabbing a ladder
	if ( Contain( adjoinDir, Dir::Up ) )
	{
		// But allow if may transition to horizontally
		// If it is nothing, we can not transition to horizontally without a ladder from a room that has transition-able of [Up] and [Right] or [Left].
		bool mayHorizontal = false;
		if ( Contain( adjoinDir, Dir::Right | Dir::Left ) )
		{
			// Judge to "may transition" if "There the player in outside of old room"
			const auto  playerBody = pPlayer->GetHitBox();
			const float left  = playerBody.Min().x;
			const float right = playerBody.Max().x;

			if ( right < currentRoomArea.Min().x ) { mayHorizontal = true; }
			if ( left  > currentRoomArea.Max().x ) { mayHorizontal = true; }
		}

		if ( !mayHorizontal && !pPlayer->NowGrabbingLadder() )
		{
			return;
		}
		// else
	}

	PrepareScrollIfNotActive( currentRoomID, nextRoomID );
	currentRoomID = nextRoomID;
}

Donya::Vector4x4 SceneGame::CalcLightViewMatrix() const
{
	return lightCamera.CalcViewMatrix();
}

void SceneGame::ReadyPlayer()
{
	if ( pPlayer ) { return; }
	// else

	if ( pFxReady )
	{
		// Wait to the effect is end

		if ( !pFxReady->IsExists() )
		{
			pFxReady->Stop();
			pFxReady->Disable();
			pFxReady.reset();

			PlayerInit( playerIniter, *pMap );
		}
	}
	else
	{
		const auto &data = FetchParameter();
		if ( data.readyFxBeginDelaySecond <= stageTimer )
		{
			const Donya::Vector3 fxReadyPos = iCamera.GetFocusPoint() + data.readyFxPosOffset;
			pFxReady = std::make_unique<Effect::Handle>( Effect::Handle::Generate( Effect::Kind::READY, fxReadyPos ) );
		}
	}
}
void SceneGame::PlayerInit( const PlayerInitializer &initializer, const Map &terrain )
{
	if ( pPlayer )
	{
		pPlayer->Uninit();
		pPlayer.reset();
	}

	pPlayer = std::make_unique<Player>();
	pPlayer->Init( initializer, terrain );

	const auto  &meterData	= FetchParameter().playerMeter;
	const float maxHP		= scast<float>( Player::Parameter().Get().maxHP );
	pPlayerMeter = std::make_unique<Meter::Drawer>();
	pPlayerMeter->Init( maxHP, maxHP, maxHP );
	pPlayerMeter->SetDrawOption( meterData.hpDrawPos, pPlayer->GetThemeColor(), meterData.hpDrawScale );
}
void SceneGame::PlayerUpdate( float elapsedTime, const Map &terrain )
{
	if ( !pPlayer ) { return; }
	// else

	const Player::Input input = MakePlayerInput( elapsedTime );

	const bool prevIsWinning = pPlayer->NowWinningPose();
	pPlayer->Update( elapsedTime, input, terrain );
	const bool currIsWinning = pPlayer->NowWinningPose();

	if ( status == State::WaitToFade )
	{
		if ( !currIsWinning && prevIsWinning )
		{
			pPlayer->PerformLeaving();
		}
	}

	if ( pPlayer->NowMiss() )
	{
		// First frame when the player missing
		if ( IsZero( elapsedSecondsAfterMiss ) )
		{
			const int remaining = Player::Remaining::Get();
			if ( remaining <= 0 )
			{
				FadeOutBGM();
			}
		}

		elapsedSecondsAfterMiss += elapsedTime;
	}
	else
	{
		elapsedSecondsAfterMiss = 0.0f;
	}

	if ( pPlayerMeter )
	{
		const auto &meterData = FetchParameter().playerMeter;
		
		pPlayerMeter->SetDestination( scast<float>( pPlayer->GetCurrentHP() ) );
		pPlayerMeter->SetDrawOption( meterData.hpDrawPos, pPlayer->GetThemeColor(), meterData.hpDrawScale );
	}
}
void SceneGame::PlayerPhysicUpdate( float elapsedTime, const Map &terrain )
{
	if ( !pPlayer ) { return; }
	// else

	using Dir = Definition::Direction;

	const Room *pCurrentRoom	= pHouse->FindRoomOrNullptr( currentRoomID );
	const auto currentRoomArea	= ( pCurrentRoom )
								? pHouse->CalcRoomArea( currentRoomID )
								: currentScreen;	// Fail safe
	const auto transitionable	= ( pCurrentRoom )
								? pCurrentRoom->GetTransitionableDirection()
								: Dir::Nil;			// Fail safe
	const float leftBorder	= Contain( transitionable, Dir::Left )
							? -FLT_MAX
							: std::min( currentRoomArea.Min().x, currentScreen.Min().x );
	const float rightBorder	= Contain( transitionable, Dir::Right )
							? FLT_MAX
							: std::max( currentRoomArea.Max().x, currentScreen.Max().x );

	prevPlayerPos = pPlayer->GetPosition();
	pPlayer->PhysicUpdate( elapsedTime, terrain, leftBorder, rightBorder );
}
Donya::Vector3 SceneGame::GetPlayerPosition() const
{
	return ( pPlayer ) ? pPlayer->GetPosition() : playerIniter.GetWorldInitialPos();
}
Donya::Vector3 SceneGame::MakeBossRoomInitialPosOf( int roomId ) const
{
	const auto  playerPos		= pPlayer->GetPosition();
	const float playerLookSign	= Donya::SignBitF( pPlayer->GetOrientation().LocalFront().x );
	const Room *pCurrentRoom	= ( pHouse ) ? pHouse->FindRoomOrNullptr( currentRoomID ) : nullptr;

	Donya::Vector3 destination;
	if ( pCurrentRoom )
	{
		const auto room = pCurrentRoom->GetArea();
		destination = ( playerLookSign < 0.0f ) ? room.Max() : room.Min();
	}
	else
	{
		destination = playerPos;
	}
		
	destination.x += FetchParameter().bossRoomInitialPosOffset * playerLookSign;
	destination.y = playerPos.y;

	return destination;
}
Player::Input  SceneGame::MakePlayerInput( float elapsedTime )
{
	Player::Input input{};

	if ( status == State::AppearBoss )
	{
		const Donya::Vector3 destination = MakeBossRoomInitialPosOf( currentRoomID );

		input.headToDestination	= true;
		input.wsDestination		= destination;
		input.useShots			= currentInput.useShots; // Allow the charging
	}
	else
	if ( status == State::Clear )
	{
		if ( wantLeave )
		{
			// Discard the elapsed time when heading to destination
			clearTimer -= elapsedTime;

			const Donya::Vector3 playerPos	= pPlayer->GetPosition();
			const Room		*pCurrentRoom	= ( pHouse ) ? pHouse->FindRoomOrNullptr( currentRoomID ) : nullptr;

			Donya::Vector3	destination		= ( pCurrentRoom ) ? pCurrentRoom->GetArea().pos : playerPos;
			destination.y = playerPos.y;

			const auto prevSign = Donya::SignBit( destination.x - prevPlayerPos.x );
			const auto currSign = Donya::SignBit( destination.x - playerPos.x );

			// Arrive to destination
			if ( currSign != prevSign || !currSign )
			{
				pPlayer->PerformWinning();
				status = State::WaitToFade;
			}
			// Head to initial position
			else
			{
				input.headToDestination = true;
				input.wsDestination = destination;
			}
		}
	}
	else
	if ( status == State::WaitToFade )
	{
		// No op
	}
	else
	{
		if ( NowThroughingDoor() )
		{
			auto destination = doorPassedPlayerPos;
			destination.y = pPlayer->GetPosition().y;

			input.headToDestination	= true;
			input.wsDestination		=
				( pThroughingDoor->NowOpenMotion() && pThroughingDoor->NowPlayingAnimation() )
				? pPlayer->GetPosition() // Stay until fully open
				: destination;
			input.useShots = currentInput.useShots; // Allow the charging
		}
		else
		{
			input = currentInput;
		}
	}

	return input;
}

void SceneGame::UpdatePlayerIniter()
{
	if ( !pPlayer ) { return; }
	// else

	const auto pArea = checkPoint.FetchPassedPointOrNullptr( pPlayer->GetHitBox() );
	if ( !pArea ) { return; }
	// else

	playerIniter.AssignParameter( pArea->GetWorldInitialPos(), pArea->ShouldLookingRight() );

	pArea->Deactivate();
}

void SceneGame::DoorUpdate()
{
	if ( !pThroughingDoor ) { return; }
	// else

	if ( !pThroughingDoor->NowOpen() && !pThroughingDoor->NowPlayingAnimation() )
	{
		pThroughingDoor = nullptr;
	}
}
bool SceneGame::NowThroughingDoor() const
{
	return pThroughingDoor;
}

void SceneGame::BossUpdate( float elapsedTime, const Donya::Vector3 &wsTargetPos )
{
	if ( !pBossContainer ) { return; }
	// else

	Boss::Input input{};
	input.wsTargetPos					= wsTargetPos;
	if ( status == State::VSBoss )
	{
		input.controllerInputDirection	= currentInput.moveVelocity;
		input.pressJump					= Input::HasTrue( currentInput.useJumps );
		input.pressShot					= Input::HasTrue( currentInput.useShots );
	}

	pBossContainer->Update( elapsedTime, input );

	const auto pBoss = pBossContainer->GetBossOrNullptr( currentRoomID );
	if ( pBoss )
	{
		const float currentBossHP = scast<float>( pBoss->GetCurrentHP() );

		if ( !pSkullMeter )
		{
			pSkullMeter = std::make_unique<Meter::Drawer>();

			// TODO: If create another boss kind, fix this
			const float maxHP = scast<float>( Boss::Parameter::GetSkull().hp );
			pSkullMeter->Init( maxHP, 0.0f, currentBossHP );

			const auto &data = FetchParameter();
			pSkullMeter->SetDrawOption( data.skullMeter.hpDrawPos, data.skullMeter.hpDrawColor, data.skullMeter.hpDrawScale );
		}

		pSkullMeter->SetDestination( currentBossHP );
	}
}

int  SceneGame::CalcCurrentRoomID() const
{
	if ( !pHouse ) { return currentRoomID; }
	// else

	const Donya::Vector3 playerPos = GetPlayerPosition();
	const int newID =  pHouse->CalcBelongRoomID( playerPos );
	return (  newID == Room::invalidID ) ? currentRoomID : newID;
}

namespace
{
	Donya::Collision::IDType ExtractPlayerID( const std::unique_ptr<Player> &pPlayer )
	{
		return ( pPlayer ) ? pPlayer->GetHurtBox().id : Donya::Collision::invalidID;
	}
	bool IsPlayerBullet( const Donya::Collision::IDType playerCollisionID, const std::shared_ptr<const Bullet::Base> &pBullet )
	{
		if ( playerCollisionID == Donya::Collision::invalidID ) { return false; }
		// else

		const auto bulletAABB	= pBullet->GetHitBox();
		const auto bulletSphere	= pBullet->GetHitSphere();
		const auto activeOwnerID= ( bulletSphere.exist ) ? bulletSphere.ownerID : bulletAABB.ownerID;
		return ( activeOwnerID == playerCollisionID ) ? true : false;
	}
	bool IsEnemyBullet( const Donya::Collision::IDType playerCollisionID, const std::shared_ptr<const Bullet::Base> &pBullet )
	{
		return !IsPlayerBullet( playerCollisionID, pBullet );
	}

	enum class SubtractorState
	{
		Not,
		HasBox,
		HasSphere
	};
	SubtractorState HasSubtractor( const std::shared_ptr<const Bullet::Base> &pBullet )
	{
		using State = SubtractorState;

		if ( !pBullet ) { return State::Not; }
		// else

		if ( pBullet->GetHitBoxSubtractor().exist ) { return State::HasBox; }
		// else

		if ( pBullet->GetHitSphereSubtractor().exist ) { return State::HasSphere; }
		// else

		return State::Not;
	}

	template<typename ObjectHitBox, typename BulletHitBox>
	bool IsHit( const ObjectHitBox &objHitBox, const std::shared_ptr<const Bullet::Base> &pBullet, const BulletHitBox &bulletHitBox, bool considerExistFlag = true )
	{
		namespace Col = Donya::Collision;

		const SubtractorState hasSubtract = HasSubtractor( pBullet );

		// Do not use subtraction version
		if ( hasSubtract == SubtractorState::Not )
		{
			return Col::IsHit( objHitBox, bulletHitBox, considerExistFlag );
		}
		// else

		// Can not use subtraction version
		if ( !pBullet )
		{
			return Col::IsHit( objHitBox, bulletHitBox, considerExistFlag );
		}
		// else

		auto IsHitImpl = [&]( const auto &hitBoxSubtractor )
		{
			const Col::Solid<BulletHitBox, decltype( hitBoxSubtractor )> a
			{
				bulletHitBox,
				hitBoxSubtractor
			};
			return Col::IsHitVSSubtracted( objHitBox, a, considerExistFlag );
		};

		if ( hasSubtract == SubtractorState::HasBox )
		{
			return IsHitImpl( pBullet->GetHitBoxSubtractor() );
		}
		// else
		if ( hasSubtract == SubtractorState::HasSphere )
		{
			return IsHitImpl( pBullet->GetHitSphereSubtractor() );
		}
		// else

		_ASSERT_EXPR( 0, L"Error: Unexpected condition!" );
		return false;
	}
	template<typename HitBoxA, typename HitBoxB>
	bool IsHit( const std::shared_ptr<const Bullet::Base> &pBulletA, const HitBoxA &hitBoxA, const std::shared_ptr<const Bullet::Base> &pBulletB, const HitBoxB &hitBoxB, bool considerExistFlag = true )
	{
		if ( !pBulletA || !pBulletB ) { return false; }
		// else

		namespace Col = Donya::Collision;

		const SubtractorState subA = HasSubtractor( pBulletA );
		const SubtractorState subB = HasSubtractor( pBulletB );

		// Do not use subtraction version
		if ( subA == SubtractorState::Not && subB == SubtractorState::Not )
		{
			return Col::IsHit( hitBoxA, hitBoxB );
		}
		// else
		
		// Do not support yet
		if ( subA != SubtractorState::Not && subB != SubtractorState::Not )
		{
			return Col::IsHit( hitBoxA, hitBoxB );
		}
		// else

		if ( subA != SubtractorState::Not )
		{
			return IsHit( hitBoxB, pBulletA, hitBoxA, considerExistFlag );
		}
		// else
		if ( subB != SubtractorState::Not )
		{
			return IsHit( hitBoxA, pBulletB, hitBoxB, considerExistFlag );
		}
		// else
		_ASSERT_EXPR( 0, L"Error: Unexpected condition!" );
		return false;
	}
}
void SceneGame::Collision_BulletVSBullet()
{
	auto &bulletAdmin = Bullet::Admin::Get();
	const size_t bulletCount = bulletAdmin.GetInstanceCount();

	std::shared_ptr<const Bullet::Base> pA = nullptr;
	std::shared_ptr<const Bullet::Base> pB = nullptr;
	auto Protectible	= []( const std::shared_ptr<const Bullet::Base> &pBullet )
	{
		using D = Definition::Damage;
		return ( pBullet ) ? D::Contain( D::Type::Protection, pBullet->GetDamage().type ) : false;
	};
	auto HitProcess		= [&]( const auto &hitBoxA, const auto &hitBoxB )
	{
		if ( !pA || !pB ) { return; }
		// else

		const bool protectibleA = Protectible( pA );
		const bool protectibleB = Protectible( pB );
		if ( protectibleA ) { pB->ProtectedBy( hitBoxA ); }
		if ( protectibleB ) { pA->ProtectedBy( hitBoxB ); }

		// Currently, there is not the hard(e.g. destructible and a lot of HP) bullet.
		// So tells "pierced" to the collided bullet(s) for easily process, in the collision between bullet and bullet.
		constexpr bool toPierce = true;
		
		// Do not call the protected bullet's method.
		// But should call the not protected one's method for play the hit SE.
		if ( !protectibleA ) { pB->CollidedToObject( toPierce, /* otherIsBullet = */ true ); }
		if ( !protectibleB ) { pA->CollidedToObject( toPierce, /* otherIsBullet = */ true ); }
	};
	
	const auto playerID = ExtractPlayerID( pPlayer );

	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pA = bulletAdmin.GetInstanceOrNullptr( i );
		if ( !pA ) { continue; }
		// else

		// Disallow collision between a protected bullet.
		// Because if allowed hitting to multiple objects in the same timing,
		// the protection attribute does not affect to some another object that collided in the same timing.
		// That bullet's collision will be disabled at next update, but I wanna apply immediately.
		if ( pA->WasProtected() ) { continue; }
		// else

		const auto aabbA		= pA->GetHitBox();
		const auto sphereA		= pA->GetHitSphere();

		const bool ownerA		= IsPlayerBullet( playerID, pA );
		const bool protectibleA	= Protectible( pA );

		for ( size_t j = i + 1; j < bulletCount; ++j )
		{
			pB = bulletAdmin.GetInstanceOrNullptr( j );
			if ( !pB )					{ continue; }
			if ( pB->WasProtected() )	{ continue; }
			// else

			const bool ownerB		= IsPlayerBullet( playerID, pB );
			const bool protectibleB	= Protectible( pB );
			if ( ownerA == ownerB ) { continue; }
			// else

			// Do collide if either one of bullet is destructible or protectible
			const bool wantCollide	=  pA->Destructible()	|| pB->Destructible()
									|| protectibleA			|| protectibleB;
			if ( !wantCollide ) { continue; }
			// else
			
			const auto aabbB	= pB->GetHitBox();
			const auto sphereB	= pB->GetHitSphere();

			if ( aabbA.exist )
			{
				if ( IsHit( pA, aabbA, pB, aabbB ) )
				{
					HitProcess( aabbA, aabbB );
					continue;
				}
				// else
				if ( IsHit( pA, aabbA, pB, sphereB ) )
				{
					HitProcess( aabbA, sphereB );
					continue;
				}
			}
			else
			if ( sphereA.exist )
			{
				if ( IsHit( pA, sphereA, pB, aabbB ) )
				{
					HitProcess( sphereA, aabbB );
					continue;
				}
				// else
				if ( IsHit( pA, sphereA, pB, sphereB ) )
				{
					HitProcess( sphereA, sphereB );
					continue;
				}
			}
		}
	}
}
void SceneGame::Collision_BulletVSBoss()
{
	if ( !pBossContainer || !isThereBoss ) { return; }
	// else

	auto  pBoss = pBossContainer->GetBossOrNullptr( currentRoomID );
	if ( !pBoss ) { return; }
	// else

	const auto bossBody = pBoss->GetHurtBox();

	auto &bulletAdmin = Bullet::Admin::Get();
	const size_t bulletCount = bulletAdmin.GetInstanceCount();

	std::shared_ptr<const Bullet::Base> pBullet = nullptr;
	auto HitProcess = [&]()
	{
		if ( !pBullet ) { return; }
		// else

		if ( pBoss->NowProtecting() )
		{
			pBullet->ProtectedBy( bossBody );
		}
		else
		{
			pBoss->GiveDamage( pBullet->GetDamage() );
			pBullet->CollidedToObject( pBoss->WillDie(), /* otherIsBullet = */ false );
		}
	};

	const auto playerID = ExtractPlayerID( pPlayer );

	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pBullet = bulletAdmin.GetInstanceOrNullptr( i );
		if ( !pBullet ) { continue; }
		// else
		if ( pBullet->WasProtected() ) { continue; }
		// else

		if ( !IsPlayerBullet( playerID, pBullet ) ) { continue; }
		// else

		// The bullet's hit box is only either AABB or Sphere is valid
		// (Invalid one's exist flag will false, so IsHit() will returns false)

		if ( IsHit( bossBody, pBullet, pBullet->GetHitBox() ) )
		{
			HitProcess();
			continue;
		}
		// else

		if ( IsHit( bossBody, pBullet, pBullet->GetHitSphere() ) )
		{
			HitProcess();
			continue;
		}
	}
}
void SceneGame::Collision_BulletVSEnemy()
{
	auto &enemyAdmin	= Enemy::Admin::Get();
	auto &bulletAdmin	= Bullet::Admin::Get();
	const size_t enemyCount		= enemyAdmin.GetInstanceCount();
	const size_t bulletCount	= bulletAdmin.GetInstanceCount();

	// Makes every call the "FindCollidingEnemyOrNullptr" returns another enemy
	std::vector<size_t> collidedEnemyIndices{};
	auto IsAlreadyCollided				= [&]( size_t enemyIndex )
	{
		const auto result = std::find( collidedEnemyIndices.begin(), collidedEnemyIndices.end(), enemyIndex );
		return ( result != collidedEnemyIndices.end() );
	};
	auto FindCollidingEnemyOrNullptr	= [&]( const std::shared_ptr<const Bullet::Base> &pOtherBullet, const auto &otherHitBox )
	{
		std::shared_ptr<const Enemy::Base> pEnemy = nullptr;
		for ( size_t i = 0; i < enemyCount; ++i )
		{
			if ( IsAlreadyCollided( i ) ) { continue; }
			// else

			pEnemy = enemyAdmin.GetInstanceOrNullptr( i );
			if ( !pEnemy ) { continue; }
			// else

			if ( IsHit( pEnemy->GetHurtBox(), pOtherBullet, otherHitBox ) )
			{
				collidedEnemyIndices.emplace_back( i );
				return pEnemy;
			}
		}

		pEnemy = nullptr;
		return pEnemy;
	};

	// The bullet's hit box is only either AABB or Sphere is valid
	// (Invalid one's exist flag will false, so IsHit() will returns false)
	Donya::Collision::Box3F		otherAABB;
	Donya::Collision::Sphere3F	otherSphere;

	std::shared_ptr<const Bullet::Base> pBullet = nullptr;
	std::shared_ptr<const Enemy::Base>  pOther  = nullptr;

	struct ProccessResult
	{
		bool collided	= false;
		bool pierced	= false;
	};
	auto Process = [&]( const auto &bulletBody )
	{
		ProccessResult result{};
		result.collided = false;
		result.pierced  = false;

		if ( !pBullet ) { return result; }
		// else

		result.pierced = true;

		pOther = FindCollidingEnemyOrNullptr( pBullet, bulletBody );
		while ( pOther )
		{
			result.collided = true;

			pOther->GiveDamage( pBullet->GetDamage() );
			if ( pOther->WillDie() )
			{
				Item::DropItemByLottery( pOther->GetPosition() );
			}
			else
			{
				result.pierced = false;
			}

			pOther = FindCollidingEnemyOrNullptr( pBullet, bulletBody );
		}

		return result;
	};

	const auto playerID = ExtractPlayerID( pPlayer );

	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pBullet = bulletAdmin.GetInstanceOrNullptr( i );
		if ( !pBullet ) { continue; }
		// else
		if ( pBullet->WasProtected() ) { continue; }
		// else

		if ( IsEnemyBullet( playerID, pBullet ) ) { continue; }
		// else

		otherAABB	= pBullet->GetHitBox();
		otherSphere	= pBullet->GetHitSphere();
		if ( !otherAABB.exist && !otherSphere.exist ) { continue; }
		// else

		const auto result	= ( otherAABB.exist )
							? Process( otherAABB	)
							: Process( otherSphere	);
		if ( result.collided )
		{
			pBullet->CollidedToObject( result.pierced, /* otherIsBullet = */ false );
		}

		collidedEnemyIndices.clear();
	}
}
void SceneGame::Collision_BulletVSPlayer()
{
	if ( !pPlayer || pPlayer->NowMiss()	) { return; }
	if ( !IsPlayingStatus( status )		) { return; } // Ignore if cleared
	if ( NowThroughingDoor()			) { return; } // Ignore when throughing a door, because that case can not control the player.
	// else

	const auto playerBody = pPlayer->GetHurtBox();

	auto &bulletAdmin = Bullet::Admin::Get();
	const size_t bulletCount = bulletAdmin.GetInstanceCount();

	// The bullet's hit box is only either AABB or Sphere is valid
	// (Invalid one's exist flag will false, so IsHit() will returns false)
	Donya::Collision::Box3F		bulletAABB;
	Donya::Collision::Sphere3F	bulletSphere;

	const auto playerID = ExtractPlayerID( pPlayer );

	std::shared_ptr<const Bullet::Base> pBullet = nullptr;
	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pBullet = bulletAdmin.GetInstanceOrNullptr( i );
		if ( !pBullet ) { continue; }
		// else
		if ( pBullet->WasProtected() ) { continue; }
		// else

		if ( IsPlayerBullet( playerID, pBullet ) ) { continue; }
		// else


		bulletAABB = pBullet->GetHitBox();
		if ( IsHit( playerBody, pBullet, bulletAABB ) )
		{
			pPlayer->GiveDamage( pBullet->GetDamage(), bulletAABB );
			pBullet->CollidedToObject( pPlayer->WillDie(), /* otherIsBullet = */ false );
			continue;
		}
		// else

		bulletSphere = pBullet->GetHitSphere();
		if ( IsHit( playerBody, pBullet, bulletSphere ) )
		{
			pPlayer->GiveDamage( pBullet->GetDamage(), bulletSphere );
			pBullet->CollidedToObject( pPlayer->WillDie(), /* otherIsBullet = */ false );
		}
	}
}
void SceneGame::Collision_BossVSPlayer()
{
	if ( !pPlayer || pPlayer->NowMiss()		)  { return; }
	if ( !pBossContainer || !isThereBoss	) { return; }
	if ( !IsPlayingStatus( status )			) { return; } // Ignore if cleared
	// else

	auto  pBoss = pBossContainer->GetBossOrNullptr( currentRoomID );
	if ( !pBoss ) { return; }
	// else

	const auto playerBody	= pPlayer->GetHurtBox();
	const auto bossBody		= pBoss->GetHitBox();

	if ( Donya::Collision::IsHit( playerBody, bossBody ) )
	{
		pPlayer->GiveDamage( pBoss->GetTouchDamage(), bossBody );
	}
}
void SceneGame::Collision_EnemyVSPlayer()
{
	if ( !pPlayer || pPlayer->NowMiss()	) { return; }
	if ( !IsPlayingStatus( status )		) { return; } // Ignore if cleared
	if ( NowThroughingDoor()			) { return; } // Ignore when throughing a door, because that case can not control the player.
	// else

	const auto playerBody	= pPlayer->GetHurtBox();

	auto  &enemyAdmin		= Enemy::Admin::Get();
	const size_t enemyCount	= enemyAdmin.GetInstanceCount();

	Donya::Collision::Box3F other;
	std::shared_ptr<const Enemy::Base> pEnemy = nullptr;
	for ( size_t i = 0; i < enemyCount; ++i )
	{
		pEnemy = enemyAdmin.GetInstanceOrNullptr( i );
		if ( !pEnemy ) { continue; }
		// else

		other = pEnemy->GetHitBox();
		if ( Donya::Collision::IsHit( other, playerBody ) )
		{
			pPlayer->GiveDamage( pEnemy->GetTouchDamage(), other );
		}
	}
}
void SceneGame::Collision_PlayerVSItem()
{
	if ( !pPlayer || pPlayer->NowMiss() ) { return; }
	// else

	const auto playerBody = pPlayer->GetHurtBox();

	auto &itemAdmin = Item::Admin::Get();
	const size_t itemCount = itemAdmin.GetInstanceCount();

	auto CatchItem = [&]( const Item::Item *pItem )
	{
		if ( !pItem ) { return; }
		// else

		const auto &itemParameter = Item::Parameter::GetItem();

		const auto itemKind = pItem->GetKind();

		pItem->WasCaught();

		Effect::Admin::Get().GenerateInstance( Effect::Kind::CatchItem, pItem->GetPosition() );

		switch ( itemKind )
		{
		case Item::Kind::ExtraLife:
			Player::Remaining::Increment();
			return;
		case Item::Kind::LifeEnergy_Big:
			pPlayer->RecoverHP( itemParameter.lifeEnergyBig.recoveryAmount );
			return;
		case Item::Kind::LifeEnergy_Small:
			pPlayer->RecoverHP( itemParameter.lifeEnergySmall.recoveryAmount );
			return;
		default: return;
		}
	};

	Donya::Collision::Box3F other;
	for ( size_t i = 0; i < itemCount; ++i )
	{
		const Item::Item *pItem = itemAdmin.GetInstanceOrNullptr( i );
		if ( !pItem ) { continue; }
		// else

		other = pItem->GetHitBox();
		if ( Donya::Collision::IsHit( other, playerBody, /* considerExistFlag = */ false ) )
		{
			CatchItem( pItem );
		}
	}
}

void SceneGame::ClearBackGround() const
{
	constexpr Donya::Vector3 gray = Donya::Color::MakeColor( Donya::Color::Code::GRAY );
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );

	if ( status == State::FirstInitialize ) { return; }
	// else

	RenderingStuffInstance::Get().ClearBuffers();

#if DEBUG_MODE
	if ( nowDebugMode )
	{
		constexpr Donya::Vector3 teal = Donya::Color::MakeColor( Donya::Color::Code::CYAN );
		constexpr FLOAT DEBUG_COLOR[4]{ teal.x, teal.y, teal.z, 1.0f };
		Donya::ClearViews( DEBUG_COLOR );
	}
#endif // DEBUG_MODE
}
void SceneGame::StartFade( Scene::Type nextSceneType )
{
	const auto color =
		  ( nextSceneType == Scene::Type::Result )
		? Donya::Color::Code::WHITE
		: ( nextSceneType == Scene::Type::Over )
		? Donya::Color::Code::GRAY
		: Donya::Color::Code::BLACK;
	nextScene = nextSceneType;

	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeSecond	= Fader::GetDefaultCloseSecond();
	config.SetColor( color );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneGame::ReturnResult()
{
	// The case of Scene::Type::Game will process at Update()
	if ( Fader::Get().IsClosed() && nextScene != Scene::Type::Null && nextScene != Scene::Type::Game )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = nextScene;
		return change;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
#include <functional>
#include "Donya/Useful.h"
#include "Effect/Effect.h"
#include "Effect/EffectKind.h"
namespace
{
	class  GuiWindow
	{
	public:
		Donya::Vector2 pos;		// Left-Top
		Donya::Vector2 offset;
		Donya::Vector2 size;	// Whole size
	public:
		GuiWindow()
			: pos( 0.0f, 0.0f ), offset( 0.0f, 32.0f ), size( 256.0f, 128.0f )
		{}
		GuiWindow( const Donya::Vector2 &pos, const Donya::Vector2 &size )
			: pos( pos ), offset( 0.0f, 32.0f ), size( size )
		{}
	public:
		void SetNextWindow( ImGuiCond_ condition = ImGuiCond_::ImGuiCond_Always ) const
		{
			ImGui::SetNextWindowSize( ToImVec( size ), condition );

			// Left-Top -> Center-Top
			Donya::Vector2 topCenter = pos;
			topCenter.x -= size.x * 0.5f;

			ImGui::SetNextWindowPos( ToImVec( topCenter + offset ), condition );
			
			Donya::Collision::Box2F nextWindowRect;
			nextWindowRect.pos		= topCenter + ( size * 0.5f );
			nextWindowRect.offset	= offset;
			nextWindowRect.size		= size * 0.5f;
			Donya::Vector2 mouse
			{
				scast<float>( Donya::Mouse::Coordinate().x ),
				scast<float>( Donya::Mouse::Coordinate().y ),
			};
			const bool mouseOnNextWindow = Donya::Collision::IsHit( mouse, nextWindowRect );
			ImGui::SetNextWindowBgAlpha( ( mouseOnNextWindow ) ? 0.8f : 0.5f );
		}
		void ShowImGuiNode( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragFloat2( u8"左上座標",			&pos.x		);
			ImGui::DragFloat2( u8"座標オフセット",	&offset.x	);
			ImGui::DragFloat2( u8"全体サイズ",		&size.x		);

			ImGui::TreePop();
		}
	};
	static GuiWindow adjustWindow	{ { 194.0f, 510.0f }, { 364.0f, 300.0f } };
	static GuiWindow debugTeller	{ {  96.0f, -32.0f }, { 128.0f,  16.0f } };
	static GuiWindow playerWindow	{ {   0.0f,   0.0f }, { 360.0f, 180.0f } };
	static GuiWindow doorWindow		{ {   0.0f,   0.0f }, { 360.0f, 180.0f } };
	static GuiWindow enemyWindow	{ {   0.0f,   0.0f }, { 360.0f, 180.0f } };
	static GuiWindow bossWindow		{ {   0.0f,   0.0f }, { 360.0f, 180.0f } };
	static bool enableFloatWindow = true;
	static GuiWindow testTileWindow{ {   0.0f,   0.0f }, { 360.0f, 180.0f } };

	constexpr const char *GetStateName( SceneGame::State kind )
	{
		switch ( kind )
		{
		case SceneGame::State::Stage:			return u8"Stage";
		case SceneGame::State::AppearBoss:		return u8"AppearBoss";
		case SceneGame::State::VSBoss:			return u8"VSBoss";
		case SceneGame::State::Clear:			return u8"Clear";
		case SceneGame::State::WaitToFade:		return u8"WaitToFade";
		default: break;
		}

		return "ERROR";
	}

	constexpr float bgAlpha = 0.6f;
}
void SceneGame::UseImGui( float elapsedTime )
{
	if ( status == State::FirstInitialize )
	{
		ImGui::SetNextWindowBgAlpha( bgAlpha );
		if ( ImGui::BeginIfAllowed() )
		{
			ImGui::Text( u8"よみこみちゅう……" );

			ImGui::Checkbox( u8"ロード画面のまま止める", &dontFinishLoadState );

			bool prgrObj = thObjects.result.Finished();
			bool doneObj = thObjects.result.Succeeded();
			ImGui::Checkbox( u8"終了したか", &prgrObj );
			ImGui::SameLine();
			ImGui::Checkbox( u8"成功したか", &doneObj );

			Performer::LoadPart::ShowImGuiNode( u8"ロード演出の設定" );

			ImGui::End();
		}

		return;
	}
	// else

	if ( nowDebugMode )
	{
		debugTeller.SetNextWindow();
		if ( ImGui::BeginIfAllowed( u8"DEBUG:ON" ) ) { ImGui::End(); }
	}

	UseScreenSpaceImGui();

	static std::array<Donya::Vector3, 3> teleportDestinations
	{
		Donya::Vector3{  26.5f, -54.0f, 0.0f },
		Donya::Vector3{  78.0f, -15.0f, 0.0f },
		Donya::Vector3{ 202.5f, -28.0f, 0.0f },
	};
	auto TeleportPlayerTo = [&]( size_t destIndex )
	{
		if ( teleportDestinations.size() <= destIndex ) { return; }
		// else

		const Donya::Vector3 &destination = teleportDestinations[destIndex];

		const Map emptyMap{}; // Used for empty argument. Fali safe.
		const Map &mapRef = ( pMap ) ? *pMap : emptyMap;

		PlayerInitializer setter{};
		setter.AssignParameter( destination );

		PlayerInit( setter, mapRef );

		currentRoomID = CalcCurrentRoomID();
		const Room *pCurrentRoom = pHouse->FindRoomOrNullptr( currentRoomID );
		if ( pSky && pCurrentRoom )
		{
			pSky->AdvanceHourTo( pCurrentRoom->GetHour(), 0.0f ); // Immediately
		}

		isThereClearEvent	= ( pClearEvent		&& pClearEvent->IsThereIn( currentRoomID ) );
		isThereBoss			= ( pBossContainer	&& pBossContainer->IsThereIn( currentRoomID ) );
		status				= State::Stage;

		AssignCameraPos();
		scroll.active			= false;
		scroll.elapsedSecond	= 0.0f;
		currentScreen = CalcCurrentScreenPlane();
	};

	auto ChangeAvailableWeapon = [&]( bool unlock )
	{
		auto &admin = SaveData::Admin::Get();

		SaveData::File tmp = admin.NowData();
		if ( unlock )
		{
			tmp.availableWeapons.Activate( Definition::WeaponKind::SkullShield );
		}
		else
		{
			tmp.availableWeapons.Reset();
		}
		admin.Write( tmp );

		admin.Save();

		if ( pPlayer )
		{
			pPlayer->ApplyAvailableWeapon( tmp.availableWeapons );
		}
	};

	// Choose a room by mouse
	if ( Donya::Mouse::Trigger( Donya::Mouse::Kind::RIGHT ) && pHouse )
	{
		const Donya::Vector4x4 toWorld = MakeScreenTransform().Inverse();

		Donya::Plane xyPlane;
		xyPlane.distance	= 0.0f;
		xyPlane.normal		= -Donya::Vector3::Front();

		auto Transform		= [&]( const Donya::Vector3 &v, float fourthParam, const Donya::Vector4x4 &m )
		{
			Donya::Vector4 tmp = m.Mul( v, fourthParam );
			tmp /= tmp.w;
			return tmp.XYZ();
		};
		auto CalcWorldPos	= [&]( const Donya::Vector2 &ssPos, const Donya::Vector3 &oldPos )
		{
			const Donya::Vector3 ssRayStart{ ssPos, 0.0f };
			const Donya::Vector3 ssRayEnd  { ssPos, 1.0f };

			const Donya::Vector3 wsRayStart	= Transform( ssRayStart,	1.0f, toWorld );
			const Donya::Vector3 wsRayEnd	= Transform( ssRayEnd,		1.0f, toWorld );

			const auto result = Donya::CalcIntersectionPoint( wsRayStart, wsRayEnd, xyPlane );
			return ( result.isIntersect ) ? result.intersection : oldPos;
		};

		const Donya::Vector2 ssMouse
		{
			scast<float>( Donya::Mouse::Coordinate().x ),
			scast<float>( Donya::Mouse::Coordinate().y ),
		};
		Donya::Vector3 wsMouse = Donya::Vector3{ ssMouse, 0.0f };
		wsMouse = CalcWorldPos( ssMouse, wsMouse );

		const int choiceID = pHouse->CalcBelongRoomID( wsMouse );
		pChosenRoom	= ( choiceID == Room::invalidID )
					? nullptr
					: pHouse->FindRoomOrNullptr( choiceID );
	}
	// "drawLightSources" and TeleportPlayerTo()
	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		if ( Donya::Keyboard::Trigger( 'L' ) )
		{
			drawLightSources = !drawLightSources;
		}
		
		const int placeCount = scast<int>( teleportDestinations.size() );
		for ( int i = 0; i < placeCount; ++i )
		{
			// 1-based, 1 or 2 or ... or size()
			if ( Donya::Keyboard::Trigger( '0' + 1 + i ) )
			{
				// 0-based
				TeleportPlayerTo( scast<size_t>( i ) );
				break;
			}
		}
	}
	if ( Donya::Keyboard::Trigger( VK_F4 ) )
	{
		projectLightCamera = !projectLightCamera;
	}
	if ( Donya::Keyboard::Trigger( VK_F7 ) )
	{
		const bool unlock = !Donya::Keyboard::Press( VK_CONTROL );
		ChangeAvailableWeapon( unlock );
	}

	ImGui::SetNextWindowBgAlpha( bgAlpha );
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else

	ImGui::Checkbox( u8"[ALT+L]	光源の可視化",		&drawLightSources	);
	ImGui::Checkbox( u8"[F4]	光視点にする",		&projectLightCamera	);
	if ( ImGui::Button( u8"[F7]			全武器解放＆セーブ" ) ) { ChangeAvailableWeapon( /* unlock = */ true  ); }
	if ( ImGui::Button( u8"[CTRL+F7]	全武器制限＆セーブ" ) ) { ChangeAvailableWeapon( /* unlock = */ false ); }
	if ( ImGui::Button( u8"[ALT+ 1 ]	自機を 開始地点 へ" ) ) { TeleportPlayerTo( 0 ); }
	if ( ImGui::Button( u8"[ALT+ 2 ]	自機を 中間地点 へ" ) ) { TeleportPlayerTo( 1 ); }
	if ( ImGui::Button( u8"[ALT+ 3 ]	自機を ボス直前 へ" ) ) { TeleportPlayerTo( 2 ); }
	Effect::Admin::Get().SetProjectionMatrix
	(
		( projectLightCamera )
		? lightCamera.GetProjectionMatrix()
		: iCamera.GetProjectionMatrix()
	);

	sceneParam.ShowImGuiNode( u8"ゲームシーンのパラメータ" );
	ImGui::Text( u8"今のゲームステート：%s", GetStateName( status ) );
	PauseProcessor::UpdateParameter();

	if ( ImGui::TreeNode( u8"ステージファイルの読み込み" ) )
	{
		static bool thenSave = true;
		ImGui::Checkbox( u8"読み込み・適用後にセーブする", &thenSave );

		auto PrepareCSVData	= []( const std::string &filePath )
		{
			CSVLoader loader;
			loader.Clear();

			if ( filePath.empty() || !Donya::IsExistFile( filePath ) )
			{
				std::string msg = u8"ファイルオープンに失敗しました。\n";
				msg += u8"ファイル名：[" + filePath + u8"]";
				Donya::ShowMessageBox
				(
					msg,
					"Loading stage is failed",
					MB_ICONEXCLAMATION | MB_OK
				);

				return loader;
			}
			// else

			if ( !loader.Load( filePath ) )
			{
				Donya::ShowMessageBox
				(
					u8"パースに失敗しました。",
					"Loading stage is failed",
					MB_ICONEXCLAMATION | MB_OK
				);
				loader.Clear();
				return loader;
			}
			// else

			return loader;
		};
		auto IsValidData	= []( const CSVLoader &loadedData )
		{
			return ( !loadedData.Get().empty() );
		};

		auto ApplyToBoss	= [&]( const CSVLoader &loadedData )
		{
			if ( !pBossContainer || !pHouse ) { return; }
			// else

			pBossContainer->RemakeByCSV( loadedData, *pHouse );

			if ( thenSave )
			{
				pBossContainer->SaveBosses( stageNumber, /* fromBinary = */ true  );
				pBossContainer->SaveBosses( stageNumber, /* fromBinary = */ false );
			}
		};
		auto ApplyToClear	= [&]( const CSVLoader &loadedData )
		{
			if ( !pClearEvent ) { return; }
			// else

			pClearEvent->RemakeByCSV( loadedData );
			if ( pHouse )
			{
				pClearEvent->ApplyRoomID( *pHouse );
			}

			if ( thenSave )
			{
				pClearEvent->SaveEvents( stageNumber, /* fromBinary = */ true  );
				pClearEvent->SaveEvents( stageNumber, /* fromBinary = */ false );
			}
		};
		auto ApplyToDoor	= [&]( const CSVLoader &loadedData )
		{
			if ( !pDoors ) { return; }
			// else

			pDoors->RemakeByCSV( loadedData );

			// The door is there in a vector, so this pointer may be dangling pointer.
			pThroughingDoor = nullptr;

			if ( thenSave )
			{
				pDoors->SaveBin ( stageNumber );
				pDoors->SaveJson( stageNumber );
			}
		};
		auto ApplyToEnemy	= [&]( const CSVLoader &loadedData )
		{
			const Donya::Vector3 playerPos = GetPlayerPosition();

			Enemy::Admin::Get().ClearInstances();
			Enemy::Admin::Get().RemakeByCSV( loadedData, playerPos, currentScreen );

			if ( thenSave )
			{
				Enemy::Admin::Get().SaveEnemies( stageNumber, /* fromBinary = */ true  );
				Enemy::Admin::Get().SaveEnemies( stageNumber, /* fromBinary = */ false );
			}
		};
		auto ApplyToItem	= [&]( const CSVLoader &loadedData )
		{
			const Map emptyMap{}; // Used for empty argument. Fali safe.
			const Map &mapRef = ( pMap ) ? *pMap : emptyMap;
			Item::Admin::Get().ClearInstances();
			Item::Admin::Get().RemakeByCSV( loadedData, mapRef );

			if ( thenSave )
			{
				Item::Admin::Get().SaveItems( stageNumber, /* fromBinary = */ true );
				Item::Admin::Get().SaveItems( stageNumber, /* fromBinary = */ false );
			}
		};
		auto ApplyToMap		= [&]( const CSVLoader &loadedData )
		{
			if ( !pMap ) { return; }
			// else

			pMap->RemakeByCSV( loadedData );
			if ( thenSave )
			{
				pMap->SaveMap( stageNumber, /* fromBinary = */ true  );
				pMap->SaveMap( stageNumber, /* fromBinary = */ false );
			}
		};
		auto ApplyToPlayer	= [&]( const CSVLoader &loadedData )
		{
			playerIniter.RemakeByCSV( loadedData );

			if ( thenSave )
			{
				playerIniter.SaveBin ( stageNumber );
				playerIniter.SaveJson( stageNumber );
			}

			const Map emptyMap{}; // Used for empty argument. Fali safe.
			const Map &mapRef = ( pMap ) ? *pMap : emptyMap;
			PlayerInit( playerIniter, mapRef );
		};
		auto ApplyToCheckPt	= [&]( const CSVLoader &loadedData )
		{
			checkPoint.RemakeByCSV( loadedData );

			if ( thenSave )
			{
				checkPoint.SaveBin ( stageNumber );
				checkPoint.SaveJson( stageNumber );
			}
		};
		auto ApplyToRoom	= [&]( const CSVLoader &loadedData )
		{
			if ( !pHouse ) { return; }
			// else

			pHouse->RemakeByCSV( loadedData );
			if ( thenSave )
			{
				pHouse->SaveRooms( stageNumber, /* fromBinary = */ true  );
				pHouse->SaveRooms( stageNumber, /* fromBinary = */ false );
			}
		};

		if ( ImGui::TreeNode( u8"バッチロード" ) )
		{
			constexpr size_t bufferSize			= 128U;
			constexpr size_t bufferSizeWithNull	= bufferSize + 1;
			using  BufferType = std::array<char, bufferSizeWithNull>;

			// These default value are my prefer
			static int readStageNumber = Definition::StageNumber::Game();
			static BufferType bufferDirectory	{ "./../../EdittedData/"	};
			static BufferType bufferPrefix		{ "Stage"					};
			static BufferType bufferBoss		{ "Boss"					};
			static BufferType bufferClear		{ "Clear"					};
			static BufferType bufferEnemy		{ "Enemy"					};
			static BufferType bufferItem		{ "Item"					};
			static BufferType bufferMap			{ "Map"						};
			static BufferType bufferRoom		{ "Room"					};
			static BufferType bufferExtension	{ ".csv"					};

			if ( ImGui::Button( u8"読み込み開始" ) )
			{
				Bullet::Admin::Get().ClearInstances();

				const std::string fileDirectory	= bufferDirectory.data();
				const std::string filePrefix	= bufferPrefix.data();
				const std::string fileExtension	= bufferExtension.data();
				const std::string strStageNo	= ( 0 <= readStageNumber && readStageNumber < 10 ) ? "0" + std::to_string( readStageNumber ) : std::to_string( readStageNumber );
				constexpr const char noSuffix = '_';
				std::string	filePath{};
				CSVLoader	loader{};
				auto ProcessOf = [&]( const BufferType &bufferIdentify, const std::function<void( const CSVLoader & )> &ApplyToXXX )
				{
					filePath =	bufferDirectory.data() +
								filePrefix + strStageNo + noSuffix +
								bufferIdentify.data() +
								bufferExtension.data();

					loader.Clear();
					loader = PrepareCSVData( filePath );

					if ( IsValidData( loader ) )
					{
						ApplyToXXX( loader );
					}
				};

				ProcessOf( bufferMap,	ApplyToMap		);
				ProcessOf( bufferMap,	ApplyToDoor		);
				ProcessOf( bufferRoom,	ApplyToRoom		);
				ProcessOf( bufferClear,	ApplyToClear	);
				ProcessOf( bufferMap,	ApplyToPlayer	);
				ProcessOf( bufferMap,	ApplyToCheckPt	);
				ProcessOf( bufferBoss,	ApplyToBoss		);
				ProcessOf( bufferEnemy,	ApplyToEnemy	);
				ProcessOf( bufferItem,	ApplyToItem		);

				if ( pMap ) { pMap->LoadModel( readStageNumber ); }

				currentRoomID = CalcCurrentRoomID();
			}

			ImGui::InputInt ( u8"読み込むステージ番号",	&readStageNumber );
			ImGui::InputText( u8"接頭辞",				bufferPrefix.data(),	bufferSize );
			ImGui::InputText( u8"ディレクトリ",			bufferDirectory.data(),	bufferSize );
			ImGui::InputText( u8"識別子・ボス",			bufferBoss.data(),		bufferSize );
			ImGui::InputText( u8"識別子・クリアイベント",	bufferClear.data(),		bufferSize );
			ImGui::InputText( u8"識別子・敵",			bufferEnemy.data(),		bufferSize );
			ImGui::InputText( u8"識別子・アイテム",		bufferItem.data(),		bufferSize );
			ImGui::InputText( u8"識別子・マップ＆自機",	bufferMap.data(),		bufferSize );
			ImGui::InputText( u8"識別子・ルーム",			bufferRoom.data(),		bufferSize );
			ImGui::InputText( u8"拡張子",				bufferExtension.data(),	bufferSize );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"個別ロード" ) )
		{
			static bool applyBoss	= false;
			static bool applyClear	= false;
			static bool applyDoor	= true;
			static bool applyEnemy	= false;
			static bool applyItem	= false;
			static bool applyMap	= true;
			static bool applyPlayer	= true;
			static bool applyCheckPt= true;
			static bool applyRoom	= false;

			ImGui::Checkbox( u8"ボスに適用",				&applyBoss		);
			ImGui::Checkbox( u8"クリアイベントに適用",	&applyClear		);
			ImGui::Checkbox( u8"敵に適用",				&applyEnemy		);
			ImGui::Checkbox( u8"アイテムに適用",			&applyItem		);
			ImGui::Checkbox( u8"マップに適用",			&applyMap		); ImGui::SameLine();
			ImGui::Checkbox( u8"自機に適用",				&applyPlayer	); ImGui::SameLine();
			ImGui::Checkbox( u8"中間ポイントに適用",		&applyCheckPt	); ImGui::SameLine();
			ImGui::Checkbox( u8"ドアに適用",				&applyDoor		);
			ImGui::Checkbox( u8"ルームに適用",			&applyRoom		);
			if ( ImGui::Button( u8"マップ関連をオフ" ) )
			{ applyMap = applyPlayer = applyCheckPt = applyDoor = false; }
			ImGui::SameLine();
			if ( ImGui::Button( u8"マップ関連をオン" ) )
			{ applyMap = applyPlayer = applyCheckPt = applyDoor = true; }
		
			if ( ImGui::Button( u8"CSVファイルを読み込む" ) )
			{
				const auto filePath	= FetchStageFilePathByCommonDialog();
				const auto loader	= PrepareCSVData( filePath );
				if ( IsValidData( loader ) )
				{
					Bullet::Admin::Get().ClearInstances();

					if ( applyBoss		) { ApplyToBoss		( loader ); }
					if ( applyClear		) { ApplyToClear	( loader ); }
					if ( applyDoor		) { ApplyToDoor		( loader ); }
					if ( applyEnemy		) { ApplyToEnemy	( loader ); }
					if ( applyItem		) { ApplyToItem		( loader ); }
					if ( applyMap		) { ApplyToMap		( loader );	}
					if ( applyPlayer	) { ApplyToPlayer	( loader );	currentRoomID = CalcCurrentRoomID(); }
					if ( applyCheckPt	) { ApplyToCheckPt	( loader );	}
					if ( applyRoom		) { ApplyToRoom		( loader );	currentRoomID = CalcCurrentRoomID(); }
				}
			}

			static int loadMapNumber = 0;
			ImGui::InputInt( u8"マップモデルに適用するステージ番号", &loadMapNumber );
			if ( ImGui::Button( u8"マップモデルを読み込む" ) && pMap )
			{
				pMap->LoadModel( loadMapNumber );
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"各オブジェクトの調整" ) )
	{
		ImGui::InputInt( u8"現在のルーム番号", &currentRoomID );

		if ( ImGui::TreeNode( u8"カメラの現在" ) )
		{
			ImGui::Checkbox( u8"移動方向を反転する・Ｘ", &isReverseCameraMoveX );
			ImGui::Checkbox( u8"移動方向を反転する・Ｙ", &isReverseCameraMoveY );
			ImGui::Checkbox( u8"回転方向を反転する・Ｘ", &isReverseCameraRotX );
			ImGui::Checkbox( u8"回転方向を反転する・Ｙ", &isReverseCameraRotY );

			auto ShowVec3 = []( const std::string &prefix, const Donya::Vector3 &v )
			{
				ImGui::Text( ( prefix + u8"[X:%5.2f][Y:%5.2f][Z:%5.2f]" ).c_str(), v.x, v.y, v.z );
			};

			const Donya::Vector3 cameraPos = iCamera.GetPosition();
			ShowVec3( u8"現在位置", cameraPos );
			ImGui::Text( "" );

			const Donya::Vector3 focusPoint = iCamera.GetFocusPoint();
			ShowVec3( u8"注視点位置", focusPoint );
			ImGui::Text( "" );
			ImGui::TreePop();
		}
		ImGui::Text( "" );

		if ( pSky ) { pSky->ShowImGuiNode( u8"空の現在" ); }
		ImGui::Text( "" );

		if ( pMap    ) { pMap->ShowImGuiNode( u8"マップの現在", stageNumber ); }
		if ( pHouse  ) { pHouse->ShowImGuiNode( u8"部屋の現在", stageNumber ); }
		checkPoint.ShowImGuiNode( u8"中間ポイントの現在", stageNumber );
		if ( pDoors  ) { pDoors->ShowImGuiNode( u8"ドアの現在", stageNumber ); }
		Door::UpdateParameter( u8"ドアのパラメータ" );
		ImGui::Text( "" );

		if ( pPlayer ) { pPlayer->ShowImGuiNode( u8"自機の現在" ); }
		Player::UpdateParameter( u8"自機のパラメータ" );
		ImGui::Text( "" );

		Bullet::Admin::Get().ShowImGuiNode( u8"弾の現在" );
		Bullet::Parameter::Update( u8"弾のパラメータ" );
		ImGui::Text( "" );

		const Donya::Vector3 playerPos = GetPlayerPosition();
		Enemy::Admin::Get().ShowImGuiNode( u8"敵の現在", stageNumber, playerPos, currentScreen );
		Enemy::Parameter::Update( u8"敵のパラメータ" );
		ImGui::Text( "" );

		if ( pBossContainer ) { pBossContainer->ShowImGuiNode( u8"ボスの現在", stageNumber ); }
		Boss::Parameter::Update( u8"ボスのパラメータ" );
		ImGui::Text( "" );

		const Map emptyMap{}; // Used for empty argument. Fali safe.
		const Map &mapRef = ( pMap ) ? *pMap : emptyMap;
		Item::Admin::Get().ShowImGuiNode( u8"アイテムの現在", stageNumber, mapRef );
		Item::Parameter::Update( u8"アイテムのパラメータ" );
		ImGui::Text( "" );

		Meter::Parameter::Update( u8"メータのパラメータ" );
		ImGui::Text( "" );

		Effect::Admin::Get().ShowImGuiNode( u8"エフェクトのパラメータ" );
		ImGui::Text( "" );

		SaveData::Admin::Get().ShowImGuiNode( u8"セーブデータの現在" );
		ImGui::Text( "" );

		if ( ImGui::Button( u8"ロード演出を再終了" ) )
		{
			loadPerformer.Start( FetchParameter().ssLoadingDrawPos, Donya::Color::Code::BLACK );
			loadPerformer.Stop();
		}
		Performer::LoadPart::ShowImGuiNode( u8"ロード演出の設定" );

		ImGui::TreePop();
	}

	RenderingStuffInstance::Get().ShowSurfacesToImGui( u8"サーフェス描画" );

	if ( ImGui::TreeNode( u8"ルーム選択" ) )
	{
		ImGui::TextDisabled( u8"右クリックで，マウス位置にあるルームを選択します。" );

		if ( pChosenRoom )
		{
			pHouse->ShowInstanceNode( u8"選んだルーム", pChosenRoom->GetID() );
			pHouse->ShowIONode( stageNumber );
		}
		else
		{
			ImGui::TextDisabled( u8"選んだルーム" );
		}

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"エフェクト生成テスト" ) )
	{
		static std::shared_ptr<Effect::Handle> pHandle = nullptr;
		static Donya::Vector3	setPos{};
		static Donya::Vector3	scale{ 1.0f, 1.0f, 1.0f };
		static Effect::Kind		kind			= Effect::Kind::CatchItem;
		static float			playSpeed		= 1.0f;
		static int32_t			startFrame		= 0;
		static bool				usePlayerPos	= true;

		ImGui::Checkbox( u8"自機の座標を使う", &usePlayerPos );
		if ( usePlayerPos )
		{
			setPos = ( pPlayer ) ? pPlayer->GetPosition() : setPos;
		}

		ImGui::DragFloat3	( u8"設定位置",		&setPos.x,	0.1f );
		ImGui::DragFloat3	( u8"スケール",		&scale.x,	0.1f );
		ImGui::DragFloat	( u8"再生速度",		&playSpeed,	0.1f );
		ImGui::DragInt		( u8"開始フレーム",	&startFrame );

		int iKind = scast<int>( kind );
		ImGui::SliderInt( u8"種類", &iKind, 0, scast<int>( Effect::Kind::KindCount ) - 1 );
		kind = scast<Effect::Kind>( iKind );

		std::string  strKind = u8"選択："; strKind += Effect::GetEffectName( kind );
		ImGui::Text( strKind.c_str() );

		if ( ImGui::Button( u8"Adminに追加" ) )
		{
			Effect::Admin::Get().GenerateInstance( kind, setPos );
		}
		ImGui::Text( "" );

		bool exists = ( pHandle && pHandle->IsExists() ) ? true : false;
		ImGui::Checkbox( u8"インスタンスは存在しているか", &exists );


		if ( ImGui::Button( u8"再設定" ) )
		{
			if ( pHandle ) { pHandle->Stop(); }
			pHandle.reset();

			pHandle = std::make_shared<Effect::Handle>( Effect::Handle::Generate( kind, setPos, startFrame ) );
			if ( !pHandle->IsExists() ) { pHandle.reset(); }
			pHandle->SetPlaySpeed( playSpeed );
		}
		if ( ImGui::Button( u8"削除" ) && pHandle )
		{
			pHandle->Stop();
			pHandle.reset();
		}

		if ( pHandle )
		{
			pHandle->SetPosition( setPos	);
			pHandle->SetScale	( scale		);
		}

		ImGui::TreePop();
	}

	ImGui::End();
}
void SceneGame::UseScreenSpaceImGui()
{
	if ( Donya::Keyboard::Press( VK_MENU ) && Donya::Keyboard::Trigger( 'F' ) )
	{
		enableFloatWindow = !enableFloatWindow;
	}

	const Donya::Vector4x4 toScreen = MakeScreenTransform();
	auto WorldToScreen = [&toScreen]( const Donya::Vector3 &world, bool isPosition = 1.0f )
	{
		Donya::Vector4 tmp = toScreen.Mul( world, ( isPosition ) ? 1.0f : 0.0f );
		tmp /= tmp.w;
		return tmp.XYZ();
	};

	adjustWindow.SetNextWindow();
	if ( ImGui::BeginIfAllowed( u8"フロートウィンドウの調整" ) )
	{
		ImGui::Text( u8"「ALT+F」でも切り替え可能" );
		ImGui::Checkbox( u8"有効にするか", &enableFloatWindow );
		adjustWindow.ShowImGuiNode( u8"このウィンドウ"	);
		playerWindow.ShowImGuiNode( u8"自機ウィンドウ"	);
		enemyWindow .ShowImGuiNode( u8"敵ウィンドウ"		);
		bossWindow  .ShowImGuiNode( u8"ボスウィンドウ"	);

		ImGui::End();
	}

	if ( !enableFloatWindow ) { return; }
	// else

	if ( pPlayer )
	{
		const Donya::Vector3 ssPos = WorldToScreen( pPlayer->GetPosition() );
		playerWindow.pos = ssPos.XY();
		playerWindow.SetNextWindow();

		if ( ImGui::BeginIfAllowed( u8"自機" ) )
		{
			pPlayer->ShowImGuiNode( u8"現在" );
			Player::UpdateParameter( u8"パラメータ" );

			ImGui::End();
		}
	}

	if ( pDoors )
	{
		auto Show = [&]( const Donya::Vector2 &ssPos, size_t doorIndex, auto ShowInstanceNodeMethod )
		{
			doorWindow.pos = ssPos;
			doorWindow.SetNextWindow();

			const std::string caption = u8"ドア" + Donya::MakeArraySuffix( doorIndex );

			if ( ImGui::BeginIfAllowed( caption.c_str() ) )
			{
				ShowInstanceNodeMethod();
				Door::UpdateParameter( u8"ドアのパラメータ" );

				ImGui::End();
			}
		};

		const size_t doorCount	= pDoors->GetDoorCount();

		Donya::Vector2 ssPos;
		Door::Instance *ptr = nullptr;
		for ( size_t i = 0; i < doorCount; ++i )
		{
			ptr = pDoors->GetInstanceOrNullptr( i );
			if ( !ptr ) { continue; }
			// else

			ssPos = WorldToScreen( ptr->GetBody().pos ).XY();
			Show
			(
				ssPos, i,
				[&]()
				{
					ptr->ShowImGuiNode( u8"ドア" + Donya::MakeArraySuffix( i ) );
				}
			);
		}
	}

	// Enemies
	{
		auto &enemyAdmin = Enemy::Admin::Get();
		const auto playerPos = GetPlayerPosition();

		auto Show = [&]( const Donya::Vector2 &ssPos, size_t enemyIndex, auto ShowInstanceNodeMethod )
		{
			enemyWindow.pos = ssPos;
			enemyWindow.SetNextWindow();

			const std::string caption = u8"敵" + Donya::MakeArraySuffix( enemyIndex );

			if ( ImGui::BeginIfAllowed( caption.c_str() ) )
			{
				ShowInstanceNodeMethod();
				enemyAdmin.ShowIONode( stageNumber, playerPos, currentScreen );
				Enemy::Parameter::Update( u8"敵のパラメータ" );

				ImGui::End();
			}
		};
		
		const size_t enemyCount	= enemyAdmin.GetInstanceCount();

		Donya::Vector2 ssPos;
		std::shared_ptr<const Enemy::Base> pEnemy = nullptr;
		for ( size_t i = 0; i < enemyCount; ++i )
		{
			pEnemy = enemyAdmin.GetInstanceOrNullptr( i );
			if ( !pEnemy ) { continue; }
			// else

			ssPos = WorldToScreen( pEnemy->GetPosition() ).XY();
			Show
			(
				ssPos, i,
				[&]()
				{
					enemyAdmin.ShowInstanceNode( i );
				}
			);
		}
	}

	if ( pBossContainer )
	{
		auto Show = [&]( const Donya::Vector2 &ssPos, size_t bossIndex, auto ShowInstanceNodeMethod )
		{
			bossWindow.pos = ssPos;
			bossWindow.SetNextWindow();

			const std::string caption = u8"ボス" + Donya::MakeArraySuffix( bossIndex );
			if ( ImGui::BeginIfAllowed( caption.c_str() ) )
			{
				ShowInstanceNodeMethod();
				Boss::Parameter::Update( u8"ボスのパラメータ" );

				ImGui::End();
			}
		};

		Donya::Vector2 ssPos;
		Boss::Container::BossSet bossSet{};
		const size_t bossCount = pBossContainer->GetBossCount();
		for ( size_t i = 0; i  < bossCount; ++i )
		{
			bossSet = pBossContainer->GetBossSet( i );
			
			const Donya::Vector3 wsPos = ( bossSet.pBoss ) ? bossSet.pBoss->GetPosition() : bossSet.initializer.wsPos;
			ssPos = WorldToScreen( wsPos ).XY();
			Show
			(
				ssPos, i,
				[&]()
				{
					pBossContainer->ShowInstanceNode( i );
				}
			);
		}
	}

	// Test tile
	if ( 0 && pMap )
	{
		const Donya::Vector4x4 toWorld = MakeScreenTransform().Inverse();

		Donya::Plane xyPlane;
		xyPlane.distance	= 0.0f;
		xyPlane.normal		= -Donya::Vector3::Front();

		auto Transform		= [&]( const Donya::Vector3 &v, float fourthParam, const Donya::Vector4x4 &m )
		{
			Donya::Vector4 tmp = m.Mul( v, fourthParam );
			tmp /= tmp.w;
			return tmp.XYZ();
		};
		auto CalcWorldPos	= [&]( const Donya::Vector2 &ssPos, const Donya::Vector3 &oldPos )
		{
			const Donya::Vector3 ssRayStart{ ssPos, 0.0f };
			const Donya::Vector3 ssRayEnd  { ssPos, 1.0f };

			const Donya::Vector3 wsRayStart	= Transform( ssRayStart,	1.0f, toWorld );
			const Donya::Vector3 wsRayEnd	= Transform( ssRayEnd,		1.0f, toWorld );

			const auto result = Donya::CalcIntersectionPoint( wsRayStart, wsRayEnd, xyPlane );
			return ( result.isIntersect ) ? result.intersection : oldPos;
		};

		const Donya::Vector2 ssMouse
		{
			scast<float>( Donya::Mouse::Coordinate().x ),
			scast<float>( Donya::Mouse::Coordinate().y ),
		};
		Donya::Vector3 wsMouse = Donya::Vector3{ ssMouse, 0.0f };
		wsMouse = CalcWorldPos( ssMouse, wsMouse );

		testTileWindow.pos = ssMouse;
		testTileWindow.SetNextWindow();
		if ( ImGui::BeginIfAllowed( u8"タイル" ) )
		{
			ImGui::Text( u8"World: [X:%5.2f][Y:%5.2f][Z:%5.2f]", wsMouse.x, wsMouse.y, wsMouse.z );
			
			const auto tileIndex = Map::ToTilePos( wsMouse );
			ImGui::Text( u8"Tile Index: [X:%3d][Y:%3d]", tileIndex.x, tileIndex.y );
			
			auto pTile = pMap->GetPlaceTileOrNullptr( wsMouse );
			if ( pTile )
			{
				const Donya::Vector3 ssPos = WorldToScreen( pTile->GetPosition() );
				// testTileWindow.pos = ssPos.XY();

				const Donya::Vector3 p = pTile->GetPosition();
				ImGui::Text( u8"wsPos: [X:%5.2f][Y:%5.2f][Z:%5.2f]", p.x, p.y, p.z );

				const auto wholeArray = pMap->GetTiles();
				const size_t rowCount = wholeArray.size();
				for ( size_t r = 0; r < rowCount; ++r )
				{
					bool shouldBreak = false;
					const size_t colCount = wholeArray[r].size();
					for ( size_t c = 0; c < colCount; ++c )
					{
						if ( wholeArray[r][c] == pTile )
						{
							ImGui::Text( u8"Actual Index:" );
							ImGui::Text( u8"[Row:%3d][Col:%3d]", r, c );

							shouldBreak = true;
							break;
						}
					}
					if ( shouldBreak ) { break; }
				}
			}
			else
			{
				ImGui::TextDisabled( u8"Now is empty tile chosen." );
			}

			ImGui::End();
		}
	}
}
#endif // USE_IMGUI
