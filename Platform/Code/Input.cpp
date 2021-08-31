#include "Input.h"

#include "Donya/Keyboard.h"
#include "Donya/Sprite.h"

#include "InputParam.h"
#include "Math.h"
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
			input.useDashes[0]	= controller.Press( Button::RT	);
			input.shiftGuns[0]	= controller.Press( Button::RB	) ? +1 : 0;
			if ( 2 <= Player::Input::variationCount )
			{
			input.useJumps[1]	= controller.Press( Button::B	);
			input.useShots[1]	= controller.Press( Button::Y	);
			input.useDashes[1]	= controller.Press( Button::LT	);
			input.shiftGuns[1]	= controller.Press( Button::LB	) ? -1 : 0;
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
			input.shiftGuns[0]	= Donya::Keyboard::Press( 'E'	) ? +1 : 0;
			if ( 2 <= Player::Input::variationCount )
			{
			input.useJumps[1]	= Donya::Keyboard::Press( VK_RSHIFT	);
			input.useShots[1]	= Donya::Keyboard::Press( 'S'	);
			input.shiftGuns[1]	= Donya::Keyboard::Press( 'W'	) ? -1 : 0;
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

		constexpr Donya::Vector2 scaleRange{ -5.0f, 5.0f };
		ImGui::Helper::ShowBezier2DNode( u8"アピール時の伸縮設定", &notifyingStretches, scaleRange.x, scaleRange.y );
		ImGui::DragFloat( u8"アピールにかける秒数", &notifyingSecond, 0.01f );
		notifyingSecond = std::max( 0.01f, notifyingSecond );
	}
#endif // USE_IMGUI


	bool Explainer::Init()
	{
		constexpr SpriteAttribute attr = SpriteAttribute::InputButtons;
		spriteId = Donya::Sprite::Load( GetSpritePath( attr ), GetSpriteInstanceCount( attr ) );
		sheet.AssignSpriteID( spriteId );

		return ( spriteId == NULL ) ? false : true;
	}
	void Explainer::Update( float deltaTime )
	{
		if ( !performing ) { return; }
		// else

		const auto &data = FetchParameter();

		timer += deltaTime;

		const auto &stretches = data.notifyingStretches;
		if ( stretches.size() < 2 )
		{
			stretch = { 1.0f, 1.0f };
			return;
		}
		// else
		if ( data.notifyingSecond <= timer || IsZero( data.notifyingSecond ) )
		{
			timer		= data.notifyingSecond;
			stretch		= stretches.back();
			performing	= false;
			return;
		}
		// else

		const float t = timer / data.notifyingSecond; // Scaling into 0.0f ~ 1.0f
		stretch = Math::CalcBezierCurve( stretches, t );
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
		sheet.scale	= Donya::Vector2::Product( stretch, Donya::Vector2::Product( sheet.scale, scale ) );

		sheet.DrawPart( drawDepth );
	}
	void Explainer::Notify()
	{
		timer		= 0.0f;
		stretch		= { 1.0f, 1.0f };
		performing	= true;
	}
}
