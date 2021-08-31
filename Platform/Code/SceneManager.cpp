#include "SceneManager.h"

#include <algorithm>

#include "Donya/Blend.h"	// Change the blend mode for fader object.
#include "Donya/Sprite.h"	// For change the sprites depth.

#include "Fader.h"
#include "Effect/EffectAdmin.h"
#include "GameStatus.h"
#include "SceneGame.h"
#include "SceneLoad.h"
#include "SceneLogo.h"
#include "SceneOver.h"
#include "SceneResult.h"
#include "SceneTitle.h"

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

void SceneMng::Update()
{
	// I do not consider the case that no one scene does not exist, so add some scene.
	if ( pScenes.empty() )
	{
		_ASSERT_EXPR( 0, L"Error: Scene is empty!" );
		PushScene( Scene::Type::Title, true );
	}


	// Update the top scene only,
	// but I also update a next scene if the top scene requests.

	Scene::Result message{};

	int updateCount = 1;
	for ( int i = 0; i < updateCount; ++i )
	{
		auto &itr = ( *std::next( pScenes.begin(), i ) );
		message = itr->Update();

		ProcessMessage( message, updateCount, i );
	}


	// Update sub systems
	Effect::Admin::Get().Update( Status::GetDeltaTime() );

	Fader::Get().Update();
}

void SceneMng::Draw()
{
	// Reset the depth to default
	Donya::Sprite::SetDrawDepth( 1.0f );


	// Draw all scenes from bottom
	const auto &end = pScenes.crend();
	for ( auto it   = pScenes.crbegin(); it != end; ++it )
	{
		( *it )->Draw();
	}


	// Draw the fading object to nearest
	Donya::Sprite::SetDrawDepth( 0.0f );

	// If use AlphaToCoverage mode, the transparency will be strange.
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );
	Fader::Get().Draw();
}

bool SceneMng::WillEmptyIfApplied( Scene::Result message ) const
{
	if ( message.request == Scene::Request::NONE ) { return false; }
	// else


	// Is the message just removes every scene?
	bool willEmpty = false;
	if ( message.HasRequest( Scene::Request::REMOVE_ALL ) )
	{
		willEmpty = true;
	}
	if ( message.HasRequest( Scene::Request::REMOVE_ME ) && pScenes.size() == 1 )
	{
		willEmpty = true;
	}


	// (sceneType == Null) means the message does not add a scene.
	// Returns true if the message just removes all scenes.
	if ( willEmpty && message.sceneType == Scene::Type::Null )
	{
		return true;
	}
	// else

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
	// else


	return true;
}
Scene::Result SceneMng::ApplyFailSafe( Scene::Result wrongMessage ) const
{
	Scene::Result gotoInitialScene;
	gotoInitialScene.AddRequest( Scene::Request::ASSIGN );
	gotoInitialScene.sceneType = Scene::Type::Logo;
	return gotoInitialScene;
}

void SceneMng::ProcessMessage( Scene::Result message, int &refUpdateCount, int &refLoopIndex )
{
	// Is the message correct?
	if ( !ValidateMessage( message ) )
	{
		_ASSERT_EXPR( 0, L"Error: The passed message is wrong!" );

		message = ApplyFailSafe( message );
	}


	// Process requests

	// !Attention to order of process message!
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
			// Adjust the index instead increment the update count
			refLoopIndex--;

			// The loop-index will be increment at Update(), so lower limit is -1.
			refLoopIndex = std::max( -1, refLoopIndex );
		}
		else
		{
			// Request to be the Update() also updates next scene
			refUpdateCount++;

			// Limit the range within the scene count
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
	case Scene::Type::Game:		PushSceneImpl<SceneGame>  ( toFront ); return;
	case Scene::Type::Over:		PushSceneImpl<SceneOver>  ( toFront ); return;
	case Scene::Type::Result:	PushSceneImpl<SceneResult>( toFront ); return;
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