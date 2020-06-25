#pragma once

#include <memory>
#include <vector>

#include "Donya/Camera.h"
#include "Donya/Collision.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "Map.h"
#include "ObjectBase.h"
#include "Player.h"
#include "Renderer.h"
#include "Scene.h"

class SceneGame : public Scene
{
private:
	Donya::ICamera						iCamera;
	Donya::XInput						controller{ Donya::Gamepad::PAD_1 };
	Donya::Collision::Box2F				currentScreen;

	std::unique_ptr<RenderingHelper>	pRenderer;

	std::unique_ptr<Player>				pPlayer;
	std::unique_ptr<Map>				pMap;

#if DEBUG_MODE
	bool wantSuppressElapsedTime	= false;
	bool nowDebugMode				= false;
	bool isReverseCameraMoveX		= true;
	bool isReverseCameraMoveY		= false;
	bool isReverseCameraRotX		= false;
	bool isReverseCameraRotY		= false;
#endif // DEBUG_MODE
public:
	SceneGame() : Scene() {}
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	Donya::Vector4x4 MakeScreenTransform() const;
	Donya::Collision::Box2F CalcCurrentScreenPlane() const;

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
