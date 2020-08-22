#include "SceneTitle.h"

#include <vector>

#include "Donya/Color.h"			// Use ClearBackGround(), StartFade().
#include "Donya/Keyboard.h"			// Make an input of player.
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Useful.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"
#include "ModelHelper.h"			// Use serialize methods
#include "Music.h"
#include "Parameter.h"

#undef max
#undef min

namespace
{
	struct Member
	{
		struct
		{
			float slerpFactor = 0.2f;
			Donya::Vector3 offsetPos;	// The offset of position from the player position.
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
				CEREAL_NVP( camera.offsetPos			),
				CEREAL_NVP( camera.offsetFocus			)
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( directionalLight ) );
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
				ImGui::DragFloat ( u8"補間倍率",						&camera.slerpFactor,		0.01f );
				ImGui::DragFloat3( u8"自身の座標（自機からの相対）",	&camera.offsetPos.x,		0.01f );
				ImGui::DragFloat3( u8"注視点の座標（自機からの相対）",	&camera.offsetFocus.x,	0.01f );

				ImGui::TreePop();
			}

			ImGui::Helper::ShowDirectionalLightNode( u8"平行光", &directionalLight );
		}
	#endif // USE_IMGUI
	};

	static ParamOperator<Member> sceneParam{ "SceneTitle" };
	Member FetchParameter()
	{
		return sceneParam.Get();
	}
}
CEREAL_CLASS_VERSION( Member, 1 )

void SceneTitle::Init()
{
	// Donya::Sound::Play( Music::BGM_Title );
#if DEBUG_MODE
	// Donya::Sound::AppendFadePoint( Music::BGM_Title, 2.0f, 0.0f, true ); // Too noisy.
#endif // DEBUG_MODE

	bool result{};

	pRenderer = std::make_unique<RenderingHelper>();
	result = pRenderer->Init();
	assert( result );

	using Spr = SpriteAttribute;
	result = sprTitleLogo.LoadSprite( GetSpritePath( Spr::TitleLogo ), GetSpriteInstanceCount( Spr::TitleLogo ) );
	assert( result );

	sceneParam.LoadParameter();

	CameraInit();
}
void SceneTitle::Uninit()
{
	//Donya::Sound::Stop( Music::BGM_Title );
}

Scene::Result SceneTitle::Update( float elapsedTime )
{
	elapsedTime = 1.0f; // Disable

#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F5 ) )
	{
		nowDebugMode = !nowDebugMode;

		if ( nowDebugMode )
		{
			iCamera.ChangeMode( Donya::ICamera::Mode::Free );
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

	CameraUpdate();

	return ReturnResult();
}

void SceneTitle::Draw( float elapsedTime )
{
	elapsedTime = 1.0f; // Disable

	ClearBackGround();

	const Donya::Vector4x4 VP{ iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix() };
	const auto &data = FetchParameter();
	
	{
		Donya::Model::Constants::PerScene::Common constant{};
		constant.directionalLight	= data.directionalLight;
		constant.eyePosition		= Donya::Vector4{ iCamera.GetPosition(), 1.0f };
		constant.viewProjMatrix		= VP;
		pRenderer->UpdateConstant( constant );
	}

	{
		// const auto &trans = data.transparency;
		// RenderingHelper::TransConstant constant{};
		// constant.zNear				= trans.zNear;
		// constant.zFar				= trans.zFar;
		// constant.lowerAlpha			= trans.lowerAlpha;
		// constant.heightThreshold	= pPlayer->GetPosition().y + trans.heightThreshold;
		// pRenderer->UpdateConstant( constant );
	}
	
	pRenderer->ActivateDepthStencilModel();
	pRenderer->ActivateRasterizerModel();
	pRenderer->ActivateSamplerModel();
	pRenderer->ActivateConstantScene();
	// pRenderer->ActivateConstantTrans();
	{
		// The drawing priority is determined by the priority of the information.

		// pRenderer->ActivateShaderNormalSkinning();
		// pRenderer->DeactivateShaderNormalSkinning();

		// pRenderer->ActivateShaderNormalStatic();
		// pRenderer->DeactivateShaderNormalStatic();
	}
	// pRenderer->DeactivateConstantTrans();
	pRenderer->DeactivateConstantScene();
	pRenderer->DeactivateDepthStencilModel();
	pRenderer->DeactivateRasterizerModel();
	pRenderer->DeactivateSamplerModel();

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{

	}
#endif // DEBUG_MODE

	// Drawing to far for avoiding to trans the BG's color.

	{
		sprTitleLogo.pos.x = Common::HalfScreenWidthF();
		sprTitleLogo.pos.y = Common::HalfScreenHeightF();
		sprTitleLogo.alpha = 1.0f;
		sprTitleLogo.Draw();
	}

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

void SceneTitle::CameraInit()
{
	iCamera.Init( Donya::ICamera::Mode::Look );
	iCamera.SetZRange( 0.1f, 1000.0f );
	iCamera.SetFOV( ToRadian( 30.0f ) );
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
void SceneTitle::AssignCameraPos()
{
	const auto &data = FetchParameter();
	
	iCamera.SetPosition  ( data.camera.offsetPos   );
	iCamera.SetFocusPoint( data.camera.offsetFocus );
}
void SceneTitle::CameraUpdate()
{
	Donya::ICamera::Controller input{};
	input.SetNoOperation();
	input.slerpPercent = FetchParameter().camera.slerpFactor;

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
			constexpr float MOVE_SPEED = 0.1f;
			movement.x = diff.x * MOVE_SPEED;
			movement.y = diff.y * MOVE_SPEED;

			if ( isReverseCameraMoveX ) { movement.x *= -1.0f; }
			if ( isReverseCameraMoveY ) { movement.y *= -1.0f; }
		}

		constexpr float FRONT_SPEED = 3.5f;
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

void SceneTitle::ClearBackGround() const
{
	constexpr Donya::Vector3 gray = Donya::Color::MakeColor( Donya::Color::Code::LIGHT_GRAY );
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );

#if DEBUG_MODE
	if ( nowDebugMode )
	{
		constexpr Donya::Vector3 teal = Donya::Color::MakeColor( Donya::Color::Code::TEAL );
		constexpr FLOAT DEBUG_COLOR[4]{ teal.x, teal.y, teal.z, 1.0f };
		Donya::ClearViews( DEBUG_COLOR );
	}
#endif // DEBUG_MODE
}

void SceneTitle::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeSecond	= Fader::GetDefaultCloseSecond();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneTitle::ReturnResult()
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
		change.sceneType = Scene::Type::Game;
		return change;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
void SceneTitle::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"タイトル・メンバーの調整" ) )
		{
			ImGui::Text( u8"「Ｆ５キー」を押すと，" );
			ImGui::Text( u8"背景の色が変わりデバッグモードとなります。" );
			ImGui::Text( "" );

			sceneParam.ShowImGuiNode( u8"シーンのパラメータ調整" );

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

			ImGui::TreePop();
		}

		ImGui::End();
	}
}
#endif // USE_IMGUI
