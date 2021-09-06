#include "SceneLogo.h"

#include "Donya/Blend.h"
#include "Donya/Constant.h"
#include "Donya/Controller.h"
#include "Donya/Donya.h"		// Use ClearViews()
#include "Donya/Keyboard.h"
#include "Donya/Sprite.h"

#if DEBUG_MODE
#include "Donya/UseImGui.h"
#endif // DEBUG_MODE

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"

#define USE_REAL_TIME_BASE	( true )
#define USE_FADE_TRANSITION	( false )

namespace
{
	namespace SkipInput
	{
		constexpr int keyboard[]
		{
			VK_RETURN,
			VK_SHIFT,
			VK_SPACE,
			'Z',
			'X',
		};
		constexpr Donya::Controller::Button xinput[]
		{
			Donya::Controller::Button::A,
			Donya::Controller::Button::B,
			Donya::Controller::Button::X,
			Donya::Controller::Button::Y,
			Donya::Controller::Button::START,
			Donya::Controller::Button::SELECT,
		};
	}

	namespace Base
	{
		constexpr int	IN_FRAME	= 20;
		constexpr int	WAIT_FRAME	= 45;
		constexpr int	OUT_FRAME	= 20;
	}

#if USE_REAL_TIME_BASE
	constexpr float	FADE_IN_TIME	= scast<float>( Base::IN_FRAME	) / 60.0f;
	constexpr float	WAIT_TIME		= scast<float>( Base::WAIT_FRAME) / 60.0f;
	constexpr float	FADE_OUT_TIME	= scast<float>( Base::OUT_FRAME	) / 60.0f;
	constexpr float	FADE_IN_SPEED	= 1.0f / FADE_IN_TIME;
	constexpr float	FADE_OUT_SPEED	= 1.0f / FADE_OUT_TIME;
#else
	constexpr int	FADE_IN_TIME	= Base::IN_FRAME;
	constexpr int	WAIT_TIME		= Base::WAIT_FRAME;
	constexpr int	FADE_OUT_TIME	= Base::OUT_FRAME;
	constexpr float	FADE_IN_SPEED	= 1.0f / scast<float>( FADE_IN_TIME  );
	constexpr float	FADE_OUT_SPEED	= 1.0f / scast<float>( FADE_OUT_TIME );
#endif // USE_REAL_TIME_BASE
}

void SceneLogo::Init()
{
	bool succeeded = true;
	for ( size_t i = 0; i < showLogos.size(); ++i )
	{
		sprites[i] = Donya::Sprite::Load( GetSpritePath( showLogos[i] ), 2U );
		if ( sprites[i] == NULL )
		{
			succeeded = false;
		}
	}

	if ( !succeeded )
	{
		_ASSERT_EXPR( 0, L"Error: Logo sprites load is failed!" );
	}

	showIndex	= 0;
	alpha		= 0.0f;
	scale		= 1.0f;
	frameTimer	= 0;
	secondTimer	= 0;

	InitFadeIn();
}
void SceneLogo::Uninit()
{

}
Scene::Result SceneLogo::Update()
{
	// Skip process if requested
	if ( WannaSkip() && status != State::END )
	{
		if ( status == State::FADE_OUT )
		{
			AdvanceLogoIndexOrEnd();
		}
		else
		{
			InitFadeOut();
		}
	}


	// Update process
	const float deltaTime = Donya::GetElapsedTime();
	switch ( status )
	{
	case SceneLogo::State::FADE_IN:		UpdateFadeIn	( deltaTime ); break;
	case SceneLogo::State::WAIT:		UpdateWait		( deltaTime ); break;
	case SceneLogo::State::FADE_OUT:	UpdateFadeOut	( deltaTime ); break;
	default: break;
	}


#if USE_IMGUI
	if ( ImGui::BeginIfAllowed() )
	{
	#if USE_REAL_TIME_BASE
		ImGui::DragFloat( u8"タイマー", &secondTimer, 0.01f );
	#else
		ImGui::DragInt( u8"タイマー", &frameTimer );
	#endif // USE_REAL_TIME_BASE

		ImGui::SliderFloat( u8"アルファ", &alpha, 0.0f, 1.0f );

		ImGui::End();
	}
#endif // USE_IMGUI


	return ReturnResult();
}
void SceneLogo::Draw()
{
	ClearBackGround();

	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );
	Donya::Sprite::DrawExt
	(
		sprites[showIndex],
		Common::HalfScreenWidthF(),
		Common::HalfScreenHeightF(),
		scale, scale,
		0.0f, alpha
	);
}

