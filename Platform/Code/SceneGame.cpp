#include "SceneGame.h"

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
	constexpr int debugTmpStageNo = -1;
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
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
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
		}
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

	wantSuppressElapsedTime = true;
	bool result{};

	pRenderer = std::make_unique<RenderingHelper>();
	result = pRenderer->Init();
	assert( result );

	sceneParam.LoadParameter();
	const auto &data = FetchParameter();
	
	result = Player::LoadResource();
	assert( result );

	pMap = std::make_unique<Map>();
	pMap->Init( debugTmpStageNo );

	pHouse = std::make_unique<House>();
	pHouse->Init( debugTmpStageNo );

	pPlayerIniter = std::make_unique<PlayerInitializer>();
	pPlayerIniter->LoadParameter( debugTmpStageNo );
	PlayerInit();

	currentRoomID = pHouse->CalcBelongRoomID( pPlayer->GetPosition() );

	CameraInit();

	Bullet::Admin::Get().ClearInstances();

	auto &enemyAdmin = Enemy::Admin::Get();
	enemyAdmin.ClearInstances();
	enemyAdmin.LoadEnemies( debugTmpStageNo, IOFromBinary );
	enemyAdmin.SaveEnemies( debugTmpStageNo, true );
}
void SceneGame::Uninit()
{
	if ( pMap		) { pMap->Uninit();		}
	if ( pHouse		) { pHouse->Uninit();	}
	if ( pPlayer	) { pPlayer->Uninit();	}
	pMap.reset();
	pHouse.reset();
	pPlayer.reset();

	Bullet::Admin::Get().ClearInstances();
	Enemy::Admin::Get().ClearInstances();

	// Donya::Sound::Stop( Music::BGM_Game );
}

Scene::Result SceneGame::Update( float elapsedTime )
{
#if DEBUG_MODE
	if ( wantSuppressElapsedTime )
	{
		wantSuppressElapsedTime = false;

		constexpr float preferFPS		= 60.0f;
		constexpr float goodElapsedTime	= 1.0f / preferFPS;
		elapsedTime = goodElapsedTime;
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
			iCamera.Init( Donya::ICamera::Mode::Look );
		}
	}
#endif // DEBUG_MODE

#if USE_IMGUI
	UseImGui();
#endif // USE_IMGUI

	controller.Update();

	if ( pMap )
	{
		pMap->Update( elapsedTime );
	}

	UpdateCurrentRoomID();
	
	PlayerUpdate( elapsedTime );

	const Donya::Vector3 playerPos = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3::Zero();
	Bullet::Admin::Get().Update( elapsedTime, currentScreen );
	Enemy::Admin::Get().Update( elapsedTime, playerPos, currentScreen );

	// PhysicUpdates
	{
		std::vector<Donya::Collision::Box3F> hitBoxes;
		if ( pMap )
		{
			const auto &tiles = pMap->GetTiles();
			for ( const auto &it : tiles )
			{
				hitBoxes.emplace_back( it.GetHitBox() );
			}
		}

		if ( pPlayer )
		{
			pPlayer->PhysicUpdate( elapsedTime, hitBoxes );
		}

		Bullet::Admin::Get().PhysicUpdate( elapsedTime );
		Enemy::Admin::Get().PhysicUpdate( elapsedTime, hitBoxes );
	}

	// CameraUpdate() depends the currentScreen, so I should update that before CameraUpdate().
	currentScreen = CalcCurrentScreenPlane();
	CameraUpdate();

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
	// pRenderer->ActivateConstantTrans();
	{
		// The drawing priority is determined by the priority of the information.

		pRenderer->ActivateShaderNormalSkinning();
		if ( pPlayer ) { pPlayer->Draw( pRenderer.get() ); }
		pRenderer->DeactivateShaderNormalSkinning();

		pRenderer->ActivateShaderNormalStatic();
		Bullet::Admin::Get().Draw( pRenderer.get() );
		Enemy::Admin::Get().Draw( pRenderer.get() );
		if ( pMap ) { pMap->Draw( pRenderer.get() ); }
		pRenderer->DeactivateShaderNormalStatic();
	}
	// pRenderer->DeactivateConstantTrans();
	pRenderer->DeactivateConstantScene();
	pRenderer->DeactivateDepthStencilModel();
	pRenderer->DeactivateRasterizerModel();
	pRenderer->DeactivateSamplerModel();

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		if ( pPlayer	) { pPlayer->DrawHitBox( pRenderer.get(), VP );		}
		if ( pMap		) { pMap->DrawHitBoxes( pRenderer.get(), VP );		}
		Bullet::Admin::Get().DrawHitBoxes( pRenderer.get(), VP );
		Enemy::Admin::Get().DrawHitBoxes( pRenderer.get(), VP );
		if ( pHouse		) { pHouse->DrawHitBoxes( pRenderer.get(), VP );	}
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

	const Donya::Vector2 halfScreenSize
	{
		Common::HalfScreenWidthF(),
		Common::HalfScreenHeightF(),
	};
	const Donya::Vector2 left	{ 0.0f,						halfScreenSize.y		};
	const Donya::Vector2 top	{ halfScreenSize.x,			0.0f					};
	const Donya::Vector2 right	{ halfScreenSize.x * 2.0f,	halfScreenSize.y		};
	const Donya::Vector2 bottom	{ halfScreenSize.x,			halfScreenSize.y * 2.0f	};

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
void SceneGame::PlayerUpdate( float elapsedTime )
{
	if ( !pPlayer ) { return; }
	// else
		
	const bool pressRight = Donya::Keyboard::Press( VK_RIGHT );
	const bool pressLeft  = Donya::Keyboard::Press( VK_LEFT  );
	const bool pressJump  = Donya::Keyboard::Press( VK_SHIFT );
	const bool pressShot  = Donya::Keyboard::Press( 'Z' );

	Player::Input input;
	if ( pressRight ) { input.moveVelocity.x += 1.0f; }
	if ( pressLeft  ) { input.moveVelocity.x -= 1.0f; }
	input.useJump = pressJump;
	input.useShot = pressShot;

	pPlayer->Update( elapsedTime, input );
}

