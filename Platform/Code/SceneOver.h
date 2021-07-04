#pragma once

#include <memory>
#include <vector>

#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "Donya/Camera.h"
#include "Donya/Collision.h"

#include "Scene.h"

class SceneOver : public Scene
{
private:
	Donya::XInput controller{ Donya::Gamepad::PAD_1 };
	
	float timer = 0.0f;

	Donya::ICamera iCamera;

	Donya::Vector3 posA;
	Donya::Collision::Collider colA;
	Donya::Vector3 posB;
	Donya::Collision::Collider colB;
public:
	SceneOver() : Scene() {}
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
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
