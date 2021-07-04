#pragma once

#include <memory>
#include <vector>

#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include <string>
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

	static std::vector<std::string> callbackStrs;
	static bool hitContinuingA;
	static bool hitContinuingB;
	static void OnHitEnterA		( DONYA_CALLBACK_ON_HIT_ENTER );
	static void OnHitContinueA	( DONYA_CALLBACK_ON_HIT_CONTINUE );
	static void OnHitExitA		( DONYA_CALLBACK_ON_HIT_EXIT );
	static void OnHitEnterB		( DONYA_CALLBACK_ON_HIT_ENTER );
	static void OnHitContinueB	( DONYA_CALLBACK_ON_HIT_CONTINUE );
	static void OnHitExitB		( DONYA_CALLBACK_ON_HIT_EXIT );
#endif // USE_IMGUI
};
#pragma once
