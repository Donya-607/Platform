#include "SceneLogo.h"

#include "Donya/Constant.h"
#include "Donya/Donya.h"		// Use ClearViews()
#include "Donya/Keyboard.h"
#include "Donya/Sprite.h"

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"


namespace
{
	constexpr int	FADE_IN_FRAME	= 20;
	constexpr int	WAIT_FRAME		= 45;
	constexpr int	FADE_OUT_FRAME	= 20;

	constexpr float	FADE_IN_SPEED	= 1.0f / scast<float>( FADE_IN_FRAME  );
	constexpr float	FADE_OUT_SPEED	= 1.0f / scast<float>( FADE_OUT_FRAME );
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
	timer		= 0;
	alpha		= 0.0f;
	scale		= 1.0f;
}

void SceneLogo::Uninit()
{

}

Scene::Result SceneLogo::Update( float elapsedTime )
{
	if ( Donya::Keyboard::Trigger( VK_RETURN ) )
	{
		if ( status != State::FADE_OUT && status != State::END )
		{
			InitFadeOut();
		}
	}

	switch ( status )
	{
	case SceneLogo::State::FADE_IN:		UpdateFadeIn	( elapsedTime ); break;
	case SceneLogo::State::WAIT:		UpdateWait		( elapsedTime ); break;
	case SceneLogo::State::FADE_OUT:	UpdateFadeOut	( elapsedTime ); break;
	default: break;
	}

	return ReturnResult();
}

void SceneLogo::Draw( float elapsedTime )
{
	ClearBackGround();

	Donya::Sprite::DrawExt
	(
		sprites[showIndex],
		Common::HalfScreenWidthF(),
		Common::HalfScreenHeightF(),
		scale, scale,
		0.0f, alpha
	);
}

void SceneLogo::InitFadeIn()
{
	timer  = 0;
	alpha  = 0.0f;
	status = State::FADE_IN;
}
void SceneLogo::UpdateFadeIn( float elapsedTime )
{
	timer++;

	alpha += FADE_IN_SPEED;

	if ( FADE_IN_FRAME <= timer )
	{
		InitWait();
	}
}

void SceneLogo::InitWait()
{
	timer  = 0;
	alpha  = 1.0f;
	status = State::WAIT;
}
void SceneLogo::UpdateWait( float elapsedTime )
{
	timer++;

	if ( WAIT_FRAME <= timer )
	{
		InitFadeOut();
	}
}

void SceneLogo::InitFadeOut()
{
	timer  = 0;
	alpha  = 1.0f;
	status = State::FADE_OUT;
}
void SceneLogo::UpdateFadeOut( float elapsedTime )
{
	timer++;

	alpha -= FADE_OUT_SPEED;

	if ( FADE_OUT_FRAME <= timer )
	{
		showIndex++;

		constexpr int logoCount = scast<int>( showLogos.size() );
		if ( logoCount <= showIndex )
		{
			showIndex = logoCount - 1;
			InitEnd();
		}
		else
		{
			InitFadeIn();
		}
	}
}

void SceneLogo::InitEnd()
{
	timer  = 0;
	alpha  = 0.0f;
	scale  = 1.0f;
	status = State::END;

	// StartFade();
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
	config.closeFrame	= Fader::GetDefaultCloseFrame();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneLogo::ReturnResult()
{
	if ( status == State::END )
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
