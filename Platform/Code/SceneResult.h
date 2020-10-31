#pragma once

#include <memory>
#include <vector>

#include "Donya/Camera.h"
#include "Donya/Collision.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/Displayer.h"
#include "Donya/GamepadXInput.h"
#include "Donya/Shader.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "Bloom.h"
#include "Enemy.h"
#include "Map.h"
#include "Player.h"
#include "Renderer.h"
#include "Scene.h"

class SceneResult : public Scene
{
public:
	struct Shader
	{
		Donya::VertexShader VS;
		Donya::PixelShader  PS;
	};
private:
	Donya::ICamera								iCamera;
	Donya::ICamera								lightCamera;

	Donya::XInput								controller{ Donya::Gamepad::PAD_1 };
	Player::Input								currentInput;
	Donya::Collision::Box3F						currentScreen;	// It used for a bullet's lifespan
	int											currentRoomID	= 0;
	PlayerInitializer							playerIniter;
	Player										plaeyr;

	std::unique_ptr<RenderingHelper>			pRenderer;
	std::unique_ptr<Donya::Displayer>			pDisplayer;
	std::unique_ptr<BloomApplier>				pBloomer;
	std::unique_ptr<Donya::Surface>				pScreenSurface;
	std::unique_ptr<Donya::Surface>				pShadowMap;
	std::unique_ptr<Shader>						pQuadShader;
	std::unique_ptr<Map>						pMap;
	std::unique_ptr<Player>						pPlayer;

	std::vector<std::unique_ptr<Enemy::Base>>	enemies;

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
	bool	CreateRenderers( const Donya::Int2 &wholeScreenSize );
	bool	CreateSurfaces( const Donya::Int2 &wholeScreenSize );
	bool	CreateShaders();
	bool	AreRenderersReady() const;

	Donya::Vector4x4 MakeScreenTransform() const;
	Donya::Collision::Box3F CalcCurrentScreenPlane() const;

	void	CameraInit();
	void	AssignCameraPos();
	void	CameraUpdate();

	void	PlayerInit( const PlayerInitializer &initializer, const Map &terrain );
	void	PlayerUpdate( float elapsedTime, const Map &terrain );
	void	PlayerPhysicUpdate( float elapsedTime, const Map &terrain );
	Donya::Vector3 GetPlayerPosition() const;

	void	EnemyInit( const Donya::Vector3 &targetPos );
	void	EnemyUpdate( float elapsedTime, const Donya::Vector3 &targetPos );
	void	EnemyPhysicUpdate( float elapsedTime, const Map &terrain );
	void	EnemyDraw( RenderingHelper *pRenderer );

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
