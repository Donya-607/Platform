#pragma once

#include <memory>
#include <vector>

#include "Donya/Constant.h"			// Use DEBUG_MODE macro.
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"			// Use USE_IMGUI macro.

#include <string>
#include "Donya/Camera.h"
#include "Donya/Collision.h"
#include "Donya/Serializer.h"

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
	Donya::Collision::Body body;
public:
	SceneOver() : Scene() {}
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( colA ),
			CEREAL_NVP( colB )
		);
		if ( 1 <= version )
		{
			archive( CEREAL_NVP( body ) );
		}
		if ( 2 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
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
	void	Save();
	void	LoadBin();
	void	LoadJson();
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

CEREAL_CLASS_VERSION( SceneOver, 1 )

