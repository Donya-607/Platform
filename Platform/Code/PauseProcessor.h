#pragma once

#include "Donya/GamepadXInput.h"

#include "Player.h"
#include "Scene.h"	// Use Scene::Result

class PauseProcessor
{
public:
	struct Result
	{
		enum class Command
		{
			Noop,
			Resume,
			ChangeScene
		};
	public:
		Command		command		= Command::Noop;
		Scene::Type	nextScene	= Scene::Type::Null;
	};
private:
	enum class Choice
	{
		Nil			= -1,
		BackToTitle	= 0,
		Resume,

		ItemCount,
	};
private:
	Choice			choice = Choice::Nil;
	Player::Input	previousInput;
	Player::Input	currentInput;
	bool			decided = false;
public:
	void	Init();
	void	Uninit();

	Result	Update( float elapsedTime, const Donya::XInput &controller );

	void	Draw( float elapsedTime );
private:
	void	UpdateInput( const Donya::XInput &controller );
	void	UpdateChooseItem();
	Result	ReturnResult( const Donya::XInput &controller );
};
