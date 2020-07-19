#include "SceneGame.h"

#include <algorithm>				// Use std::find
#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Color.h"			// Use ClearBackGround(), StartFade().
#include "Donya/Keyboard.h"			// Make an input of player.
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Template.h"			// Use Clamp
#include "Donya/Useful.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Bullet.h"
#include "Common.h"
#include "Enemy.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"
#include "Parameter.h"
#include "PlayerParam.h"

#if DEBUG_MODE
#include "CSVLoader.h"
#pragma comment( lib, "comdlg32.lib" ) // Used for common-dialog
#endif // DEBUG_MODE

namespace
{
#if DEBUG_MODE
	constexpr bool IOFromBinary = false;
#else
	constexpr bool IOFromBinary = true;
#endif // DEBUG_MODE
}

namespace
{
	struct SceneParam
	{
		struct
		{
			float slerpFactor	= 0.2f;
			float fovDegree		= 30.0f;
			Donya::Vector3 offsetPos{ 0.0f, 5.0f, -10.0f };	// The offset of position from the player position.
			Donya::Vector3 offsetFocus;	// The offset of focus from the player position.
		}
		camera;
		
		Donya::Model::Constants::PerScene::DirectionalLight directionalLight;

		float waitSecondRetry = 1.0f; // Waiting second between Miss ~ Re-try
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
				CEREAL_NVP( camera.offsetFocus			),
				CEREAL_NVP( directionalLight.color		),
				CEREAL_NVP( directionalLight.direction	)
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( waitSecondRetry ) );
			}
			if ( 2 <= version )
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
			
			if ( ImGui::TreeNode( u8"平行光" ) )
			{
				ImGui::ColorEdit4  ( u8"色",		&directionalLight.color.x );
				ImGui::SliderFloat4( u8"方向",	&directionalLight.direction.x, -1.0f, 1.0f );
				
				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"秒数関連" ) )
			{
				ImGui::DragFloat( u8"ミスからリトライまでの待機秒数", &waitSecondRetry, 0.01f );

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
CEREAL_CLASS_VERSION( SceneParam, 0 )

void SceneGame::Init()
{
	// Donya::Sound::Play( Music::BGM_Game );
#if DEBUG_MODE
	// Donya::Sound::AppendFadePoint( Music::BGM_Game, 2.0f, 0.0f, true ); // Too noisy.
#endif // DEBUG_MODE

	sceneParam.LoadParameter();
	
	bool result{};

	pRenderer = std::make_unique<RenderingHelper>();
	result = pRenderer->Init();
	assert( result );

#if DEBUG_MODE
	stageNumber = 0; // temporary debug stage
#endif // DEBUG_MODE

	pPlayerIniter = std::make_unique<PlayerInitializer>();
	pPlayerIniter->LoadParameter( stageNumber );

	InitStage( stageNumber );
}
void SceneGame::Uninit()
{
	UninitStage();

	// Donya::Sound::Stop( Music::BGM_Game );
}

Scene::Result SceneGame::Update( float elapsedTime )
{
#if DEBUG_MODE
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
			CameraInit();
		}
	}
#endif // DEBUG_MODE

#if USE_IMGUI
	UseImGui();
