#pragma once

#include <memory>
#include <vector>

#include "Donya/Camera.h"
#include "Donya/Collision.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "Renderer.h"
#include "Scene.h"
#include "UI.h"

class SceneOver : public Scene
{
private:
	Donya::ICamera						iCamera;
	Donya::XInput						controller{ Donya::Gamepad::PAD_1 };
	
	// std::unique_ptr<RenderingHelper>	pRenderer;
	
	// std::unique_ptr<Player>				pPlayer;
	// std::unique_ptr<PlayerInitializer>	pPlayerIniter;

#if DEBUG_MODE
	bool nowDebugMode = false;
#endif // DEBUG_MODE
public:
	SceneOver() : Scene() {}
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	void	CameraInit();
	void	AssignCameraPos();
	void	CameraUpdate();

	void	ClearBackGround() const;
	void	StartFade();
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#endif // USE_IMGUI
};
#pragma once
