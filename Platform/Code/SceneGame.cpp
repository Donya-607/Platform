#include "SceneGame.h"

#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Color.h"			// Use ClearBackGround(), StartFade().
#include "Donya/Constant.h"
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

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"
#include "Parameter.h"
#include "PlayerParam.h"

#if DEBUG_MODE
#include "CSVLoader.h"
#pragma comment( lib, "comdlg32.lib" ) // Used for common-dialog
#endif // DEBUG_MODE

#if DEBUG_MODE
namespace
{
	constexpr int debugTmpStageNo = -1;
}
#endif // DEBUG_MODE

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

		PlayerInitializer testPlayerInit;
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
				CEREAL_NVP( directionalLight.direction	),
				CEREAL_NVP( testPlayerInit				)
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

			testPlayerInit.ShowImGuiNode( u8"自機の初期位置", debugTmpStageNo, false );
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

void SceneBattle::Init()
{
	// Donya::Sound::Play( Music::BGM_Game );
#if DEBUG_MODE
	// Donya::Sound::AppendFadePoint( Music::BGM_Game, 2.0f, 0.0f, true ); // Too noisy.
#endif // DEBUG_MODE

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

	pPlayer = std::make_unique<Player>();
	pPlayer->Init( data.testPlayerInit );

	CameraInit();
}
void SceneBattle::Uninit()
{
	if ( pMap		) { pMap->Uninit();		}
	if ( pPlayer	) { pPlayer->Uninit();	}
	pMap.reset();
	pPlayer.reset();

	// Donya::Sound::Stop( Music::BGM_Game );
}

Scene::Result SceneBattle::Update( float elapsedTime )
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
	if ( pPlayer )
	{
		const bool pressRight = Donya::Keyboard::Press( VK_RIGHT );
		const bool pressLeft  = Donya::Keyboard::Press( VK_LEFT  );
		const bool pressJump  = Donya::Keyboard::Press( VK_SHIFT );

		Player::Input input;
		if ( pressRight ) { input.moveVelocity.x += 1.0f; }
		if ( pressLeft  ) { input.moveVelocity.x -= 1.0f; }
		input.useJump = pressJump;

		pPlayer->Update( elapsedTime, input );
	}

	// Update actor velocity
	{
		if ( Donya::Keyboard::Trigger( VK_SHIFT ) )
		{
			constexpr float strength = 0.8f;
			actorVelocity.y = strength;
		}
		else
		{
			constexpr float gravity		= 0.1f;
			constexpr float resistance	= 0.5f;
			constexpr float maxSpeed	= 0.4f;

			constexpr int resistGravityFrame = 35;
			static int pressTimer = 0;
			pressTimer = ( Donya::Keyboard::Press( VK_SHIFT ) ) ? pressTimer + 1 : 0;


			actorVelocity.y -=	( 0 < pressTimer && pressTimer <= resistGravityFrame )
								? gravity * resistance
								: gravity;
			actorVelocity.y = Donya::Clamp( actorVelocity.y, -maxSpeed, maxSpeed );
		}

		constexpr float accel = 999.0f;
		constexpr float decel = 999.0f;
		constexpr float maxSpeed = 0.05f;
		if ( Donya::Keyboard::Press( VK_RIGHT ) )
		{ actorVelocity.x += accel; }
		else
		if ( Donya::Keyboard::Press( VK_LEFT  ) )
		{ actorVelocity.x -= accel; }
		else
		{
			const int sign = Donya::SignBit( actorVelocity.x );
			actorVelocity.x -= decel * sign;

			const int currSign = Donya::SignBit( actorVelocity.x );
			if ( currSign != sign )
			{
				actorVelocity.x = 0.0f;
			}
		}
		actorVelocity.x = Donya::Clamp( actorVelocity.x, -maxSpeed, maxSpeed );
	}

	// Move the actors
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
		else
		{
			actor.MoveX( actorVelocity.x, hitBoxes );
			actor.MoveZ( actorVelocity.z, hitBoxes );
			const int collideIndex = actor.MoveY( actorVelocity.y, hitBoxes );
			if ( collideIndex != -1 )
			{
				actorVelocity.y = 0.0f;
			}
		}
	}

	CameraUpdate();

	return ReturnResult();
}

void SceneBattle::Draw( float elapsedTime )
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
		if ( pMap ) { pMap->Draw( pRenderer.get() ); }
		pRenderer->DeactivateShaderNormalStatic();
	}
	// pRenderer->DeactivateConstantTrans();
	pRenderer->DeactivateConstantScene();
	pRenderer->DeactivateDepthStencilModel();
	pRenderer->DeactivateRasterizerModel();
	pRenderer->DeactivateSamplerModel();

	// for ( const auto &it : solids )
	// {
	// 	it.DrawHitBox( pRenderer.get(), VP, { 1.0f, 0.5f, 0.0f, 1.0f } );
	// }
	// actor.DrawHitBox( pRenderer.get(), VP, { 0.4f, 1.0f, 0.4f, 1.0f } );

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		if ( pMap		) { pMap->DrawHitBoxes( pRenderer.get(), VP ); }
		if ( pPlayer	) { pPlayer->DrawHitBox( pRenderer.get(), VP ); }
	}
#endif // DEBUG_MODE
		
#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= VP;
		constant.drawColor		= Donya::Vector4{ 0.5f, 1.0f, 0.8f, 0.5f };
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

	}
#endif // DEBUG_MODE
}

void SceneBattle::CameraInit()
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
void SceneBattle::AssignCameraPos()
{
	const auto &data = FetchParameter();
	const Donya::Vector3 playerPos = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3::Zero();

	iCamera.SetPosition  ( playerPos + data.camera.offsetPos   );
	iCamera.SetFocusPoint( playerPos + data.camera.offsetFocus );
}
void SceneBattle::CameraUpdate()
{
	const auto &data = FetchParameter();

#if USE_IMGUI
	iCamera.SetFOV( ToRadian( data.camera.fovDegree ) );
	iCamera.SetProjectionPerspective();
#endif // USE_IMGUI

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

void SceneBattle::ClearBackGround() const
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

void SceneBattle::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();;
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneBattle::ReturnResult()
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
void SceneBattle::UseImGui()
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

		if ( ImGui::Button( u8"CSVファイルからステージを生成" ) )
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
			const auto &data = loader.Get();
			if ( !data.empty() )
			{
				// The data was loaded successfully here

				if ( pMap )
				{
					pMap->RemakeByCSV( loader );
				}
			}
		}

		Player::UpdateParameter( u8"自機のパラメータ" );
		if ( pPlayer ) { pPlayer->ShowImGuiNode( u8"自機の現在" ); }

		if ( pMap ) { pMap->ShowImGuiNode( u8"マップの現在", debugTmpStageNo ); }

		ImGui::TreePop();
	}
	
	ImGui::End();
}
#endif // USE_IMGUI