#endif // USE_IMGUI

	if ( Fader::Get().IsClosed() && nextScene == Scene::Type::Game )
	{
		UninitStage();
		InitStage( stageNumber );
	}

	controller.Update();
	AssignCurrentInput();

	if ( pMap ) { pMap->Update( elapsedTime ); }
	const Map emptyMap{}; // Used for empty argument. Fali safe.
	const Map &mapRef = ( pMap ) ? *pMap : emptyMap;

	const int oldRoomID = currentRoomID;
	UpdateCurrentRoomID();
	if ( oldRoomID != currentRoomID )
	{
		isThereClearEvent	= ( pClearEvent		&& pClearEvent->IsThereIn( currentRoomID ) );
		isThereBoss			= ( pBossContainer	&& pBossContainer->IsThereIn( currentRoomID ) );
	}
	if ( isThereBoss && pBossContainer )
	{
		if ( status == State::StrategyStage )
		{
			status = State::BattleBoss;
			pBossContainer->StartupBossIfStandby( currentRoomID );
		}
		else if ( !pBossContainer->IsAliveIn( currentRoomID ) )
		{
			// TODO: Disallow the control of player here.
			// then wait, then goto next state.
			isThereBoss = false;
			status = ( isThereClearEvent ) ? State::ClearStage : State::StrategyStage;
		}
	}
	if ( isThereClearEvent && !isThereBoss && !Fader::Get().IsExist() )
	{
		// TODO: Begin a clear performance here,
		// then call it.
		StartFade( Scene::Type::Result );
	}

	const Room *pCurrentRoom = pHouse->FindRoomOrNullptr( currentRoomID );
	
	PlayerUpdate( elapsedTime, mapRef );
	if ( FetchParameter().waitSecondRetry <= elapsedSecondsAfterMiss && !Fader::Get().IsExist() )
	{
		const int remaining = Player::Remaining::Get();
		if ( remaining <= 0 )
		{
			// Donya::Sound::AppendFadePoint( Music::BGM_Game, 2.0f, 0.0f, true );

			// TODO: Go to a game-over scene
			StartFade( Scene::Type::Over );
			// TODO: Impl it
			// The remaining count will be re-set at transitioned scene.
		}
		else
		{
			// Re-try the game
			StartFade( Scene::Type::Game );

			Player::Remaining::Decrement();
		}
	}

	const Donya::Vector3 playerPos = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3::Zero();
	Bullet::Admin::Get().Update( elapsedTime, currentScreen );
	Enemy::Admin::Get().Update( elapsedTime, playerPos, currentScreen );

	BossUpdate( elapsedTime, playerPos );

	// PhysicUpdates
	{
		using Dir = Definition::Direction;

		const auto currentRoomArea	= ( pCurrentRoom )
									? pHouse->CalcRoomArea( currentRoomID )
									: currentScreen; // Fail safe
		const auto transitionable	= ( pCurrentRoom )
									? pCurrentRoom->GetTransitionableDirection()
									: Dir::Nil;

		if ( pPlayer )
		{
			const float leftBorder	= Contain( transitionable, Dir::Left )
									? -FLT_MAX
									: currentRoomArea.Min().x;
			const float rightBorder	= Contain( transitionable, Dir::Right )
									? FLT_MAX
									: currentRoomArea.Max().x;

			pPlayer->PhysicUpdate( elapsedTime, mapRef, leftBorder, rightBorder );
		}

		Bullet::Admin::Get().PhysicUpdate( elapsedTime );
		Enemy::Admin::Get().PhysicUpdate( elapsedTime, mapRef );

		if ( pBossContainer ) { pBossContainer->PhysicUpdate( elapsedTime, mapRef ); }
	}

	// CameraUpdate() depends the currentScreen, so I should update that before CameraUpdate().
	currentScreen = CalcCurrentScreenPlane();
	CameraUpdate();

	// Kill the player if fall out from current room
	if ( pPlayer && !pPlayer->NowMiss() )
	{
		const float playerTop  = pPlayer->GetHitBox().Max().y;
		const float roomBottom = ( pCurrentRoom ) ? pCurrentRoom->GetArea().Min().y : currentScreen.Min().y;
		if ( playerTop < roomBottom )
		{
			pPlayer->KillMe();
		}
	}

	Collision_BulletVSBoss();
	Collision_BulletVSEnemy();
	Collision_BulletVSPlayer();
	Collision_BossVSPlayer();
	Collision_EnemyVSPlayer();

	return ReturnResult();
}

