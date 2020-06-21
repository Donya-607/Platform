#pragma once

#include "Donya/GamepadXInput.h"

#include "Scene.h"

class ScenePause : public Scene
{
private:
	enum class Choice
	{
		Nil			= -1,
		BackToTitle	= 0,
		Resume,
		ReTry,
	};
private:
	Choice			choice = Choice::Nil;
	Donya::XInput	controller{ Donya::XInput::PadNumber::PAD_1 };
public:
	ScenePause() : Scene() {}
public:
	void	Init();
	void	Uninit();

	Result	Update( float elapsedTime );

	void	Draw( float elapsedTime );
private:
	void	UpdateChooseItem();

	Result	ReturnResult();
};
