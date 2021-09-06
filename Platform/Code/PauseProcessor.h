#pragma once

#include "Donya/UseImGui.h"

#include "Player.h"
#include "Music.h"
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
public:
	enum class Choice
	{
		Nil			= -1,
		Resume,
		BackToTitle,

		ItemCount,
	};
	static constexpr const char *GetItemName( Choice attr )
	{
		switch ( attr )
		{
		case Choice::Nil:			return "Nil";
		case Choice::Resume:		return "Resume";
		case Choice::BackToTitle:	return "BackToTitle";
		default: break;
		}

		return "ERROR";
	}
public:
	static void LoadParameter();
#if USE_IMGUI
	static void UpdateParameter();
#endif // USE_IMGUI
private:
	Music::ID		nowPlayingBGM;
	Choice			choice = Choice::Nil;
	Player::Input	previousInput;
	Player::Input	currentInput;
	bool			decided = false;
public:
	void	Init( const Music::ID &currentPlayingBGM );
	void	Uninit();

	Result	Update( float deltaTime );

	void	Draw();
private:
	void	SetVolume( float volume );
	void	UpdateInput();
	void	UpdateChooseItem();
	Result	ReturnResult();
};
