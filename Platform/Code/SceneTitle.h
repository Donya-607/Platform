#pragma once

#include <memory>

#include "Donya/Camera.h"
#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/Font.h"
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
#include "UI.h"

class SceneTitle : public Scene
{
public:
	enum class Choice
	{
		Start = 0,
		Option,
		
		ItemCount, // Do not select anything
	};
private:
	struct Scroll
	{
		bool			active			= false;
		float			elapsedSecond	= 0.0f;
		Donya::Vector3	cameraFocusStart;
		Donya::Vector3	cameraFocusDest;
	};
	struct Shader
	{
		Donya::VertexShader VS;
		Donya::PixelShader  PS;
	};
private:
	Donya::ICamera							iCamera;
	Scroll									scroll;
	Donya::ICamera							lightCamera;

	Donya::XInput							controller{ Donya::Gamepad::PAD_1 };
	Donya::Collision::Box3F					currentScreen;
	int										currentRoomID	= 0;
	
	Scene::Type								nextScene		= Scene::Type::Null;
	
	std::unique_ptr<RenderingHelper>		pRenderer;
	std::unique_ptr<Donya::Font::Renderer>	pFontRenderer;
	std::unique_ptr<Donya::Displayer>		pDisplayer;
	std::unique_ptr<BloomApplier>			pBloomer;
	std::unique_ptr<Donya::Surface>			pScreenSurface;
	std::unique_ptr<Donya::Surface>			pShadowMap;
	std::unique_ptr<Shader>					pQuadShader;
	std::unique_ptr<Map>					pMap;
	std::unique_ptr<House>					pHouse;
	std::unique_ptr<Player>					pPlayer;
	std::unique_ptr<PlayerInitializer>		pPlayerIniter;

	float		elapsedSecond	= 0.0f;
	Choice		chooseItem		= Choice::ItemCount;
	bool		wasDecided		= false;

	UIObject	sprTitleLogo;

#if DEBUG_MODE
	bool nowDebugMode			= false;
	bool isReverseCameraMoveX	= false;
	bool isReverseCameraMoveY	= true;
	bool isReverseCameraRotX	= false;
	bool isReverseCameraRotY	= false;
	Donya::Vector3 previousCameraPos; // In not debugMode
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

	void	UpdateChooseItem();

	Donya::Vector4x4 MakeScreenTransform() const;
	Donya::Collision::Box3F CalcCurrentScreenPlane() const;

	void	CameraInit();
	Donya::Vector3 ClampFocusPoint( const Donya::Vector3 &focusPoint, int roomID );
	void	PrepareScrollIfNotActive( int oldRoomID, int newRoomID );
	void	AssignCameraPos();
	void	CameraUpdate( float elapsedTime );

	Donya::Vector4x4 CalcLightViewProjectionMatrix() const;

	void	PlayerInit();
	void	PlayerUpdate( float elapsedTime, const Map &terrain );

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
