#pragma once

#include <memory>
#include <vector>

#include "Donya/Camera.h"
#include "Donya/Collision.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "ClearEvent.h"
#include "Map.h"
#include "ObjectBase.h"
#include "Player.h"
#include "Renderer.h"
#include "Room.h"
#include "Scene.h"

class SceneGame : public Scene
{
private:
	Donya::ICamera						iCamera;
	Donya::XInput						controller{ Donya::Gamepad::PAD_1 };
	Donya::Collision::Box3F				currentScreen;
	int									currentRoomID	= 0;
	
	Scene::Type							nextScene		= Scene::Type::Null;

	std::unique_ptr<RenderingHelper>	pRenderer;

	std::unique_ptr<Map>				pMap;
	std::unique_ptr<House>				pHouse;
	std::unique_ptr<ClearEvent>			pClearEvent;
	std::unique_ptr<Player>				pPlayer;
	std::unique_ptr<PlayerInitializer>	pPlayerIniter;

	int   stageNumber				= 0;
	float elapsedSecondsAfterMiss	= 0.0f;

#if DEBUG_MODE
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
	Donya::Collision::Box3F CalcCurrentScreenPlane() const;

	void	InitStage( int stageNo );
	void	UninitStage();

	void	CameraInit();
	void	AssignCameraPos();
	void	CameraUpdate();

	void	PlayerInit();
	void	PlayerUpdate( float elapsedTime );

	void	UpdateCurrentRoomID();

	void	Collision_BulletVSEnemy();
	void	Collision_EnemyVSPlayer();

	void	ClearBackGround() const;
	void	StartFade( Scene::Type nextSceneType );
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
	void	UseScreenSpaceImGui();
#endif // USE_IMGUI
};
