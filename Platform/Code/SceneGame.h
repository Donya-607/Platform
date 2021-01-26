#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Donya/Camera.h"
#include "Donya/Collision.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "Boss.h"
#include "CheckPoint.h"
#include "ClearEvent.h"
#include "Door.h"
#include "Effect/Effect.h"
#include "Map.h"
#include "Music.h"
#include "ObjectBase.h"
#include "PauseProcessor.h"
#include "Performances/LoadPart.h"
#include "Player.h"
#include "Room.h"
#include "Scene.h"
#include "Sky.h"
#include "Thread.h"

class SceneGame : public Scene
{
public:
	enum class State
	{
		FirstInitialize,
		Stage,
		AppearBoss,
		VSBoss,
		Clear,
		WaitToFade
	};
	struct Scroll
	{
		bool			active			= false;
		float			elapsedSecond	= 0.0f;
		Donya::Vector3	cameraFocusStart;
		Donya::Vector3	cameraFocusDest;
	};
private:
	Donya::ICamera						iCamera;
	Scroll								scroll;
	Donya::ICamera						lightCamera;

	Donya::XInput						controller{ Donya::Gamepad::PAD_1 };
	Player::Input						currentInput;
	Donya::Collision::Box3F				currentScreen;
	int									currentRoomID		= 0;
	Music::ID							currentPlayingBGM	= Music::BGM_Game;
	PlayerInitializer					playerIniter;
	CheckPoint::Container				checkPoint;

	Scene::Type							nextScene			= Scene::Type::Null;
	State								status				= State::FirstInitialize;

	Performer::LoadPart					loadPerformer;
	std::unique_ptr<PauseProcessor>		pPauser;

	std::unique_ptr<Meter::Drawer>		pPlayerMeter;
	std::unique_ptr<Meter::Drawer>		pSkullMeter;
	std::unique_ptr<Effect::Handle>		pFxReady;
	std::unique_ptr<Map>				pMap;
	std::unique_ptr<Sky>				pSky;
	std::unique_ptr<House>				pHouse;
	std::unique_ptr<Door::Container>	pDoors;
	std::unique_ptr<ClearEvent>			pClearEvent;
	std::unique_ptr<Boss::Container>	pBossContainer;
	std::unique_ptr<Player>				pPlayer;

	int				stageNumber					= 0;
	float			stageTimer					= 0.0f;	// Elapsed second from start a stage
	float			elapsedSecondsAfterMiss		= 0.0f;
	float			clearTimer					= 0.0f;	// Elapsed second from clear a stage
	bool			isThereClearEvent			= false;
	bool			isThereBoss					= false;
	bool			wantLeave					= false;// It is valid when the status == State::Clear
	Donya::Vector3	prevPlayerPos;						// It is used to judge the timing that the player arrives to desired position
	Door::Instance	*pThroughingDoor			= nullptr;
	Donya::Vector3	doorPassedPlayerPos;				// Destination of passing a door
	
	Definition::WeaponKind	willUnlockWeapon	= Definition::WeaponKind::Buster; // "::Buster" means a none(that is always available)

	Thread thObjects;
	
#if DEBUG_MODE
	bool	nowDebugMode				= false;
	bool	isReverseCameraMoveX		= true;
	bool	isReverseCameraMoveY		= false;
	bool	isReverseCameraRotX			= false;
	bool	isReverseCameraRotY			= false;
	const Room *pChosenRoom				= nullptr; // It used for ImGui
	Donya::Vector3 previousCameraPos; // In not debugMode
#endif // DEBUG_MODE
public:
	SceneGame() : Scene() {}
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	void	PlayBGM( Music::ID kind );
	void	FadeOutBGM() const;

	Donya::Vector4x4 MakeScreenTransform() const;
	Donya::Collision::Box3F CalcCurrentScreenPlane() const;

	void	InitStage( Music::ID nextBGM, int stageNo, bool reloadMapModel, State transDestinationState = State::Stage );
	void	UninitStage();

	void	AssignCurrentInput();
	
	float	PauseUpdate( float elapsedTime );
	void	BeginPause();
	void	EndPause();

	bool	IsPlayingStatus( State verify ) const;

	void	FirstInitStateUpdate( float elapsedTime );
	
	void	StageStateUpdate( float elapsedTime );

	void	AppearBossStateInit();
	void	AppearBossStateUpdate( float elapsedTime );
	
	void	VSBossStateInit( Definition::WeaponKind bossWeapon );
	void	VSBossStateUpdate( float elapsedTime );

	void	ClearStateInit();
	void	ClearStateUpdate( float elapsedTime );

	void	CameraInit();
	Donya::Vector3 ClampFocusPoint( const Donya::Vector3 &focusPoint, int roomID );
	void	PrepareScrollIfNotActive( int oldRoomID, int newRoomID );
	void	AssignCameraPos();
	void	CameraUpdate( float elapsedTime );

	void	UpdateCurrentRoomID();

	Donya::Vector4x4 CalcLightViewMatrix() const;


	void	ReadyPlayer();
	void	PlayerInit( const PlayerInitializer &initializer, const Map &terrain );
	void	PlayerUpdate( float elapsedTime, const Map &terrain );
	void	PlayerPhysicUpdate( float elapsedTime, const Map &terrain );
	Donya::Vector3 GetPlayerPosition() const;
	Donya::Vector3 MakeBossRoomInitialPosOf( int roomId ) const;
	Player::Input  MakePlayerInput( float elapsedTime );
	
	void	UpdatePlayerIniter();

	void	DoorUpdate();
	bool	NowThroughingDoor() const;

	void	BossUpdate( float elapsedTime, const Donya::Vector3 &wsTargetPos );

	int		CalcCurrentRoomID() const;

	void	Collision_BulletVSBullet();
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
	void	UseImGui( float elapsedTime );
	void	UseScreenSpaceImGui();
#endif // USE_IMGUI
};
