#include "ScenePause.h"

#include <algorithm>

#include "Donya/Constant.h"
#include "Donya/Keyboard.h"
#include "Donya/Sprite.h"
#include "Donya/Sound.h"
#include "Donya/Vector.h"

#include "Common.h"
#include "Effect/EffectAdmin.h"
#include "FilePath.h"
#include "Music.h"
#include "Input.h"

#undef max
#undef min

void ScenePause::Init()
{
	Effect::Admin::Get().SetUpdateSpeed( 0.0f );
}

void ScenePause::Uninit()
{
	Effect::Admin::Get().SetUpdateSpeed( 1.0f );
}

Scene::Result ScenePause::Update( float elapsedTime )
{
	UpdateInput();

	UpdateChooseItem();

	return ReturnResult();
}

void ScenePause::Draw( float elapsedTime )
{
	const float oldDepth = Donya::Sprite::GetDrawDepth();
	Donya::Sprite::SetDrawDepth( 0.0f );
	Donya::Sprite::DrawRect
	(
		Common::HalfScreenWidthF(),	Common::HalfScreenWidthF(),
		Common::ScreenWidthF(),		Common::ScreenWidthF(),
		Donya::Color::Code::BLACK,
		0.5f
	);
	Donya::Sprite::SetDrawDepth( oldDepth );
}

void ScenePause::UpdateInput()
{
	static const Donya::Vector2 deadZone
	{
		Donya::XInput::GetDeadZoneLeftStick(),
		Donya::XInput::GetDeadZoneLeftStick()
	};

	controller.Update();
	previousInput = currentInput;
	currentInput  = Input::MakeCurrentInput( controller, deadZone );
}

void ScenePause::UpdateChooseItem()
{
	auto Tilted = []( float value, int sign )
	{
		return Donya::SignBit( value ) == sign;
	};

	const auto &prev		= previousInput;
	const auto &curr		= currentInput;
	const bool back			= Tilted( curr.moveVelocity.y, -1 ) && !Tilted( prev.moveVelocity.y, -1 );
	const bool advance		= Tilted( curr.moveVelocity.y, +1 ) && !Tilted( prev.moveVelocity.y, +1 );
	const bool trgDecide	= Input::HasTrue( curr.useShots ) && !Input::HasTrue( prev.useShots );

	int index = scast<int>( choice );
	int oldIndex = index;

	if ( back		) { index--; }
	if ( advance	) { index++; }

	index = std::max( 0, std::min( scast<int>( Choice::ItemCount ) - 1, index ) );

	if ( index != oldIndex )
	{
	#if DEBUG_MODE
		Donya::Sound::Play( Music::DEBUG_Weak );
	#endif // DEBUG_MODE
	}

	choice = scast<Choice>( index );
}

Scene::Result ScenePause::ReturnResult()
{
	if ( controller.Trigger( Donya::Gamepad::Button::START ) || Donya::Keyboard::Trigger( 'P' ) )
	{
	#if DEBUG_MODE
		Donya::Sound::Play( Music::DEBUG_Weak );
	#endif // DEBUG_MODE

		Scene::Result change{};
		change.AddRequest( Scene::Request::REMOVE_ME );
		return change;
	}

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}
