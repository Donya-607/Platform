#pragma once

#include <memory>

#include "Donya/Camera.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "Renderer.h"
#include "Scene.h"

class SceneTitle : public Scene
{
private:
	Donya::ICamera						iCamera;
	Donya::XInput						controller{ Donya::Gamepad::PAD_1 };

	std::unique_ptr<RenderingHelper>	pRenderer;

#if DEBUG_MODE
	bool nowDebugMode			= false;
	bool isReverseCameraMoveX	= false;
	bool isReverseCameraMoveY	= true;
	bool isReverseCameraRotX	= false;
	bool isReverseCameraRotY	= false;
#endif // DEBUG_MODE
public:
	SceneTitle() : Scene() {}
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
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#endif // USE_IMGUI
};
