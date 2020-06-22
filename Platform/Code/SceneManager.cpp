#include "SceneManager.h"

#include <algorithm>

#include "Donya/Blend.h"	// Change the blend mode for fader object.
#include "Donya/Sprite.h"	// For change the sprites depth.

#include "Fader.h"
#include "SceneGame.h"
#include "SceneLoad.h"
#include "SceneLogo.h"
#include "SceneTitle.h"
#include "ScenePause.h"

#undef max
#undef min

void SceneMng::Init( Scene::Type initScene )
{
	PushScene( initScene, /* toFront = */ true );

	Fader::Get().Init();
}

void SceneMng::Uninit()
{
	for ( auto &it : pScenes )
	{
		it->Uninit();
	}

	pScenes.clear();

	Fader::Get().Init();
}

void SceneMng::Update( float elapsedTime )
{
	if ( pScenes.empty() )
	{
		_ASSERT_EXPR( 0, L"Error: Scene is empty." );
		// Fail safe.
		PushScene( Scene::Type::Title, true );
	}

	Scene::Result message{};

	int updateCount = 1;
	for ( int i = 0; i < updateCount; ++i )
	{
		auto &itr = ( *std::next( pScenes.begin(), i ) );
		message = itr->Update( elapsedTime );

		ProcessMessage( message, updateCount, i );
	}

	Fader::Get().Update();
}

void SceneMng::Draw( float elapsedTime )
{
	Donya::Sprite::SetDrawDepth( 1.0f );

	const auto &end = pScenes.crend();
	for ( auto it   = pScenes.crbegin(); it != end; ++it )
	{
		( *it )->Draw( elapsedTime );
	}

	Donya::Sprite::SetDrawDepth( 0.0f );

	// If use AlphaToCoverage mode, the transparency will be strange.
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );
	Fader::Get().Draw();
}

bool SceneMng::WillEmptyIfApplied( Scene::Result message ) const
{
	if ( message.request == Scene::Request::NONE ) { return false; }
	// else

	bool willEmpty = false;

	if ( message.HasRequest( Scene::Request::REMOVE_ALL ) )
	{
		willEmpty = true;
	}
	if ( message.HasRequest( Scene::Request::REMOVE_ME ) && pScenes.size() == 1 )
	{
		willEmpty = true;
	}

	if ( willEmpty && message.sceneType == Scene::Type::Null )
	{
		return true;
	}

	return false;
}
bool SceneMng::ValidateMessage( Scene::Result message ) const
{
	if ( message.request == Scene::Request::NONE ) { return true; }
	// else

	if ( WillEmptyIfApplied( message ) )
	{
		return false;
	}

	return true;
}
Scene::Result SceneMng::ApplyFailSafe( Scene::Result wrongMessage ) const
{
	if ( WillEmptyIfApplied( wrongMessage ) )
	{
		wrongMessage.sceneType = Scene::Type::Logo;
	}

	return wrongMessage;
}

void SceneMng::ProcessMessage( Scene::Result message, int &refUpdateCount, int &refLoopIndex )
{
	if ( !ValidateMessage( message ) )
	{
		_ASSERT_EXPR( 0, L"Error: The passed message is wrong!" );

		message = ApplyFailSafe( message );
	}

	// Attention to order of process message.
	// ex) [pop_front() -> push_front()] [push_front() -> pop_front]

	if ( message.HasRequest( Scene::Request::REMOVE_ME ) )
	{
		PopScene( /* fromFront = */ true );
	}

	if ( message.HasRequest( Scene::Request::REMOVE_ALL ) )
	{
		PopAll();
	}

	if ( message.HasRequest( Scene::Request::ADD_SCENE ) )
	{
		PushScene( message.sceneType, /* toFront = */ true );
	}
	
	if ( message.HasRequest( Scene::Request::APPEND_SCENE ) )
	{
		PushScene( message.sceneType, /* toFront = */ false );
	}
	
	if ( message.HasRequest( Scene::Request::UPDATE_NEXT ) )
	{
		if ( message.HasRequest( Scene::Request::REMOVE_ME ) )
		{
			refLoopIndex--;
			refLoopIndex = std::max( -1, refLoopIndex );
			// The loop-index will be increment, so lower limit is -1.
		}
		else
		{
			refUpdateCount++;
			refUpdateCount = std::min( scast<int>( pScenes.size() ), refUpdateCount );
		}
	}
}

void SceneMng::PushScene( Scene::Type type, bool toFront )
{
	switch ( type )
	{
	case Scene::Type::Logo:		PushSceneImpl<SceneLogo>  ( toFront ); return;
	case Scene::Type::Load:		PushSceneImpl<SceneLoad>  ( toFront ); return;
	case Scene::Type::Title:	PushSceneImpl<SceneTitle> ( toFront ); return;
	case Scene::Type::Game:		PushSceneImpl<SceneBattle>( toFront ); return;
	case Scene::Type::Pause:	PushSceneImpl<ScenePause> ( toFront ); return;
	default: _ASSERT_EXPR( 0, L"Error: The scene does not exist."   ); return;
	}
}

void SceneMng::PopScene( bool fromFront )
{
	if ( fromFront )
	{
		pScenes.front()->Uninit();
		pScenes.pop_front();
	}
	else
	{
		pScenes.back()->Uninit();
		pScenes.pop_back();
	}
}

void SceneMng::PopAll()
{
	for ( auto &it : pScenes )
	{
		it->Uninit();
	}
	pScenes.clear();
}