#pragma once

#include <memory>

#include "Donya/Color.h"
#include "Donya/Constant.h"
#include "Donya/Template.h"

/// <summary>
/// TODO : Change to the user can specify drawing sprite or color.
/// </summary>
class Fader final : public Donya::Singleton<Fader>
{
	friend class Donya::Singleton<Fader>;
public:
	/// <summary>
	/// You can specify the interactionType of fade-out, fade-in.
	/// </summary>
	enum class Type
	{
		Scroll,		// 
		Gradually,	// It will gradually become transparent.
	};
	/// <summary>
	/// If set [DOWN|RIGHT] to parameter, the fade will move to right-bottom from left-top.<para></para>
	/// If set [UP|DOWN] or [LEFT|RIGHT], will UP or LEFT prioritize.
	/// </summary>
	enum Direction
	{
		UP		= 1 << 0,
		DOWN	= 1 << 1,
		LEFT	= 1 << 2,
		RIGHT	= 1 << 3,
	};

	/// <summary>
	/// "interactionType" : You specify interactionType of fade.<para></para>
	/// "closeSecond" : You specify time of completely close(per second).<para></para>
	/// "parameter" :<para></para>
	/// Type::Scroll : Used to judge direction(you can specify by Fader::Direction).<para></para>
	/// Type::Gradually : Used to fill color. This is linking to Donya::Color::Code.
	/// </summary>
	struct Configuration
	{
		Type			type{};			// You specify interactionType of fade.
		float			closeSecond{};	// You specify time of completely close(per second)
		unsigned int	parameter{};	// [Type::Scroll : Used to judge direction(you can specify by Fader::Direction)] [Type::Gradually : Used to fill color. This is linking to Donya::Color::Code]
	public:
		static Configuration UseDefault( Type fadeType );
	public:
		void SetDefault( Type fadeType );

		/// <summary>
		/// Use when the interactionType is Scroll.
		/// </summary>
		void SetDirection( Direction moveDirection );
		/// <summary>
		/// Use when the interactionType is Scroll.
		/// </summary>
		void SetDirection( Direction dirX, Direction dirY );
		/// <summary>
		/// Use when the interactionType is Scroll.
		/// </summary>
		void NormalizeDirection();

		/// <summary>
		/// Use when the interactionType is Gradually.
		/// </summary>
		void SetColor( Donya::Color::Code colorCode );
		/// <summary>
		/// Use when the interactionType is Gradually.<para></para>
		/// These float value are expected to [0.0f ~ 1.0f].
		/// </summary>
		void SetColor( float R, float G, float B );
	};

	/// <summary>
	/// This option used in when if not completed current fade process.
	/// </summary>
	enum class AssignmentOption
	{
		Nothing,	// Doing nothing.
		Overwrite,	// Overwrite by the new fade. the old fade will be dispose immediately.
		Reserve,	// Reserved fade will apply when finished current fade.
	};
public:
	static float GetDefaultCloseSecond();
private:
	struct Impl;
	std::unique_ptr<Impl> pImpl;
private:
	Fader();
public:
	~Fader();
public:
	/// <summary>
	/// Initialize and reset current state.
	/// </summary>
	void Init();
	/// <summary>
	/// Please call every frame.
	/// </summary>
	void Update();
	void Draw();
public:
	/// <summary>
	/// "config" : Please set configuration of fade.<para></para>
	/// "option" : You can choose behavior when if the current fade is not complete.
	/// </summary>
	void StartFadeOut( Configuration config, AssignmentOption option = AssignmentOption::Reserve );
public:
	/// <summary>
	/// Returns true if only when the fade is completely closed.
	/// </summary>
	bool IsClosed() const;
	/// <summary>
	/// Returns true when exist instance even one.
	/// </summary>
	bool IsExist() const;
};
