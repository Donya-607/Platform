#include "LoadPart.h"
#include "LoadParam.h"

#include "../Donya/Sprite.h"

#include "../Math.h"
#include "../Parameter.h"

namespace Performer
{
	namespace Parameter
	{
		static ParamOperator<LoadParam> parameter{ "Load", "Performance/" };

		void Load()
		{
			parameter.LoadParameter();
		}
		const LoadParam &Get()
		{
			return parameter.Get();
		}

	#if USE_IMGUI
		void Update( const std::string &nodeCaption )
		{
			parameter.ShowImGuiNode( nodeCaption );
		}
	#endif // USE_IMGUI
	}

	void LoadPart::LoadParameter()
	{
		Parameter::Load();
	}
#if USE_IMGUI
	void LoadPart::ShowImGuiNode( const std::string &nodeCaption )
	{
		Parameter::Update( nodeCaption );
	}

	void LoadParam::Icon::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		config.ShowImGuiNode( u8"基本設定" );
		ImGui::DragFloat( u8"一周にかかる秒数", &cycleSecond, 0.01f );
		cycleSecond = std::max( 0.01f, cycleSecond );

		constexpr Donya::Vector2 offsetRange{ -2000.0f,	2000.0f	};
		constexpr Donya::Vector2 scaleRange { -10.0f,	10.0f	};
		ImGui::Helper::ShowBezier2DNode( u8"バウンド・移動量",	&bounceOffsets,		offsetRange.x,	offsetRange.y );
		ImGui::Helper::ShowBezier2DNode( u8"バウンド・変形",		&bounceStretches,	scaleRange.x,	scaleRange.y  );

		ImGui::TreePop();
	}
	void LoadParam::String::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		config.ShowImGuiNode( u8"基本設定" );
		ImGui::DragFloat( u8"静止する秒数",				&staySecond,	0.01f );
		ImGui::DragFloat( u8"各文字の跳ねにかける秒数",	&partPopSecond,	0.01f );
		staySecond		= std::max( 0.0f,	staySecond		);
		partPopSecond	= std::max( 0.01f,	partPopSecond	);
		ImGui::DragFloat2( u8"跳ねの移動量", &popOffset.x, 1.0f );

		ImGui::TreePop();
	}
	void LoadParam::ShowImGuiNode()
	{
		partIcon.ShowImGuiNode	( u8"アイコン設定"	);
		partString.ShowImGuiNode( u8"文字列設定"		);
	}
#endif // USE_IMGUI


	void LoadPart::Icon::Init()
	{
		timer  = 0.0f;
		active = false;

		const auto &data = Parameter::Get().partIcon;
		// Assign the parameters without sprite identifier
		sprite = data.config;

		// Assign the sprite identifier only
		constexpr auto attr = SpriteAttribute::LoadingIcon;
		const bool result = sprite.LoadSprite( GetSpritePath( attr ), GetSpriteInstanceCount( attr ) );
		if ( !result )
		{
			_ASSERT_EXPR( 0, L"Error: Load the Loading-Icon is failed!" );
		}
	}
	void LoadPart::Icon::Update( float elapsedTime )
	{
		if ( !active ) { return; }
		// else

		timer += elapsedTime;

		const auto &data = Parameter::Get().partIcon;
		if ( IsZero( data.cycleSecond ) )
		{
			_ASSERT_EXPR( 0, L"Error: Division by zero!" );
			stretch = Donya::Vector2::Zero();
			return;
		}
		// else

		const float angle		= ToRadian( timer * 360.0f );
		const float cycleSpeed	= 1.0f / ( data.cycleSecond * 2.0f );
		// Use only semicircle part(0.0f ~ pi) of sine
		const float t			= fabsf( sinf( angle * cycleSpeed ) );

		bounce  = Math::CalcBezierCurve( data.bounceOffsets,	t );
		stretch = Math::CalcBezierCurve( data.bounceStretches,	t );
	}
	void LoadPart::Icon::Draw( float drawDepth )
	{
		if ( !active ) { return; }
		// else

		const auto oldPos	= sprite.pos;
		const auto oldScale	= sprite.scale;

		sprite.pos += basePos + bounce;
		sprite.scale.Product( stretch );
		sprite.Draw( drawDepth );

		sprite.pos		= oldPos;
		sprite.scale	= oldScale;
	}
	void LoadPart::Icon::Start( const Donya::Vector2 &ssBasePos )
	{
		timer	= 0.0f;
		stretch	= { 1.0f, 1.0f };
		bounce	= { 0.0f, 0.0f };
		basePos	= ssBasePos;
		active	= true;
	}
	void LoadPart::Icon::Stop()
	{
		active = false;
	}


	void LoadPart::String::Init()
	{
		timer = 0.0f;

		const auto &data = Parameter::Get().partString;

		scale		= data.config.scale;
		posOffset	= data.config.pos;
		pivot		= data.config.origin;
	}
	void LoadPart::String::Update( float elapsedTime )
	{
		if ( !active ) { return; }
		// else

		timer += elapsedTime;


	}
	void LoadPart::String::Draw( float drawDepth )
	{
		if ( !active ) { return; }
		// else


	}
	void LoadPart::String::Start( const Donya::Vector2 &ssBasePos )
	{
		timer	= 0.0f;
		basePos	= ssBasePos;
		active	= true;
	}
	void LoadPart::String::Stop()
	{
		active = false;
	}


	void LoadPart::Init()
	{

	}
	void LoadPart::Uninit()
	{

	}
	void LoadPart::Update( float elapsedTime )
	{

	}
	void LoadPart::Draw( float drawDepth )
	{

	}
	void LoadPart::Start( const Donya::Vector2 &ssBasePos )
	{

	}
	void LoadPart::Stop()
	{

	}
}