bool SceneLogo::WannaSkip() const
{
	for ( const auto &key : SkipInput::keyboard )
	{
		if ( Donya::Keyboard::Trigger( key ) ) { return true; }
	}

	if ( !Donya::Controller::IsConnected( Donya::Controller::Pad0 ) ) { return false; }
	// else

	for ( const auto &button : SkipInput::xinput )
	{
		if ( Donya::Controller::Trigger( button ) ) { return true; }
	}

	return false;
}
bool SceneLogo::HasRemainLogo() const
{
	constexpr int logoCount = scast<int>( showLogos.size() );
	return ( showIndex < logoCount );
}

void SceneLogo::AdvanceLogoIndexOrEnd()
{
	showIndex++;

	if ( HasRemainLogo() )
	{
		// Show next logo
		InitFadeIn();
	}
	else
	{
		// Goto next scene
		showIndex = scast<int>( showLogos.size() ) - 1;
		InitEnd();
	}
}

void SceneLogo::InitFadeIn()
{
	alpha		= 0.0f;
	status		= State::FADE_IN;
	frameTimer	= 0;
	secondTimer	= 0;
}
void SceneLogo::UpdateFadeIn( float deltaTime )
{
	bool done = false;

#if USE_REAL_TIME_BASE
	secondTimer	+= deltaTime;
	alpha		+= FADE_IN_SPEED * deltaTime;
	done		= ( FADE_IN_TIME <= secondTimer );
#else
	frameTimer	+= 1;
	alpha		+= FADE_IN_SPEED;
	done		= ( FADE_IN_TIME <= frameTimer );
#endif // USE_REAL_TIME_BASE

	if ( done )
	{
		InitWait();
	}
}

void SceneLogo::InitWait()
{
	alpha		= 1.0f;
	status		= State::WAIT;
	frameTimer	= 0;
	secondTimer	= 0;
}
void SceneLogo::UpdateWait( float deltaTime )
{
	bool done = false;

#if USE_REAL_TIME_BASE
	secondTimer	+= deltaTime;
	done		= ( WAIT_TIME <= secondTimer );
#else
	frameTimer	+= 1;
	done		= ( WAIT_TIME <= frameTimer );
#endif // USE_REAL_TIME_BASE

	if ( done )
	{
		InitFadeOut();
	}
}

void SceneLogo::InitFadeOut()
{
	alpha		= 1.0f;
	status		= State::FADE_OUT;
	frameTimer	= 0;
	secondTimer	= 0;
}
void SceneLogo::UpdateFadeOut( float deltaTime )
{
	bool done = false;

#if USE_REAL_TIME_BASE
	secondTimer	+= deltaTime;
	alpha		-= FADE_OUT_SPEED * deltaTime;
	done		= ( FADE_OUT_TIME <= secondTimer );
#else
	frameTimer	+= 1;
	alpha		-= FADE_OUT_SPEED;
	done		= ( FADE_OUT_TIME <= frameTimer );
#endif // USE_REAL_TIME_BASE

	if ( done )
	{
		AdvanceLogoIndexOrEnd();
	}
}

void SceneLogo::InitEnd()
{
	alpha		= 0.0f;
	scale		= 1.0f;
	status		= State::END;
	frameTimer	= 0;
	secondTimer	= 0;

#if USE_FADE_TRANSITION
	StartFade();
#endif // USE_FADE_TRANSITION
}

void SceneLogo::ClearBackGround() const
{
	constexpr Donya::Vector3 gray{ Donya::Color::MakeColor( Donya::Color::Code::GRAY ) };
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );
}
void SceneLogo::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeSecond	= Fader::GetDefaultCloseSecond();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneLogo::ReturnResult()
{
#if USE_FADE_TRANSITION
	if ( Fader::Get().IsClosed() )
#else
	if ( status == State::END )
#endif // USE_FADE_TRANSITION
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Load;
		return change;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}
