#include "PauseProcessor.h"

#include "Donya/Sound.h"
#include "Donya/Sprite.h"

#include "Effect/EffectAdmin.h"

#include "Common.h"
#include "Input.h"
#include "Music.h"

void PauseProcessor::Init()
{
	Effect::Admin::Get().SetUpdateSpeed( 0.0f );
}
void PauseProcessor::Uninit()
{
	Effect::Admin::Get().SetUpdateSpeed( 1.0f );
}

PauseProcessor::Result PauseProcessor::Update( float elapsedTime, const Donya::XInput &controller )
{
	UpdateInput( controller );
	UpdateChooseItem();

	return ReturnResult( controller );
}

void PauseProcessor::Draw( float elapsedTime )
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

void PauseProcessor::UpdateInput( const Donya::XInput &controller )
{
	static const Donya::Vector2 deadZone
	{
		Donya::XInput::GetDeadZoneLeftStick(),
		Donya::XInput::GetDeadZoneLeftStick()
	};

	previousInput = currentInput;
	currentInput  = Input::MakeCurrentInput( controller, deadZone );
}
void PauseProcessor::UpdateChooseItem()
{
	auto Tilted = []( float value, int sign )
	{
		return Donya::SignBit( value ) == sign;
	};

	const auto &prev	= previousInput;
	const auto &curr	= currentInput;
	const bool back		= Tilted( curr.moveVelocity.y, -1 ) && !Tilted( prev.moveVelocity.y, -1 );
	const bool advance	= Tilted( curr.moveVelocity.y, +1 ) && !Tilted( prev.moveVelocity.y, +1 );
	decided				= Input::HasTrue( curr.useShots ) && !Input::HasTrue( prev.useShots );

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
PauseProcessor::Result PauseProcessor::ReturnResult( const Donya::XInput &controller )
{
	Result rv;

	if ( Input::IsPauseRequested( controller ) )
	{
		rv.command = Result::Command::Resume;
	}
	else
	if ( decided )
	{
		switch ( choice )
		{
		case Choice::BackToTitle:
			rv.command		= Result::Command::ChangeScene;
			rv.nextScene	= Scene::Type::Title;
			break;
		case Choice::Resume:
			rv.command		= Result::Command::Resume;
			break;
		default:
			// No op
			break;
		}
	}

	return rv;
}