void SceneGame::UpdateCurrentRoomID()
{
	if ( !pPlayer || !pHouse ) { return; }
	// else

	const auto playerPos = pPlayer->GetPosition();
	if ( Donya::Collision::IsHit( playerPos, currentScreen ) ) { return; }
	// else

	const int nextID = pHouse->CalcBelongRoomID( playerPos );
	if ( nextID != Room::invalidID )
	{
		currentRoomID = nextID;
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

void SceneGame::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();;
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneGame::ReturnResult()
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F2 ) && !Fader::Get().IsExist() )
	{
		Donya::Sound::Play( Music::DEBUG_Strong );

		StartFade();
	}
#endif // DEBUG_MODE

	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Title;
		return change;
	}

	bool requestPause	= controller.Trigger( Donya::Gamepad::Button::START ) || controller.Trigger( Donya::Gamepad::Button::SELECT ) || Donya::Keyboard::Trigger( 'P' );
	bool allowPause		= !Fader::Get().IsExist();
	if ( 0 && requestPause && allowPause )
	{
		Donya::Sound::Play( Music::DEBUG_Weak );

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
#include "Room.h"
void SceneGame::UseImGui()
{
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else
	
	if ( ImGui::TreeNode( u8"ゲーム・メンバーの調整" ) )
	{
		ImGui::Text( u8"「Ｆ５キー」を押すと，" );
		ImGui::Text( u8"背景の色が変わりデバッグモードとなります。" );
		ImGui::Text( "" );

		sceneParam.ShowImGuiNode( u8"シーンのパラメータ" );
		const auto &data = FetchParameter();

		if ( ImGui::TreeNode( u8"カメラ情報" ) )
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

			ImGui::Text( u8"【デバッグモード時のみ有効】" );
			ImGui::Text( u8"「ＡＬＴキー」を押している間のみ，" );
			ImGui::Text( u8"「左クリック」を押しながらマウス移動で，" );
			ImGui::Text( u8"カメラの回転ができます。" );
			ImGui::Text( u8"「マウスホイール」を押しながらマウス移動で，" );
			ImGui::Text( u8"カメラの並行移動ができます。" );
			ImGui::Text( "" );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"ステージファイルの読み込み" ) )
		{
			static bool applyMap	= true;
			static bool applyHouse	= false;
			static bool applyPlayer	= true;
			static bool thenSave	= true;
			ImGui::Checkbox( u8"マップに適用",		&applyMap		);
			ImGui::SameLine();
			ImGui::Checkbox( u8"自機に適用",			&applyPlayer	);

			ImGui::Checkbox( u8"ルームに適用",		&applyHouse		);
			
			ImGui::Checkbox( u8"適用後にセーブする",	&thenSave		);

			if ( ImGui::Button( u8"CSVファイルを読み込む" ) )
			{
				auto PrepareCSVData = []()
				{
					CSVLoader loader;
					loader.Clear();

					const auto filePath = FetchStageFilePathByCommonDialog();
					if ( filePath.empty() || !Donya::IsExistFile( filePath ) )
					{
						std::string msg = u8"ファイルロードに失敗しました。\n";
						msg += u8"ファイル：[" + filePath + u8"]";
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
				const auto loader = PrepareCSVData();
				if ( !loader.Get().empty() )
				{
					// The data was loaded successfully here

					wantSuppressElapsedTime = true;

					if ( applyMap && pMap )
					{
						pMap->RemakeByCSV( loader );
						if ( thenSave )
						{
							pMap->SaveMap( debugTmpStageNo, /* fromBinary = */ true  );
							pMap->SaveMap( debugTmpStageNo, /* fromBinary = */ false );
						}
					}

					if ( applyHouse && pHouse )
					{
						pHouse->RemakeByCSV( loader );
						if ( thenSave )
						{
							pHouse->SaveRooms( debugTmpStageNo, /* fromBinary = */ true  );
							pHouse->SaveRooms( debugTmpStageNo, /* fromBinary = */ false );
						}
					}

					if ( applyPlayer )
					{
						if ( !pPlayerIniter ) { pPlayerIniter = std::make_unique<PlayerInitializer>(); }
						pPlayerIniter->RemakeByCSV( loader );
						if ( thenSave )
						{
							pPlayerIniter->SaveBin ( debugTmpStageNo );
							pPlayerIniter->SaveJson( debugTmpStageNo );
						}
						PlayerInit();
					}

					Bullet::Admin::Get().ClearInstances();
				}
			}

			ImGui::TreePop();
		}
		
		Player::UpdateParameter( u8"自機のパラメータ" );
		if ( pPlayer ) { pPlayer->ShowImGuiNode( u8"自機の現在" ); }

		if ( pMap ) { pMap->ShowImGuiNode( u8"マップの現在", debugTmpStageNo ); }

		if ( pHouse ) { pHouse->ShowImGuiNode( u8"部屋の現在", debugTmpStageNo ); }

		ImGui::TreePop();
	}

	Bullet::Parameter::Update( u8"弾のパラメータ" );
	Bullet::Admin::Get().ShowImGuiNode( u8"弾の現在" );
	Enemy::Parameter::Update( u8"敵のパラメータ" );
	Enemy::Admin::Get().ShowImGuiNode( u8"敵の現在", debugTmpStageNo );
	
	ImGui::End();
}
#endif // USE_IMGUI
