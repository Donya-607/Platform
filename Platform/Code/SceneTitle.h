#pragma once

#include <array>
#include <memory>

#include "Donya/Camera.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include "Boss.h"
#include "Bloom.h"
#include "ClearEvent.h"
#include "Map.h"
#include "ObjectBase.h"
#include "Player.h"
#include "Renderer.h"
#include "Room.h"
#include "Scene.h"
#include "Sky.h"
#include "UI.h"

class SceneTitle : public Scene
{
public:
	enum class CameraState
	{
		Attract,
		Controllable,
		StartPerformance,

		StateCount
	};
	static constexpr int cameraStateCount = scast<int>( CameraState::StateCount );

	enum class PerformanceState
	{
		NotPerforming,
		ToLeave,
		ToFade,
		Wait,

		StateCount
	};
	static constexpr int performanceStateCount = scast<int>( PerformanceState::StateCount );

	enum class Choice
	{
		Start	= 0,	// It uses when a save data is not exist
		NewGame	= 0,
		LoadGame,
		
		ItemCount,		// Do not select anything
	};
private:
	struct Shader
	{
		Donya::VertexShader VS;
		Donya::PixelShader  PS;
	};
private:
	std::array<Donya::ICamera, cameraStateCount>	stateCameras;
	Donya::ICamera									lightCamera;

	Donya::XInput									controller{ Donya::Gamepad::PAD_1 };
	Player::Input									previousInput; // Use for detect a trigger/release timing
	Player::Input									currentInput;
	Donya::Collision::Box3F							currentScreen;
	int												currentRoomID		= 0;
	PlayerInitializer								playerIniter;

	Scene::Type										nextScene			= Scene::Type::Null;
	CameraState										currCameraStatus	= CameraState::Attract;
	CameraState										beforeCameraStatus	= CameraState::Attract;
	float											transCameraTime		= 1.0f; // 0.0f ~ 1.0f, Lerp( beforeCameraStatus -> currCameraStatus, time )

	PerformanceState								performanceStatus	= PerformanceState::NotPerforming;

	std::unique_ptr<RenderingHelper>				pRenderer;
	std::unique_ptr<Donya::Displayer>				pDisplayer;
	std::unique_ptr<BloomApplier>					pBloomer;
	std::unique_ptr<Donya::Surface>					pScreenSurface;
	std::unique_ptr<Donya::Surface>					pShadowMap;
	std::unique_ptr<Shader>							pQuadShader;
	std::unique_ptr<Map>							pMap;
	std::unique_ptr<Sky>							pSky;
	std::unique_ptr<House>							pHouse;
	std::unique_ptr<Player>							pPlayer;
	std::unique_ptr<Boss::Base>						pBoss;

	float		elapsedSecond		= 0.0f;
	float		elapsedSecondSinceLastInput = 0.0f; // It is valid when the performanceStatus == PerformanceState::NotPerforming
	int			horizDiffSignFromInitialPos = 0;
	float		performTimer		= 0.0f;
	float		afterDecidedTimer	= 0.0f;
	Choice		chooseItem			= Choice::Start;
	bool		wasDecided			= false;
	bool		saveDataIsExist		= false;
	bool		returnToAttract		= false; // It is valid when the performanceStatus == PerformanceState::NotPerforming

	UIObject	sprTitleLogo;

#if DEBUG_MODE
	bool		nowDebugMode			= false;
	bool		isReverseCameraMoveX	= true;
	bool		isReverseCameraMoveY	= false;
	bool		isReverseCameraRotX		= false;
	bool		isReverseCameraRotY		= false;
#endif // DEBUG_MODE
public:
	SceneTitle() : Scene() {}
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

	void	LoadSaveData();

	void	UpdateInput();
	void	UpdateChooseItem();
	void	UpdatePerformance( float elapsedTime );

	void	ChangeCameraState( CameraState next );

	Donya::Vector4x4 MakeScreenTransform() const;
	Donya::Collision::Box3F CalcCurrentScreenPlane() const;

	void	CameraInit();
	void	AssignCameraPos();
	void	CameraUpdate( float elapsedTime );
	const Donya::ICamera &GetCurrentCamera( CameraState key ) const;

	Donya::Vector4x4 CalcLightViewMatrix() const;

	void	PlayerInit( const Map &terrain );
	void	PlayerUpdate( float elapsedTime, const Map &terrain );
	Player::Input  MakePlayerInput( float elapsedTime );
	Donya::Vector3 GetPlayerPosition() const;

	void	BossUpdate( float elapsedTime, const Donya::Vector3 &targetPos );

	int		CalcCurrentRoomID() const;

	void	ClearBackGround() const;
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#endif // USE_IMGUI
};
