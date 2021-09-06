#pragma once

#include <array>

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
	static constexpr std::array<SpriteAttribute, 2> showLogos
	{
		SpriteAttribute::FMODLogoBlack,
		SpriteAttribute::EffekseerLogo
	};
private:
	std::array<size_t, showLogos.size()> sprites{ 0 };
	State	status		= State::FADE_IN;
	int		showIndex	= 0;	// 0-based.
	float	alpha		= 0.0f;	// 0.0f ~ 1.0f.
	float	scale		= 1.0f;	// Use for magnification.
private:
	int		frameTimer	= 0;	// For frame base.
	float	secondTimer	= 0;	// For real-time base.
public:
	SceneLogo() : Scene() {}
	~SceneLogo() = default;
public:
	void	Init() override;
	void	Uninit() override;
	Result	Update() override;
	void	Draw() override;
private:
	bool	WannaSkip() const;
	bool	HasRemainLogo() const;
	void	AdvanceLogoIndexOrEnd();
private:
	void	InitFadeIn();
	void	UpdateFadeIn( float deltaTime );
	void	InitWait();
	void	UpdateWait( float deltaTime );
	void	InitFadeOut();
	void	UpdateFadeOut( float deltaTime );
	void	InitEnd();
private:
	void	ClearBackGround() const;
	void	StartFade() const;
	Result	ReturnResult();
};