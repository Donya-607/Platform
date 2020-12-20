#pragma once

#include <memory>
#include <vector>

#include "Donya/Camera.h"
#include "Donya/Collision.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/Displayer.h"
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "Enemy.h"
#include "Input.h"
#include "Map.h"
#include "Meter.h"
#include "Player.h"
#include "Scene.h"

class SceneResult : public Scene
{
public:
	enum class State
	{
		Performance,
		Wait
	};
private:
	Donya::ICamera								iCamera;
	Donya::ICamera								lightCamera;
	State										status = State::Performance;

	Donya::XInput								controller{ Donya::Gamepad::PAD_1 };
	Donya::Collision::Box3F						currentScreen;	// It used for a bullet's lifespan
	int											currentRoomID	= 0;
	PlayerInitializer							playerIniter;

	std::unique_ptr<Input::Explainer>			pInputExplainer;
	std::unique_ptr<Map>						pMap;
	std::unique_ptr<Player>						pPlayer;
	std::unique_ptr<Meter::Drawer>				pMeter;

	std::vector<std::unique_ptr<Enemy::Base>>	enemies;

	bool	willSkip		= false;
	float	currentTimer	= 0.0f;
	float	previousTimer	= 0.0f;
	float	extinctTime		= -1.0f;	// A negative value means not extincted
	float	arriveTime		= 0.0f;
	Donya::Vector3 prevPlayerPos;		// It is used to judge the timing that the player arrives to desired position
	Donya::Vector3 centerPos;
#if DEBUG_MODE
	bool	nowDebugMode				= false;
	bool	isReverseCameraMoveX		= true;
	bool	isReverseCameraMoveY		= false;
	bool	isReverseCameraRotX			= false;
	bool	isReverseCameraRotY			= false;
#endif // DEBUG_MODE
public:
	SceneResult() : Scene() {}
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	Donya::Vector3 CalcCenterPoint( const Map &terrain ) const;

	Donya::Vector4x4 MakeScreenTransform() const;
	Donya::Collision::Box3F CalcCurrentScreenPlane() const;

	bool	WantToSkip() const;

	void	CameraInit();
	void	AssignCameraPos();
	void	CameraUpdate();

	void	PlayerInit( const PlayerInitializer &initializer, const Map &terrain );
	void	PlayerUpdate( float elapsedTime, const Map &terrain );
	void	PlayerPhysicUpdate( float elapsedTime, const Map &terrain );
	Donya::Vector3 GetPlayerPosition() const;

	void	RegenerateEnemies( const Donya::Vector3 &targetPos );
	void	EnemyUpdate( float elapsedTime, const Donya::Vector3 &targetPos );
	void	EnemyPhysicUpdate( float elapsedTime, const Map &terrain );
	void	EnemyDraw( RenderingHelper *pRenderer );

	void	Collision_BulletVSBullet();
	void	Collision_BulletVSEnemy();

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
