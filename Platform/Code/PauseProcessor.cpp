#include "PauseProcessor.h"

#include "Donya/Sound.h"
#include "Donya/Sprite.h"

#include "Effect/EffectAdmin.h"

#include "Common.h"
#include "FontHelper.h"
#include "Input.h"
#include "PauseParam.h"

namespace
{
	static ParamOperator<PauseParam> parameter{ "Pause" };
	PauseParam FetchParameter()
	{
		return parameter.Get();
	}
}
#if USE_IMGUI
void PauseParam::ShowImGuiNode()
{
	if ( ImGui::TreeNode( u8"ëËñºï\é¶" ) )
	{
		ImGui::DragFloat ( u8"ÉXÉPÅ[Éã", &titleScale, 0.01f );
		ImGui::DragFloat2( u8"íÜêSç¿ïW", &titlePos.x, 1.0f  );

		ImGui::TreePop();
	}

	constexpr size_t itemCount = scast<size_t>( PauseProcessor::Choice::ItemCount );
	if ( itemPositions.size() != itemCount )
	{
		constexpr Donya::Vector2 center{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };
		itemPositions.resize( itemCount, center );
	}
	if ( ImGui::TreeNode( u8"çÄñ⁄ä÷òA" ) )
	{
		ImGui::DragFloat( u8"ëIëï®ÇÃÉXÉPÅ[Éã", &chooseItemScale, 0.01f );
		ImGui::DragFloat( u8"ëºÇÃï®ÇÃÉXÉPÅ[Éã", &otherItemScale,  0.01f );

		if ( ImGui::TreeNode( u8"íÜêSç¿ïW" ) )
		{
			std::string caption{};
			for ( size_t i = 0; i < itemCount; ++i )
			{
				caption = PauseProcessor::GetItemName( scast<PauseProcessor::Choice>( i ) );
				ImGui::DragFloat2( caption.c_str(), &itemPositions[i].x );
			}
					
			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	ImGui::SliderFloat	( u8"É|Å[ÉYíÜÇÃÇaÇfÇlâπó ", &soundVolume,		0.0f, 1.0f );
	ImGui::DragFloat	( u8"âπó ïœçXÇ…Ç©ÇØÇÈïbêî", &fadeSoundSecond, 0.01f );
}
#endif // USE_IMGUI

void PauseProcessor::LoadParameter()
{
	parameter.LoadParameter();
}
#if USE_IMGUI
void PauseProcessor::UpdateParameter()
{
	parameter.ShowImGuiNode( u8"É|Å[ÉYíÜÇÃÉpÉâÉÅÅ[É^" );
}
#endif // USE_IMGUI

void PauseProcessor::Init( const Music::ID &currentPlayingBGM )
{
	nowPlayingBGM = currentPlayingBGM;
	choice = Choice::Nil;

	SetVolume( FetchParameter().soundVolume );
	Effect::Admin::Get().SetUpdateSpeed( 0.0f );
}
void PauseProcessor::Uninit()
{
	SetVolume( 1.0f );
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
	const auto &data = FetchParameter();

	const float oldDepth = Donya::Sprite::GetDrawDepth();
	Donya::Sprite::SetDrawDepth( 0.0f );

	Donya::Sprite::DrawRect
	(
		Common::HalfScreenWidthF(),	Common::HalfScreenWidthF(),
		Common::ScreenWidthF(),		Common::ScreenWidthF(),
		Donya::Color::Code::BLACK,
		0.5f
	);

	const auto pFontRenderer = FontHelper::GetRendererOrNullptr( FontAttribute::Main );
	if ( pFontRenderer )
	{
		constexpr Donya::Vector2 pivot			{ 0.5f, 0.5f };
		constexpr Donya::Vector4 white			{ 1.0f, 1.0f, 1.0f, 1.0f };
		constexpr Donya::Vector4 selectColor	= white;
		constexpr Donya::Vector4 unselectColor	{ 0.4f, 0.4f, 0.4f, 1.0f };

		const Donya::Vector2 selectScale   = data.chooseItemScale;
		const Donya::Vector2 unselectScale = data.otherItemScale;

		const int positionCount = scast<int>( data.itemPositions.size() );
		auto DrawImpl	= [&]( const wchar_t *itemName, Choice item, const Donya::Vector2 &scale, const Donya::Vector4 &color )
		{
			const int itemIndex = scast<int>( item );
			const Donya::Vector2 pos =
				( positionCount <= itemIndex )
				? Donya::Vector2::Zero()
				: data.itemPositions[itemIndex];

			pFontRenderer->DrawExt( itemName, pos, pivot, scale, color );
		};
		auto Draw		= [&]( const wchar_t *itemName, Choice item, bool selected )
		{
			const Donya::Vector2 &scale =
				( selected )
				? selectScale
				: unselectScale;

			const Donya::Vector4 &color =
				( selected )
				? selectColor
				: unselectColor;

			DrawImpl( itemName, item, scale, color );
		};

		Draw( L"ÇqÇdÇrÇtÇlÇd",	Choice::Resume,			( choice == Choice::Resume		) );
		Draw( L"ÇdÇwÇhÇs",		Choice::BackToTitle,	( choice == Choice::BackToTitle	) );

		pFontRenderer->DrawExt( L"-ÇoÇ`ÇtÇrÇd-", data.titlePos, pivot, data.titleScale, white );

		Donya::Sprite::Flush();
	}

	Donya::Sprite::SetDrawDepth( oldDepth );
}

void PauseProcessor::SetVolume( float volume )
{
	const auto &data = FetchParameter();
	Donya::Sound::AppendFadePoint( nowPlayingBGM, data.fadeSoundSecond, volume, /* isEnableForAll = */ true );
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
	const bool up		= Tilted( curr.moveVelocity.y, +1 ) && !Tilted( prev.moveVelocity.y, +1 );
	const bool down		= Tilted( curr.moveVelocity.y, -1 ) && !Tilted( prev.moveVelocity.y, -1 );
	decided				= Input::HasTrue( curr.useShots ) && !Input::HasTrue( prev.useShots );

	constexpr int lastItem = scast<int>( Choice::ItemCount ) - 1;

	if ( choice == Choice::Nil )
	{
		const bool right	= Tilted( curr.moveVelocity.x, +1 ) && !Tilted( prev.moveVelocity.x, +1 );
		const bool left		= Tilted( curr.moveVelocity.x, -1 ) && !Tilted( prev.moveVelocity.x, -1 );

		if ( left || right )
		{
			choice = scast<Choice>( 0 );
		}
		else
		if ( decided )
		{
			choice  = scast<Choice>( 0 );
			decided = false;
		}
		else
		if ( up )
		{
			choice = scast<Choice>( 0 );
		}
		else
		if ( down )
		{
			choice = scast<Choice>( lastItem );
		}

		return;
	}
	// else

	int index = scast<int>( choice );
	int oldIndex = index;

	if ( up		) { index--; }
	if ( down	) { index++; }
	index = std::max( 0, std::min( lastItem, index ) );

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
		case Choice::Resume:
			rv.command		= Result::Command::Resume;
			break;
		case Choice::BackToTitle:
			rv.command		= Result::Command::ChangeScene;
			rv.nextScene	= Scene::Type::Title;
			break;
		default:
			// No op
			break;
		}
	}

	return rv;
}
