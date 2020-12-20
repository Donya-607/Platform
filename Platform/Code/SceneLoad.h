#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <thread>

#include "Donya/UseImGui.h"

#include "Performances/LoadPart.h"
#include "Scene.h"
#include "Thread.h"

class SceneLoad : public Scene
{
private:
	enum ThreadKind
	{
		Effect = 0,
		Models,
		Sounds,
		Sprites,

		Renderer,

		ThreadCount
	};
private:
	std::array<Thread, ThreadKind::ThreadCount> threads;

	Performer::LoadPart loadPerformer;

#if DEBUG_MODE
	float elapsedTimer	= 0;
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
	bool	AllFinished() const;
	bool	AllSucceeded() const;
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
