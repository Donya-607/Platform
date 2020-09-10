#pragma once

#include <memory>
#include <mutex>
#include <thread>

#include "Donya/Font.h"
#include "Donya/UseImGui.h"

#include "Scene.h"

class SceneLoad : public Scene
{
private:
	bool finishModels	= false;
	bool finishSounds	= false;
	bool finishSprites	= false;
	std::unique_ptr<std::thread> pThreadModels	= nullptr;
	std::unique_ptr<std::thread> pThreadSounds	= nullptr;
	std::unique_ptr<std::thread> pThreadSprites	= nullptr;

	bool allSucceeded	= true;
	std::mutex	succeedMutex;

	std::unique_ptr<Donya::Font::Renderer>	pFontRenderer;
	float		fontAlpha		= 1.0f;
	float		flushingTimer	= 0.0f;

#if DEBUG_MODE
	float		elapsedTimer	= 0;
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
