#pragma once

#include <memory>
#include <vector>

#include "Donya/Camera.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "ObjectBase.h"
#include "Player.h"
#include "Renderer.h"
#include "Scene.h"

class SceneBattle : public Scene
{
private:
	Donya::ICamera						iCamera;
	Donya::XInput						controller{ Donya::Gamepad::PAD_1 };

	std::unique_ptr<RenderingHelper>	pRenderer;

	std::vector<Solid>					solids;
	std::unique_ptr<Player>				pPlayer;
	Actor								actor;
	Donya::Vector3						actorVelocity;

#if DEBUG_MODE
	bool nowDebugMode			= false;
	bool isReverseCameraMoveX	= true;
	bool isReverseCameraMoveY	= false;
	bool isReverseCameraRotX	= false;
	bool isReverseCameraRotY	= false;
#endif // DEBUG_MODE
public:
	SceneBattle() : Scene() {}
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
