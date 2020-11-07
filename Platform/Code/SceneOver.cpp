#include "SceneOver.h"

#include <vector>

#undef max
#undef min

#include "Donya/Color.h"			// Use ClearBackGround(), StartFade().
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Common.h"
#include "Effect/EffectAdmin.h"
#include "Fader.h"
#include "FontHelper.h"
#include "Music.h"
#include "Parameter.h"
#include "Player.h"
#include "PlayerParam.h"

namespace
{
#if DEBUG_MODE
	constexpr bool IOFromBinary = false;
#else
	constexpr bool IOFromBinary = true;
#endif // DEBUG_MODE

#if USE_IMGUI
	static bool dontTransition = false;
#endif // USE_IMGUI
}

namespace
{
	struct SceneParam
	{
		float waitToFadeSecond = 3.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( waitToFadeSecond ) );

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode()
		{
			ImGui::DragFloat( u8"遷移までの待機秒数", &waitToFadeSecond, 0.01f );
			waitToFadeSecond = std::max( 0.0f, waitToFadeSecond );
		}
	#endif // USE_IMGUI
	};

	static ParamOperator<SceneParam> sceneParam{ "SceneOver" };
	const SceneParam &FetchParameter()
	{
		return sceneParam.Get();
	}
}
CEREAL_CLASS_VERSION( SceneParam, 0 )

void SceneOver::Init()
{
	Player::Remaining::Set( Player::Parameter().Get().initialRemainCount );

	timer = 0.0f;

	Effect::Admin::Get().ClearInstances();

	Donya::Sound::Play( Music::BGM_Over );
}
void SceneOver::Uninit()
{
	Donya::Sound::Stop( Music::BGM_Over );

	Effect::Admin::Get().ClearInstances();
}

Scene::Result SceneOver::Update( float elapsedTime )
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F2 ) && !Fader::Get().IsExist() )
	{
		Donya::Sound::Play( Music::DEBUG_Strong );

		StartFade();
	}
#endif // DEBUG_MODE

#if USE_IMGUI
	UseImGui();
#endif // USE_IMGUI

	controller.Update();

	timer += elapsedTime;
	if ( FetchParameter().waitToFadeSecond <= timer )
	{
		if ( !Fader::Get().IsExist() )
		{
		#if USE_IMGUI
			if(!dontTransition )
		#endif // USE_IMGUI
			StartFade();
		}
	}

#if 0
	if ( !Fader::Get().IsExist() )
	{
		bool shouldSkip = false;
		if ( controller.IsConnected() )
		{
			shouldSkip = controller.Trigger( Donya::Gamepad::Button::A );
		}
		else
		{
			shouldSkip = Donya::Keyboard::Trigger( 'Z' );
		}

		if ( shouldSkip )
		{
			StartFade();
		}
	}
#endif // 0

	return ReturnResult();
}

void SceneOver::Draw( float elapsedTime )
{
	ClearBackGround();

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{

	}
#endif // DEBUG_MODE

	const auto pFontRenderer = FontHelper::GetRendererOrNullptr( FontAttribute::Main );
	if ( pFontRenderer )
	{
		constexpr Donya::Vector2 pivot { 0.5f, 0.5f };
		constexpr Donya::Vector2 center{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };
		constexpr Donya::Vector4 color { Donya::Color::MakeColor( Donya::Color::Code::LIGHT_GRAY ), 1.0f };
		constexpr Donya::Vector2 scale { 4.0f, 4.0f };

		pFontRenderer->DrawExt( L"GAME OVER", center, pivot, scale, color );
		Donya::Sprite::Flush();
	}
}

void SceneOver::ClearBackGround() const
{
	constexpr Donya::Vector3 gray = Donya::Color::MakeColor( Donya::Color::Code::GRAY );
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );
}
void SceneOver::StartFade()
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeSecond	= Fader::GetDefaultCloseSecond();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneOver::ReturnResult()
{
	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Title;
		return change;
	}

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
void SceneOver::UseImGui()
{
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else
	
	sceneParam.ShowImGuiNode( u8"ゲームオーバーシーンのパラメータ" );

	ImGui::Checkbox( u8"遷移を止める", &dontTransition );

	ImGui::End();
}
#endif // USE_IMGUI
