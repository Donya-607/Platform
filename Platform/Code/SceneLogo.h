#pragma once

#include <array>

#include "Donya/GamepadXInput.h"

#include "FilePath.h"
#include "Scene.h"

/// <summary>
/// This is only show the rights logo.
/// </summary>
class SceneLogo : public Scene
{
private:
	enum class State
	{
		FADE_IN,
		WAIT,
		FADE_OUT,
		END
	};
private:
	static constexpr std::array<SpriteAttribute, 1> showLogos
	{
		SpriteAttribute::FMODLogoBlack
	};
private:
	std::array<size_t, showLogos.size()> sprites{ 0 };
	Donya::XInput	controller{ Donya::Gamepad::PAD_1 }; // For skip
	State			status		= State::FADE_IN;
	int				showIndex	= 0;	// 0-based.
	float			alpha		= 0.0f;	// 0.0f ~ 1.0f.
	float			scale		= 1.0f;	// Use for magnification.
private:
	int				frameTimer	= 0;	// For frame base.
	float			secondTimer	= 0;	// For real-time base.
public:
	SceneLogo() : Scene() {}
	~SceneLogo() = default;
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	bool	WannaSkip() const;
	bool	HasRemainLogo() const;
	void	AdvanceLogoIndexOrEnd();
private:
	void	InitFadeIn();
	void	UpdateFadeIn( float elapsedTime );
	void	InitWait();
	void	UpdateWait( float elapsedTime );
	void	InitFadeOut();
	void	UpdateFadeOut( float elapsedTime );
	void	InitEnd();
private:
	void	ClearBackGround() const;
	void	StartFade() const;
	Result	ReturnResult();
};