void SceneGame::Draw( float elapsedTime )
{
	// elapsedTime = 1.0f; // Disable

	ClearBackGround();

	const Donya::Vector4x4 VP{ iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix() };
	const auto &data = FetchParameter();
	
	// Update scene constant.
	{
		Donya::Model::Constants::PerScene::Common constant{};
		constant.directionalLight	= data.directionalLight;
		constant.eyePosition		= Donya::Vector4{ iCamera.GetPosition(), 1.0f };
		constant.viewProjMatrix		= VP;
		pRenderer->UpdateConstant( constant );
	}

	pRenderer->ActivateDepthStencilModel();
	pRenderer->ActivateRasterizerModel();
	pRenderer->ActivateSamplerModel();
	pRenderer->ActivateConstantScene();
	{
		// The drawing priority is determined by the priority of the information.

		pRenderer->ActivateShaderNormalSkinning();
		Bullet::Admin::Get().Draw( pRenderer.get() );
		if ( pPlayer		) { pPlayer->Draw( pRenderer.get() ); }
		if ( pBossContainer	) { pBossContainer->Draw( pRenderer.get() ); }
		pRenderer->DeactivateShaderNormalSkinning();

		pRenderer->ActivateShaderNormalStatic();
		Enemy::Admin::Get().Draw( pRenderer.get() );
		if ( pMap ) { pMap->Draw( pRenderer.get() ); }
		pRenderer->DeactivateShaderNormalStatic();
	}
	pRenderer->DeactivateConstantScene();
	pRenderer->DeactivateDepthStencilModel();
	pRenderer->DeactivateRasterizerModel();
	pRenderer->DeactivateSamplerModel();

#if DEBUG_MODE
	// Object's hit/hurt boxes
	if ( Common::IsShowCollision() )
	{
		if ( pPlayer		) { pPlayer->DrawHitBox( pRenderer.get(), VP );			}
		if ( pBossContainer	) { pBossContainer->DrawHitBoxes( pRenderer.get(), VP );}
		if ( pClearEvent	) { pClearEvent->DrawHitBoxes( pRenderer.get(), VP );	}
		if ( pMap			) { pMap->DrawHitBoxes( pRenderer.get(), VP );			}
		Bullet::Admin::Get().DrawHitBoxes( pRenderer.get(), VP );
		Enemy ::Admin::Get().DrawHitBoxes( pRenderer.get(), VP );
		if ( pHouse			) { pHouse->DrawHitBoxes( pRenderer.get(), VP );		}
	}
#endif // DEBUG_MODE

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= VP;
		constant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 0.6f };
		constant.lightDirection	= data.directionalLight.direction.XYZ();
		
		auto DrawCube = [&]( const Donya::Vector3 &pos, const Donya::Vector3 &scale = { 1.0f, 1.0f, 1.0f } )
		{
			constant.matWorld._11 = scale.x * 2.0f;
			constant.matWorld._22 = scale.y * 2.0f;
			constant.matWorld._33 = scale.z * 2.0f;
			constant.matWorld._41 = pos.x;
			constant.matWorld._42 = pos.y;
			constant.matWorld._43 = pos.z;
			pRenderer->ProcessDrawingCube( constant );
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
	}
#endif // DEBUG_MODE
}

