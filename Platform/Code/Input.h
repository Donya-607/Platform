#pragma once

#include <array>

#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Player.h"
#include "UI.h"

#if USE_IMGUI
#include <string>
#endif // USE_IMGUI

namespace Input
{
	template<typename T>
	constexpr bool HasTrue( const std::array<T, Player::Input::variationCount> &arr )
	{
		for ( const auto &it : arr )
		{
			if ( it ) { return true; }
		}
		return false;
	}
	bool HasButtonInput( const Player::Input &input );
	bool HasStickInput( const Player::Input &input );
	
	Player::Input MakeCurrentInput( const Donya::XInput &controller, const Donya::Vector2 &stickDeadZone );

	bool IsPauseRequested( const Donya::XInput &controller );
}
namespace Input
{
	enum class Type
	{
		ShiftWeapon_Back,
		ShiftWeapon_Advance,

		TypeCount
	};
	constexpr const char *GetTypeName( Type type )
	{
		switch ( type )
		{
		case Type::ShiftWeapon_Back:	return "ShiftWeapon_Back";
		case Type::ShiftWeapon_Advance:	return "ShiftWeapon_Advance";
		default: break;
		}

		return "ERROR";
	}

	void LoadParameter();
#if USE_IMGUI
	void UpdateParameter( const std::string &nodeCaption );
#endif // USE_IMGUI

	class Explainer
	{
	private:
		size_t  spriteId = NULL;
		mutable UIObject sheet;
	private:
		float			timer = 0.0f;
		Donya::Vector2	stretch{ 1.0f, 1.0f };
		bool			performing = false;
	public:
		bool Init();
		void Update( float elapsedTime );
		void Draw( Type type, bool showControllerType, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssScale = { 1.0f, 1.0f }, float drawDepth = 0.0f ) const;
	public:
		void Notify();
	};
}
