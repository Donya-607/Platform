#include "SceneResult.h"

#include <algorithm>				// Use std::find
#include <vector>

#undef max
#undef min

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

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"
#include "Parameter.h"

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

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			// archive( CEREAL_NVP( x ) );

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		void ShowImGuiNode()
		{
			
		}
	};

	// static ParamOperator<SceneParam> sceneParam{ "SceneResult" };
	// const SceneParam &FetchParameter()
	// {
	// 	return sceneParam.Get();
	// }
}
CEREAL_CLASS_VERSION( SceneParam, 0 )

void SceneResult::Init()
{
	// Donya::Sound::Play( Music::BGM_Game );
#if DEBUG_MODE
	// Donya::Sound::AppendFadePoint( Music::BGM_Game, 2.0f, 0.0f, true ); // Too noisy.
#endif // DEBUG_MODE

	bool result{};

#if DEBUG_MODE
	result = sprTmpDraw.LoadSprite( L"./Data/Images/Result/GotWeapon.png", 2U );
	assert( result );
#endif // DEBUG_MODE

	CameraInit();
}
void SceneResult::Uninit()
{
	// Donya::Sound::Stop( Music::BGM_Game );
}

Scene::Result SceneResult::Update( float elapsedTime )
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

	CameraUpdate();

	return ReturnResult();
}

void SceneResult::Draw( float elapsedTime )
{
	// elapsedTime = 1.0f; // Disable

	ClearBackGround();

	const Donya::Vector4x4 VP{ iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix() };
	

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{

	}
#endif // DEBUG_MODE

#if DEBUG_MODE
	sprTmpDraw.pos.x = Common::HalfScreenWidthF();
	sprTmpDraw.pos.y = Common::HalfScreenHeightF();
	sprTmpDraw.Draw();
#endif // DEBUG_MODE

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= VP;
		constant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 0.6f };
		constant.lightDirection = Donya::Vector3{ 0.0f, -1.0f, 0.0f };
		
		auto DrawCube = [&]( const Donya::Vector3 &pos, const Donya::Vector3 &scale = { 1.0f, 1.0f, 1.0f } )
		{
			constant.matWorld._11 = scale.x * 2.0f;
			constant.matWorld._22 = scale.y * 2.0f;
			constant.matWorld._33 = scale.z * 2.0f;
			constant.matWorld._41 = pos.x;
			constant.matWorld._42 = pos.y;
			constant.matWorld._43 = pos.z;
			// pRenderer->ProcessDrawingCube( constant );
		};

	}
#endif // DEBUG_MODE
}

void SceneResult::CameraInit()
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
void SceneResult::AssignCameraPos()
{
#if DEBUG_MODE
	constexpr Donya::Vector3 focus{ 0.0f, 0.0f,  0.0f };
	constexpr Donya::Vector3 eye  { 0.0f, 0.0f, -1.0f };
	iCamera.SetPosition  ( eye   );
	iCamera.SetFocusPoint( focus );
#endif // DEBUG_MODE
}
void SceneResult::CameraUpdate()
{
	Donya::ICamera::Controller input{};
	input.SetNoOperation();
	input.slerpPercent = 1.0f;

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
		}
		else
		if ( Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) )
		{
			constexpr float MOVE_SPEED = 0.05f;
			movement.x = -diff.x * MOVE_SPEED;
			movement.y = diff.y * MOVE_SPEED;
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

void SceneResult::ClearBackGround() const
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
void SceneResult::StartFade()
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();;
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneResult::ReturnResult()
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F2 ) && !Fader::Get().IsExist() )
	{
		Donya::Sound::Play( Music::DEBUG_Strong );

		StartFade();
	}
#endif // DEBUG_MODE

	// TODO: Temporary condition, should fix this
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
void SceneResult::UseImGui()
{
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else
	
	// sceneParam.ShowImGuiNode( u8"リザルトシーンのパラメータ" );

	ImGui::End();
}
#endif // USE_IMGUI
