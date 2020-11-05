#include "LoadPart.h"
#include "LoadParam.h"

#include "../Donya/Sprite.h"

#include "../Common.h"
#include "../FontHelper.h"
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
		ImGui::DragFloat( u8"跳ねを行う秒数の前後範囲",	&popInflRange,	0.01f );
		staySecond		= std::max( 0.0f,	staySecond		);
		partPopSecond	= std::max( 0.01f,	partPopSecond	);
		popInflRange	= std::max( 0.0f,	popInflRange	);
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


	namespace
	{
		constexpr int Length( const wchar_t *str )
		{
			// See https://stackoverflow.com/questions/25890784/computing-length-of-a-c-string-at-compile-time-is-this-really-a-constexpr
			return ( *str ) ? 1 + Length( str + 1 ) : 0;
		}
		constexpr const wchar_t *showingStr = L"Loading...";
		constexpr int    showingStrCount  = Length( showingStr ) + 1;
		constexpr float  showingStrCountF = scast<float>( showingStrCount );
		constexpr size_t showingStrCountS = scast<size_t>( showingStrCount );
	}
	void LoadPart::String::Init()
	{
		timer = 0.0f;

		popWeights.resize( showingStrCount, 0.0f );

		const auto &data = Parameter::Get().partString;

		scale		= data.config.scale;
		posOffset	= data.config.pos;
		pivot		= data.config.origin;
	}
	void LoadPart::String::Update( float elapsedTime )
	{
		if ( !active ) { return; }
		// else

		if ( popWeights.size() != showingStrCountS )
		{
			_ASSERT_EXPR( 0, L"Error: Character count did not match!" );
			popWeights.resize( showingStrCount, 0.0f );
		}

		const auto &data = Parameter::Get().partString;
		const float wholeCycleSecond = data.staySecond + ( data.partPopSecond * showingStrCountF );

		timer += elapsedTime;
		timer = fmodf( timer, wholeCycleSecond );

		if ( timer < data.staySecond )
		{
			for ( auto &it : popWeights ) { it = 0.0f; }
			return;
		}
		// else

		for ( int i = 0; i < showingStrCount; ++i )
		{
			auto &character = popWeights[i];

			float iF = scast<float>( i );

			const float currentStartSec	= data.staySecond + ( data.partPopSecond * iF );
			const float currentEndSec	= currentStartSec + data.partPopSecond;
			if	(
					currentStartSec	<= timer - data.popInflRange ||
					currentEndSec	>= timer + data.popInflRange
				)
			{
				character = 0.0f;
				continue;
			}
			// else

			// TODO: lerp the weight by distance
			character = 1.0f;
		}
	}
	void LoadPart::String::Draw( float drawDepth )
	{
		if ( !active ) { return; }
		// else

		const auto pRenderer = FontHelper::GetRendererOrNullptr( FontAttribute::Main );
		if ( !pRenderer ) { return; }
		// else

		const auto &data = Parameter::Get().partString;

		Donya::Vector2 popAmount;
		Donya::Vector2 drawPosOffset;
		Donya::Vector2 drawnLength;
		const Donya::Vector2 baseDrawPos = basePos + posOffset;
		for ( int i = 0; i < showingStrCount; ++i )
		{
			const float &weight = popWeights[i];
			popAmount = data.popOffset * weight;

			drawnLength = pRenderer->DrawExt
			(
				showingStr,
				baseDrawPos + drawPosOffset + popAmount,
				pivot, scale
			);
			drawPosOffset.x += drawnLength.x;
		}
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
		timer		= 0.0f;
		BGMaskAlpha	= 1.0f;
		active		= false;

		partIcon.Init();
		partString.Init();
	}
	void LoadPart::Uninit()
	{
		// No op
	}
	void LoadPart::Update( float elapsedTime )
	{
		if ( !active ) { return; }
		// else

		timer += elapsedTime;

		partIcon.Update( elapsedTime );
		partString.Update( elapsedTime );
	}
	void LoadPart::Draw( float drawDepth )
	{
		if ( !active ) { return; }
		// else

		const float oldDepth = Donya::Sprite::GetDrawDepth();
		Donya::Sprite::SetDrawDepth( drawDepth );
		Donya::Sprite::DrawRect
		(
			Common::HalfScreenWidthF(),	Common::HalfScreenWidthF(),
			Common::ScreenWidthF(),		Common::ScreenWidthF(),
			Donya::Color::Code::BLACK,
			BGMaskAlpha
		);
		Donya::Sprite::SetDrawDepth( oldDepth );

		partIcon.Draw( drawDepth );
		partString.Draw( drawDepth );
	}
	void LoadPart::Start( const Donya::Vector2 &ssBasePos )
	{
		timer		= 0.0f;
		BGMaskAlpha	= 1.0f;
		active		= true;

		partIcon.Start( ssBasePos );
		partString.Start( ssBasePos );
	}
	void LoadPart::Stop()
	{
		active = false;

		partIcon.Stop();
		partString.Stop();
	}
}