Donya::Vector4x4 SceneGame::MakeScreenTransform() const
{
	const Donya::Vector4x4 matViewProj = iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix();
	const Donya::Vector4x4 matViewport = Donya::Vector4x4::MakeViewport( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
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

	constexpr Donya::Vector2 halfScreenSize
	{
		Common::HalfScreenWidthF(),
		Common::HalfScreenHeightF(),
	};
	constexpr Donya::Vector2 left	{ 0.0f,						halfScreenSize.y		};
	constexpr Donya::Vector2 top	{ halfScreenSize.x,			0.0f					};
	constexpr Donya::Vector2 right	{ halfScreenSize.x * 2.0f,	halfScreenSize.y		};
	constexpr Donya::Vector2 bottom	{ halfScreenSize.x,			halfScreenSize.y * 2.0f	};

	Donya::Vector3 nowLeft		{ currentScreen.Min().x,	currentScreen.pos.y,	0.0f };
	Donya::Vector3 nowTop		{ currentScreen.pos.x,		currentScreen.Min().y,	0.0f };
	Donya::Vector3 nowRight		{ currentScreen.Max().x,	currentScreen.pos.y,	0.0f };
	Donya::Vector3 nowBottom	{ currentScreen.pos.x,		currentScreen.Max().y,	0.0f };

	nowLeft		= CalcWorldPos( left,	nowLeft		);
	nowTop		= CalcWorldPos( top,	nowTop		);
	nowRight	= CalcWorldPos( right,	nowRight	);
	nowBottom	= CalcWorldPos( bottom, nowBottom	);

	const float halfWidth	= fabsf( nowRight.x - nowLeft.x ) * 0.5f;
	const float halfHeight	= fabsf( nowBottom.y - nowTop.y ) * 0.5f;

	Donya::Collision::Box3F nowScreen;
	nowScreen.pos.x  = nowLeft.x + halfWidth;	// Specify center
	nowScreen.pos.y  = nowTop.y  - halfHeight;	// Specify center
	nowScreen.pos.z  = 0.0f;
	nowScreen.size.x = halfWidth;
	nowScreen.size.y = halfHeight;
	nowScreen.size.z = FLT_MAX;
	return nowScreen;
}

void SceneGame::InitStage( int stageNo )
{
	status = State::StrategyStage;

	bool result = true;

	pMap = std::make_unique<Map>();
	pMap->Init( stageNo );

	pHouse = std::make_unique<House>();
	pHouse->Init( stageNo );

	pClearEvent = std::make_unique<ClearEvent>();
	pClearEvent->Init( stageNo );

	pBossContainer = std::make_unique<Boss::Container>();
	pBossContainer->Init( stageNo );
#if DEBUG_MODE
	pBossContainer->SaveBosses( stageNo, true );
#endif // DEBUG_MODE

	PlayerInit();

	currentRoomID = pHouse->CalcBelongRoomID( pPlayer->GetPosition() );

	CameraInit();
	// The calculation of screen space uses camera's view and projection matrix, so I must calc after CameraInit().
	currentScreen = CalcCurrentScreenPlane();

	Bullet::Admin::Get().ClearInstances();

	auto &enemyAdmin = Enemy::Admin::Get();
	enemyAdmin.ClearInstances();
	enemyAdmin.LoadEnemies( stageNo, IOFromBinary );
#if DEBUG_MODE
	enemyAdmin.SaveEnemies( stageNo, true );
#endif // DEBUG_MODE
}
void SceneGame::UninitStage()
{
	if ( pMap			) { pMap->Uninit();				}
	if ( pHouse			) { pHouse->Uninit();			}
	if ( pBossContainer	) { pBossContainer->Uninit();	}
	if ( pPlayer		) { pPlayer->Uninit();			}
	pMap.reset();
	pClearEvent.reset();
	pBossContainer.reset();
	pHouse.reset();
	pPlayer.reset();

	Bullet::Admin::Get().ClearInstances();
	Enemy::Admin::Get().ClearInstances();
}

void SceneGame::AssignCurrentInput()
{
	bool pressLeft	= false;
	bool pressRight	= false;
	bool pressUp	= false;
	bool pressDown	= false;
	bool pressJump	= false;
	bool pressShot	= false;

	// TODO: To be changeable the input key or button

	if ( controller.IsConnected() )
	{
		using Button	= Donya::Gamepad::Button;
		using Direction	= Donya::Gamepad::StickDirection;
		
		pressLeft	= controller.Press( Button::LEFT	) || controller.PressStick( Direction::LEFT		);
		pressRight	= controller.Press( Button::RIGHT	) || controller.PressStick( Direction::RIGHT	);
		pressUp		= controller.Press( Button::UP		) || controller.PressStick( Direction::UP		);
		pressDown	= controller.Press( Button::DOWN	) || controller.PressStick( Direction::DOWN		);
		pressJump	= controller.Press( Button::A );
		pressShot	= controller.Press( Button::B );
	}
	else
	{
		pressLeft	= Donya::Keyboard::Press( VK_LEFT	);
		pressRight	= Donya::Keyboard::Press( VK_RIGHT	);
		pressUp		= Donya::Keyboard::Press( VK_UP		);
		pressDown	= Donya::Keyboard::Press( VK_DOWN	);
		pressJump	= Donya::Keyboard::Press( VK_SHIFT	);
		pressShot	= Donya::Keyboard::Press( 'Z' );
	}

	currentInput.Clear();
	if ( pressLeft	) { currentInput.inputDirection.x -= 1.0f; }
	if ( pressRight	) { currentInput.inputDirection.x += 1.0f; }
	if ( pressUp	) { currentInput.inputDirection.y += 1.0f; } // World space direction
	if ( pressDown	) { currentInput.inputDirection.y -= 1.0f; } // World space direction
	currentInput.pressJump = pressJump;
	currentInput.pressShot = pressShot;
}

void SceneGame::CameraInit()
{
	const auto &data = FetchParameter();

	iCamera.Init( Donya::ICamera::Mode::Look );
	iCamera.SetZRange( 0.1f, 1000.0f );
	iCamera.SetFOV( ToRadian( data.camera.fovDegree ) );
	iCamera.SetScreenSize( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
	AssignCameraPos();
	iCamera.SetProjectionPerspective();

	// I can setting a configuration,
	// but current data is not changed immediately.
	// So update here.
	Donya::ICamera::Controller moveInitPoint{};
	moveInitPoint.SetNoOperation();
	moveInitPoint.slerpPercent = 1.0f;
	iCamera.Update( moveInitPoint );
}
void SceneGame::AssignCameraPos()
{
	const auto &data = FetchParameter();
	Donya::Vector3 focusPos = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3::Zero();
	
	if ( pHouse )
	{
		const auto area = pHouse->CalcRoomArea( currentRoomID );
		if ( area != Donya::Collision::Box3F::Nil() )
		{
			const auto min = area.Min();
			const auto max = area.Max();
			
			const float halfAreaWidth		= ( max.x - min.x ) * 0.5f;
			const float halfAreaHeight		= ( max.y - min.y ) * 0.5f;
			const float halfScreenWidth		= ( currentScreen.Max().x - currentScreen.Min().x ) * 0.5f;
			const float halfScreenHeight	= ( currentScreen.Max().y - currentScreen.Min().y ) * 0.5f;
			const float halfWidth			= std::min( halfAreaWidth,  halfScreenWidth  ); // If area size smaller than screen, set to center the focus point
			const float halfHeight			= std::min( halfAreaHeight, halfScreenHeight ); // If area size smaller than screen, set to center the focus point

			focusPos += data.camera.offsetFocus; // Clamp the center pos in offseted value
			focusPos.x = Donya::Clamp( focusPos.x, min.x + halfWidth,  max.x - halfWidth  );
			focusPos.y = Donya::Clamp( focusPos.y, min.y + halfHeight, max.y - halfHeight );
			focusPos.z = Donya::Clamp( focusPos.z, min.z, max.z );
			focusPos -= data.camera.offsetFocus; // Back to before offset pos because below setting process expects that value
		}
	}

	iCamera.SetPosition  ( focusPos + data.camera.offsetPos   );
	iCamera.SetFocusPoint( focusPos + data.camera.offsetFocus );
}
void SceneGame::CameraUpdate()
{
	const auto &data = FetchParameter();

#if USE_IMGUI
	iCamera.SetFOV( ToRadian( data.camera.fovDegree ) );
	iCamera.SetProjectionPerspective();
#endif // USE_IMGUI

	currentScreen = CalcCurrentScreenPlane();

	Donya::ICamera::Controller input{};
	input.SetNoOperation();
	input.slerpPercent = data.camera.slerpFactor;

#if !DEBUG_MODE
	AssignCameraPos();
	iCamera.Update( input );
#else
	if ( !nowDebugMode )
	{
		AssignCameraPos();
		iCamera.Update( input );
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
		else
		if ( Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) )
		{
			constexpr float MOVE_SPEED = 0.05f;
			movement.x = diff.x * MOVE_SPEED;
			movement.y = diff.y * MOVE_SPEED;

			if ( isReverseCameraMoveX ) { movement.x *= -1.0f; }
			if ( isReverseCameraMoveY ) { movement.y *= -1.0f; }
		}

		constexpr float FRONT_SPEED = 2.0f;
		movement.z = FRONT_SPEED * scast<float>( Donya::Mouse::WheelRot() );
	}

	input.moveVelocity		= movement;
	input.yaw				= rotation.x;
	input.pitch				= rotation.y;
	input.roll				= 0.0f;
	input.moveInLocalSpace	= true;

	iCamera.Update( input );

#endif // !DEBUG_MODE
}

void SceneGame::PlayerInit()
{
	if ( !pPlayerIniter )
	{
		_ASSERT_EXPR( 0, L"Error: Initializer is not available!" );
		return;
	}
	// else

	if ( pPlayer )
	{
		pPlayer->Uninit();
		pPlayer.reset();
	}

	pPlayer = std::make_unique<Player>();
	pPlayer->Init( *pPlayerIniter );
}
void SceneGame::PlayerUpdate( float elapsedTime, const Map &terrain )
{
	if ( !pPlayer ) { return; }
	// else

	Player::Input input{};
	input.moveVelocity	= currentInput.inputDirection;
	input.useJump		= currentInput.pressJump;
	input.useShot		= currentInput.pressShot;

	pPlayer->Update( elapsedTime, input, terrain );

	if ( pPlayer->NowMiss() )
	{
		elapsedSecondsAfterMiss += elapsedTime;
	}
	else
	{
		elapsedSecondsAfterMiss = 0.0f;
	}
}

void SceneGame::BossUpdate( float elapsedTime, const Donya::Vector3 &wsTargetPos )
{
	if ( !pBossContainer ) { return; }
	// else

	Boss::Input input{};
	input.wsTargetPos				= wsTargetPos;
	input.controllerInputDirection	= currentInput.inputDirection;
	input.pressJump					= currentInput.pressJump;
	input.pressShot					= currentInput.pressShot;

	pBossContainer->Update( elapsedTime, input );
}

void SceneGame::UpdateCurrentRoomID()
{
	if ( !pPlayer || !pHouse ) { return; }
	// else

	const auto	playerPos	= pPlayer->GetPosition();
	const int	nextID		= pHouse->CalcBelongRoomID( playerPos );
	if ( nextID != Room::invalidID )
	{
		currentRoomID = nextID;
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
			// TODO: Play protection SE
		}
		else
		{
			pBoss->GiveDamage( pBullet->GetDamage() );
			pBullet->CollidedToObject( pBoss->WillDie() );
			// TODO: Play hit SE
		}
	};

	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pBullet = bulletAdmin.GetInstanceOrNullptr( i );
		if ( !pBullet ) { continue; }
		// else

		// The bullet's hit box is only either AABB or Sphere is valid
		// (Invalid one's exist flag will false, so IsHit() will returns false)

		if ( Donya::Collision::IsHit( bossBody, pBullet->GetHitBox() ) )
		{
			HitProcess();
			continue;
		}
		// else

		if ( Donya::Collision::IsHit( bossBody, pBullet->GetHitSphere() ) )
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
	auto IsHitImpl						= [&]( const auto &a, const auto &b )
	{
		return Donya::Collision::IsHit( a, b );
	};
	auto FindCollidingEnemyOrNullptr	= [&]( const auto &otherHitBox )
	{
		std::shared_ptr<const Enemy::Base> pEnemy = nullptr;
		for ( size_t i = 0; i < enemyCount; ++i )
		{
			if ( IsAlreadyCollided( i ) ) { continue; }
			// else

			pEnemy = enemyAdmin.GetInstanceOrNullptr( i );
			if ( !pEnemy ) { continue; }
			// else

			if ( IsHitImpl( pEnemy->GetHurtBox(), otherHitBox ) )
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

		pOther = FindCollidingEnemyOrNullptr( bulletBody );
		while ( pOther )
		{
			result.collided = true;

			pOther->GiveDamage( pBullet->GetDamage() );
			if ( !pOther->WillDie() )
			{
				result.pierced = false;
			}

			pOther = FindCollidingEnemyOrNullptr( bulletBody );
		}

		return result;
	};

	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pBullet = bulletAdmin.GetInstanceOrNullptr( i );
		if ( !pBullet ) { continue; }
		// else

		otherAABB	= pBullet->GetHitBox();
		otherSphere	= pBullet->GetHitSphere();
		if ( !otherAABB.exist && !otherSphere.exist ) { continue; }
		// else

		const auto result	= ( otherAABB.exist )
							? Process( otherAABB   )
							: Process( otherSphere );
		if ( result.collided )
		{
			pBullet->CollidedToObject( result.pierced );
		}

		collidedEnemyIndices.clear();
	}
}
void SceneGame::Collision_BulletVSPlayer()
{
	if ( !pPlayer || pPlayer->NowMiss() ) { return; }
	// else

	const auto playerBody = pPlayer->GetHurtBox();

	auto &bulletAdmin = Bullet::Admin::Get();
	const size_t bulletCount = bulletAdmin.GetInstanceCount();

	// The bullet's hit box is only either AABB or Sphere is valid
	// (Invalid one's exist flag will false, so IsHit() will returns false)
	Donya::Collision::Box3F		bulletAABB;
	Donya::Collision::Sphere3F	bulletSphere;

	std::shared_ptr<const Bullet::Base> pBullet = nullptr;
	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pBullet = bulletAdmin.GetInstanceOrNullptr( i );
		if ( !pBullet ) { continue; }
		// else

		bulletAABB = pBullet->GetHitBox();
		if ( Donya::Collision::IsHit( playerBody, bulletAABB ) )
		{
			pPlayer->GiveDamage( pBullet->GetDamage(), bulletAABB );
			pBullet->CollidedToObject( pPlayer->WillDie() );
			continue;
		}
		// else

		bulletSphere = pBullet->GetHitSphere();
		if ( Donya::Collision::IsHit( playerBody, bulletSphere ) )
		{
			pPlayer->GiveDamage( pBullet->GetDamage(), bulletSphere );
			pBullet->CollidedToObject( pPlayer->WillDie() );
		}
	}
}
void SceneGame::Collision_BossVSPlayer()
{
	if ( !pPlayer || pPlayer->NowMiss() )  { return; }
	if ( !pBossContainer || !isThereBoss ) { return; }
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
	if ( !pPlayer ) { return; }
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

void SceneGame::ClearBackGround() const
{
	constexpr Donya::Vector3 gray = Donya::Color::MakeColor( Donya::Color::Code::GRAY );
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );

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
	nextScene = nextSceneType;

	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeSecond	= Fader::GetDefaultCloseSecond();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneGame::ReturnResult()
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F2 ) && !Fader::Get().IsExist() )
	{
		Donya::Sound::Play( Music::DEBUG_Strong );

		StartFade( Scene::Type::Title );
	}
