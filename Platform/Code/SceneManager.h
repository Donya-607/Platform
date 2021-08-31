#ifndef INCLUDED_SCENE_MANAGER_H_
#define INCLUDED_SCENE_MANAGER_H_

#include <memory>
#include <list>

#include "Scene.h"	// Use Scene::Result, Scene::Type.

/// <summary>
/// You must call Init() when create.
/// </summary>
class SceneMng
{
private:
	std::list<std::unique_ptr<Scene>> pScenes;
public:
	SceneMng()  = default;
	~SceneMng() = default;
	SceneMng( const	SceneMng &  ) = delete;
	SceneMng(		SceneMng && ) = delete;
	SceneMng & operator = ( const SceneMng &  ) = delete;
	SceneMng & operator = (		  SceneMng && ) = delete;
public:
	void Init( Scene::Type initScene );
	void Uninit();
	void Update();
	void Draw();
private:
	/// <summary>
	/// Returns true if the scenes will be empty after I process the message.
	/// </summary>
	bool WillEmptyIfApplied( Scene::Result message ) const;
	/// <summary>
	/// Returns false if the message is wrong.
	/// </summary>
	bool ValidateMessage( Scene::Result message ) const;
	Scene::Result ApplyFailSafe( Scene::Result wrongMessage ) const;

	void ProcessMessage( Scene::Result message, int &refUpdateCount, int &refLoopIndex );

	template<class SceneName>
	void PushSceneImpl( bool toFront )
	{
		if ( toFront )
		{
			pScenes.push_front( std::make_unique<SceneName>() );
			pScenes.front()->Init();
		}
		else
		{
			pScenes.push_back ( std::make_unique<SceneName>() );
			pScenes.back()->Init();
		}
	}
	/// <summary>
	/// Also doing scene initialize.
	/// </summary>
	void PushScene( Scene::Type type, bool toFront );
	void PopScene( bool fromFront );

	void PopAll();
};

#endif // INCLUDED_SCENE_MANAGER_H_