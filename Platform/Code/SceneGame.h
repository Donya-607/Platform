#pragma once

#include <memory>
#include <vector>

#include "Donya/Camera.h"
#include "Donya/Collision.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "Boss.h"
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
	enum class State
	{
		StrategyStage,
		BattleBoss,
		ClearStage
	};
	struct Input
	{
		Donya::Vector2 inputDirection; // Controller's stick or directional-pad, or keyboard's arrow key.
		bool pressJump = false; // Current frame.
		bool pressShot = false; // Current frame.
	public:
		void Clear()
		{
			inputDirection = 0.0f;
			pressJump = false;
			pressShot = false;
		}
	};
	struct Scroll
	{
		bool			active = false;
		float			elapsedSecond = 0.0f;
		Donya::Vector3	cameraFocusStart;
		Donya::Vector3	cameraFocusDest;
	};
private:
	Donya::ICamera						iCamera;
	Scroll								scroll;

	Donya::XInput						controller{ Donya::Gamepad::PAD_1 };
	Input								currentInput;
	Donya::Collision::Box3F				currentScreen;
	int									currentRoomID	= 0;
	
	Scene::Type							nextScene		= Scene::Type::Null;
	State								status			= State::StrategyStage;

	std::unique_ptr<RenderingHelper>	pRenderer;

	std::unique_ptr<Map>				pMap;
	std::unique_ptr<House>				pHouse;
	std::unique_ptr<ClearEvent>			pClearEvent;
	std::unique_ptr<Boss::Container>	pBossContainer;
	std::unique_ptr<Player>				pPlayer;
	std::unique_ptr<PlayerInitializer>	pPlayerIniter;

	int		stageNumber				= 0;
	float	elapsedSecondsAfterMiss	= 0.0f;
	bool	isThereClearEvent		= false;
	bool	isThereBoss				= false;

#if DEBUG_MODE
	bool	nowDebugMode			= false;
	bool	isReverseCameraMoveX	= true;
	bool	isReverseCameraMoveY	= false;
	bool	isReverseCameraRotX		= false;
	bool	isReverseCameraRotY		= false;
	const Room *pChosenRoom			= nullptr; // It used for ImGui
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

	void	AssignCurrentInput();

	void	CameraInit();
	Donya::Vector3 ClampFocusPoint( const Donya::Vector3 &focusPoint, int roomID );
	void	PrepareScrollIfNotActive( int oldRoomID, int newRoomID );
	void	AssignCameraPos();
	void	CameraUpdate( float elapsedTime );

	void	PlayerInit();
	void	PlayerUpdate( float elapsedTime, const Map &terrain );
	
	void	BossUpdate( float elapsedTime, const Donya::Vector3 &wsTargetPos );

	int		CalcCurrentRoomID() const;

	void	Collision_BulletVSBoss();
	void	Collision_BulletVSEnemy();
	void	Collision_BulletVSPlayer();
	void	Collision_BossVSPlayer();
	void	Collision_EnemyVSPlayer();
	void	Collision_PlayerVSItem();

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