#endif // DEBUG_MODE

	// TODO: Temporary condition, should fix this
	if ( Fader::Get().IsClosed() && nextScene != Scene::Type::Null )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = nextScene;
		return change;
	}

	bool requestPause	= controller.Trigger( Donya::Gamepad::Button::START ) || controller.Trigger( Donya::Gamepad::Button::SELECT ) || Donya::Keyboard::Trigger( 'P' );
	bool allowPause		= !Fader::Get().IsExist();
	if ( 0 && requestPause && allowPause )
	{
	#if DEBUG_MODE
		Donya::Sound::Play( Music::DEBUG_Weak );
	#endif // DEBUG_MODE

		Scene::Result pause{};
		pause.AddRequest( Scene::Request::ADD_SCENE );
		pause.sceneType = Scene::Type::Pause;
		return pause;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
#include <functional>
#include "Donya/Useful.h"
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
	static GuiWindow adjustWindow{ { 194.0f, 510.0f }, { 364.0f, 300.0f } };
	static GuiWindow playerWindow{ {   0.0f,   0.0f }, { 360.0f, 180.0f } };
	static GuiWindow enemyWindow { {   0.0f,   0.0f }, { 360.0f, 180.0f } };
	static GuiWindow bossWindow  { {   0.0f,   0.0f }, { 360.0f, 180.0f } };
	static bool enableFloatWindow = true;
	static GuiWindow testTileWindow{ {   0.0f,   0.0f }, { 360.0f, 180.0f } };
}
void SceneGame::UseImGui()
{
	UseScreenSpaceImGui();

	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else

	sceneParam.ShowImGuiNode( u8"ゲームシーンのパラメータ" );

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
		auto ApplyToEnemy	= [&]( const CSVLoader &loadedData )
		{
			Enemy::Admin::Get().ClearInstances();
			Enemy::Admin::Get().RemakeByCSV( loadedData );

			if ( thenSave )
			{
				Enemy::Admin::Get().SaveEnemies( stageNumber, /* fromBinary = */ true  );
				Enemy::Admin::Get().SaveEnemies( stageNumber, /* fromBinary = */ false );
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
			if ( !pPlayerIniter ) { pPlayerIniter = std::make_unique<PlayerInitializer>(); }
			pPlayerIniter->RemakeByCSV( loadedData );

			if ( thenSave )
			{
				pPlayerIniter->SaveBin ( stageNumber );
				pPlayerIniter->SaveJson( stageNumber );
			}

			PlayerInit();
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
			static BufferType bufferDirectory	{ "./../../EdittedData/"	};
			static BufferType bufferPrefix		{ "PlatformMap_"			};
			static BufferType bufferBoss		{ "Boss"					};
			static BufferType bufferClear		{ "Clear"					};
			static BufferType bufferEnemy		{ "Enemy"					};
			static BufferType bufferMap			{ "Map"						};
			static BufferType bufferRoom		{ "Room"					};
			static BufferType bufferExtension	{ ".csv"					};

			if ( ImGui::Button( u8"読み込み開始" ) )
			{
				Bullet::Admin::Get().ClearInstances();

				const std::string fileDirectory	= bufferDirectory.data();
				const std::string filePrefix	= bufferPrefix.data();
				const std::string fileExtension	= bufferExtension.data();
				std::string	filePath{};
				CSVLoader	loader{};
				auto ProcessOf = [&]( const BufferType &bufferIdentify, const std::function<void( const CSVLoader & )> &ApplyToXXX )
				{
					filePath =	bufferDirectory.data() +
								filePrefix + bufferIdentify.data() +
								bufferExtension.data();

					loader.Clear();
					loader = PrepareCSVData( filePath );

					if ( IsValidData( loader ) )
					{
						ApplyToXXX( loader );
					}
				};

				ProcessOf( bufferBoss,	ApplyToBoss		);
				ProcessOf( bufferClear,	ApplyToClear	);
				ProcessOf( bufferEnemy,	ApplyToEnemy	);
				ProcessOf( bufferMap,	ApplyToMap		);
				ProcessOf( bufferMap,	ApplyToPlayer	);
				ProcessOf( bufferRoom,	ApplyToRoom		);
			}

			ImGui::InputText( u8"ディレクトリ",			bufferDirectory.data(),	bufferSize );
			ImGui::InputText( u8"接頭辞",				bufferPrefix.data(),	bufferSize );
			ImGui::InputText( u8"識別子・ボス",			bufferBoss.data(),		bufferSize );
			ImGui::InputText( u8"識別子・クリアイベント",	bufferClear.data(),		bufferSize );
			ImGui::InputText( u8"識別子・敵",			bufferEnemy.data(),		bufferSize );
			ImGui::InputText( u8"識別子・マップ＆自機",	bufferMap.data(),		bufferSize );
			ImGui::InputText( u8"識別子・ルーム",			bufferRoom.data(),		bufferSize );
			ImGui::InputText( u8"拡張子",				bufferExtension.data(),	bufferSize );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"個別ロード" ) )
		{
			static bool applyBoss	= false;
			static bool applyClear	= false;
			static bool applyEnemy	= false;
			static bool applyMap	= true;
			static bool applyPlayer	= true;
			static bool applyRoom	= false;

			ImGui::Checkbox( u8"ボスに適用",				&applyBoss		);
			ImGui::Checkbox( u8"クリアイベントに適用",	&applyClear		);
			ImGui::Checkbox( u8"敵に適用",				&applyEnemy		);
			ImGui::Checkbox( u8"マップに適用",			&applyMap		); ImGui::SameLine();
			ImGui::Checkbox( u8"自機に適用",				&applyPlayer	);
			ImGui::Checkbox( u8"ルームに適用",			&applyRoom		);
		
			if ( ImGui::Button( u8"CSVファイルを読み込む" ) )
			{
				const auto filePath	= FetchStageFilePathByCommonDialog();
				const auto loader	= PrepareCSVData( filePath );
				if ( IsValidData( loader ) )
				{
					Bullet::Admin::Get().ClearInstances();

					if ( applyBoss		) { ApplyToBoss		( loader ); }
					if ( applyClear		) { ApplyToClear	( loader ); }
					if ( applyEnemy		) { ApplyToEnemy	( loader ); }
					if ( applyMap		) { ApplyToMap		( loader );	}
					if ( applyPlayer	) { ApplyToPlayer	( loader );	}
					if ( applyRoom		) { ApplyToRoom		( loader );	}
				}
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

		if ( pPlayer ) { pPlayer->ShowImGuiNode( u8"自機の現在" ); }
		Player::UpdateParameter( u8"自機のパラメータ" );
		ImGui::Text( "" );

		if ( pMap    ) { pMap->ShowImGuiNode( u8"マップの現在", stageNumber ); }
		if ( pHouse  ) { pHouse->ShowImGuiNode( u8"部屋の現在", stageNumber ); }
		ImGui::Text( "" );

		Bullet::Admin::Get().ShowImGuiNode( u8"弾の現在" );
		Bullet::Parameter::Update( u8"弾のパラメータ" );
		ImGui::Text( "" );

		Enemy::Admin::Get().ShowImGuiNode( u8"敵の現在", stageNumber );
		Enemy::Parameter::Update( u8"敵のパラメータ" );
		ImGui::Text( "" );

		if ( pBossContainer ) { pBossContainer->ShowImGuiNode( u8"ボスの現在", stageNumber ); }
		Boss::Parameter::Update( u8"ボスのパラメータ" );

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

	// Enemies
	{
		auto Show = [&]( const Donya::Vector2 &ssPos, size_t enemyIndex, auto ShowInstanceNodeMethod )
		{
			enemyWindow.pos = ssPos;
			enemyWindow.SetNextWindow();

			const std::string caption = u8"敵" + Donya::MakeArraySuffix( enemyIndex );

			if ( ImGui::BeginIfAllowed( caption.c_str() ) )
			{
				ShowInstanceNodeMethod();
				Enemy::Parameter::Update( u8"敵のパラメータ" );

				ImGui::End();
			}
		};

		auto  &enemyAdmin		= Enemy::Admin::Get();
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
