#pragma once

#include <memory>
#include <mutex>
#include <thread>

#include "Donya/UseImGui.h"

#include "Scene.h"
#include "UI.h"

class SceneLoad : public Scene
{
private:
	bool finishSprites	= false;
	bool finishSounds	= false;
	std::unique_ptr<std::thread> pThreadSprites = nullptr;
	std::unique_ptr<std::thread> pThreadSounds  = nullptr;

	bool allSucceeded	= true;
	std::mutex	succeedMutex;

	UIObject	sprNowLoading;
	float		flushingTimer = 0.0f;

#if DEBUG_MODE
	float		elapsedTimer = 0;
#endif // DEBUG_MODE
public:
	SceneLoad() : Scene() {}
	~SceneLoad()
	{
		ReleaseAllThread();
	}
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	void	ReleaseAllThread();
private:
	bool	SpritesInit();
	void	SpritesUpdate( float elapsedTime );
private:
	bool	IsFinished() const;
private:
	void	ClearBackGround() const;
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#endif // USE_IMGUI
};
