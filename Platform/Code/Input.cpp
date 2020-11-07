#include "Input.h"

#include "Donya/Keyboard.h"
#include "Donya/Sprite.h"

#include "InputParam.h"
#include "Parameter.h"

namespace Input
{
	bool HasButtonInput( const Player::Input &input )
	{
		return
			HasTrue( input.useJumps  )
		||	HasTrue( input.useShots  )
		||	HasTrue( input.useDashes )
		||	HasTrue( input.shiftGuns )
		;
	}
	bool HasStickInput( const Player::Input &input )
	{
		return !input.moveVelocity.IsZero();
	}
	
	Player::Input MakeCurrentInput( const Donya::XInput &controller, const Donya::Vector2 &deadZone )
	{
		Player::Input input{};

		bool pressLeft	= false;
		bool pressRight	= false;
		bool pressUp	= false;
		bool pressDown	= false;

		// TODO: To be changeable the input key or button

		if ( controller.IsConnected() )
		{
			using Button	= Donya::Gamepad::Button;
			using Direction	= Donya::Gamepad::StickDirection;

			const auto stick = controller.LeftStick();
		
			pressLeft	= controller.Press( Button::LEFT	) || ( stick.x <= -deadZone.x );
			pressRight	= controller.Press( Button::RIGHT	) || ( stick.x >= +deadZone.x );
			pressUp		= controller.Press( Button::UP		) || ( stick.y >= +deadZone.y );
			pressDown	= controller.Press( Button::DOWN	) || ( stick.y <= -deadZone.y );
		
			input.useJumps[0]	= controller.Press( Button::A	);
			input.useShots[0]	= controller.Press( Button::X	);
			input.useDashes[0]	= controller.Press( Button::LT	);
			input.shiftGuns[0]	= controller.Press( Button::PRESS_R	);
			if ( 2 <= Player::Input::variationCount )
			{
			input.useJumps[1]	= controller.Press( Button::B	);
			input.useShots[1]	= controller.Press( Button::Y	);
			input.useDashes[1]	= controller.Press( Button::RT	);
			}
		}
		else
		{
			pressLeft	= Donya::Keyboard::Press( VK_LEFT	);
			pressRight	= Donya::Keyboard::Press( VK_RIGHT	);
			pressUp		= Donya::Keyboard::Press( VK_UP		);
			pressDown	= Donya::Keyboard::Press( VK_DOWN	);

			input.useJumps[0]	= Donya::Keyboard::Press( 'Z'	);
			input.useShots[0]	= Donya::Keyboard::Press( 'X'	);
			input.useDashes[0]	= Donya::Keyboard::Press( 'A'	);
			input.shiftGuns[0]	= Donya::Keyboard::Press( 'C'	);
			if ( 2 <= Player::Input::variationCount )
			{
			input.useJumps[1]	= Donya::Keyboard::Press( VK_RSHIFT	);
			input.useShots[1]	= Donya::Keyboard::Press( 'S'	);
			}
		}

		if ( pressLeft	) { input.moveVelocity.x -= 1.0f; }
		if ( pressRight	) { input.moveVelocity.x += 1.0f; }
		if ( pressUp	) { input.moveVelocity.y += 1.0f; } // World space direction
		if ( pressDown	) { input.moveVelocity.y -= 1.0f; } // World space direction

		return input;
	}

	bool IsPauseRequested( const Donya::XInput &controller )
	{
		const bool button	= controller.Trigger( Donya::Gamepad::Button::START );
		const bool key		= Donya::Keyboard::Trigger( 'P' );
		return button || key;
	}
}
namespace Input
{
	static ParamOperator<Parameter> parameter{ "Input" };
	void LoadParameter()
	{
		parameter.LoadParameter();
	}
	const Parameter &FetchParameter()
	{
		return parameter.Get();
	}
#if USE_IMGUI
	void UpdateParameter( const std::string &nodeCaption )
	{
		parameter.ShowImGuiNode( nodeCaption );
	}
#endif // USE_IMGUI


	constexpr size_t typeCount = scast<size_t>( Type::TypeCount );

#if USE_IMGUI
	void Parameter::ShowImGuiNode()
	{
		if ( texViews.size() != typeCount )
		{
			texViews.resize( typeCount );
		}
		if ( ImGui::TreeNode( u8"テクスチャ情報設定" ) )
		{
			std::string caption;
			for ( size_t i = 0; i < typeCount; ++i )
			{
				caption = GetTypeName( scast<Type>( i ) );

				auto &v = texViews[i];
				v.keyboard.ShowImGuiNode	( caption + u8"・キーボード"		);
				v.controller.ShowImGuiNode	( caption + u8"・コントローラ"	);
			}

			ImGui::TreePop();
		}
	}
#endif // USE_IMGUI


	bool Explainer::Init()
	{
		constexpr SpriteAttribute attr = SpriteAttribute::InputButtons;
		spriteId = Donya::Sprite::Load( GetSpritePath( attr ), GetSpriteInstanceCount( attr ) );
		sheet.AssignSpriteID( spriteId );

		return ( spriteId == NULL ) ? false : true;
	}
	void Explainer::Draw( Type type, bool showController, const Donya::Vector2 &ssPos, const Donya::Vector2 &scale, float drawDepth ) const
	{
		const auto &data = FetchParameter();
		if ( data.texViews.size() != typeCount ) { return; }
		// else

		const auto &view = data.texViews[scast<size_t>( type )];

		sheet = ( showController ) ? view.controller : view.keyboard;
		sheet.AssignSpriteID( spriteId );
		sheet.pos	= ssPos;
		sheet.scale	= Donya::Vector2::Product( sheet.scale, scale );

		sheet.DrawPart( drawDepth );
	}
}
