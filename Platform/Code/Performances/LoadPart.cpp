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

		constexpr Donya::Vector2 offsetRange{ -512.0f,	512.0f	};
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
		ImGui::DragFloat( u8"フェードアウトにかける秒数", &fadeSecond, 0.01f );;
		fadeSecond = std::max( 0.0f, fadeSecond );
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
	void LoadPart::Icon::Update( float deltaTime )
	{
		if ( !active ) { return; }
		// else

	#if USE_IMGUI
		// Apply the modification as immediately
		{
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
	#endif // USE_IMGUI

		timer += deltaTime;

		const auto &data = Parameter::Get().partIcon;
		if ( IsZero( data.cycleSecond ) )
		{
			_ASSERT_EXPR( 0, L"Error: Division by zero!" );
			stretch = Donya::Vector2::Zero();
			return;
		}
		// else

		// Fail safe
		if ( data.bounceOffsets.size() < 2 )
		{
			bounce = Donya::Vector2::Zero();
			return;
		}
		if ( data.bounceStretches.size() < 2 )
		{
			bounce = Donya::Vector2::Zero();
			return;
		}
		// else

		const float cycleSpeed	= 1.0f / data.cycleSecond;
		const float angle		= ToRadian( timer * 360.0f );
		const float sin_01		= ( sinf( angle * cycleSpeed ) + 1.0f ) * 0.5f;

		bounce  = Math::CalcBezierCurve( data.bounceOffsets,	sin_01 );
		stretch = Math::CalcBezierCurve( data.bounceStretches,	sin_01 );
	}
	void LoadPart::Icon::Draw( float drawDepth, float drawAlpha )
	{
		if ( !active && drawAlpha <= 0.0f ) { return; }
		// else

		const auto oldPos	= sprite.pos;
		const auto oldAlpha	= sprite.alpha;
		const auto oldScale	= sprite.scale;

		sprite.pos += basePos + bounce;
		sprite.scale = Donya::Vector2::Product( sprite.scale, stretch );
		sprite.alpha *= drawAlpha;
		sprite.Draw( drawDepth );

		sprite.pos		= oldPos;
		sprite.alpha	= oldAlpha;
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
		constexpr int    showingStrCount  = Length( showingStr );
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
	void LoadPart::String::Update( float deltaTime )
	{
		if ( !active ) { return; }
		// else

	#if USE_IMGUI
		// Apply the modification as immediately
		{
			const auto &data = Parameter::Get().partString;
			scale		= data.config.scale;
			posOffset	= data.config.pos;
			pivot		= data.config.origin;
		}
	#endif // USE_IMGUI

		if ( popWeights.size() != showingStrCountS )
		{
			_ASSERT_EXPR( 0, L"Error: Character count did not match!" );
			popWeights.resize( showingStrCount, 0.0f );
		}

		const auto &data = Parameter::Get().partString;
		const float wholeCycleSecond = data.staySecond + ( data.partPopSecond * showingStrCountF );

		timer += deltaTime;
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
			const float left  = timer - data.popInflRange;
			const float right = timer + data.popInflRange;
			if ( currentStartSec - data.popInflRange <= timer && timer < currentEndSec + data.popInflRange )
			{
				character = 1.0f;
			}
			else
			{
				// TODO: lerp the weight by distance
				character = 0.0f;
			}
		}
	}
	void LoadPart::String::Draw( float drawDepth, float drawAlpha )
	{
		if ( !active && drawAlpha <= 0.0f ) { return; }
		// else

		const auto pRenderer = FontHelper::GetRendererOrNullptr( FontAttribute::Main );
		if ( !pRenderer ) { return; }
		// else

		const auto &data = Parameter::Get().partString;

		const float oldDepth = Donya::Sprite::GetDrawDepth();
		Donya::Sprite::SetDrawDepth( drawDepth );

		Donya::Vector2 popAmount;
		Donya::Vector2 drawPosOffset;
		Donya::Vector2 drawnLength;
		const Donya::Vector2 baseDrawPos	= basePos + posOffset;
		const Donya::Vector4 drawColor		{ 1.0f, 1.0f, 1.0f, drawAlpha };
		for ( int i = 0; i < showingStrCount; ++i )
		{
			const float &weight = popWeights[i];
			popAmount = data.popOffset * weight;

			drawnLength = pRenderer->DrawExt
			(
				std::wstring{ showingStr[i] },
				baseDrawPos + drawPosOffset + popAmount,
				pivot, scale,
				drawColor
			);
			drawPosOffset.x += drawnLength.x;
		}

		Donya::Sprite::SetDrawDepth( oldDepth );
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
		alpha		= 1.0f;
		active		= false;
		maskColor	= { 0.0f, 0.0f, 0.0f };

		partIcon.Init();
		partString.Init();
	}
	void LoadPart::Uninit()
	{
		// No op
	}
	void LoadPart::UpdateIfActive( float deltaTime )
	{
		if ( !active )
		{
			const auto &fadeSec = Parameter::Get().fadeSecond;
			if ( IsZero( fadeSec ) )
			{
				alpha = 0.0f;
			}
			else
			{
				const float toOpaque = 1.0f / fadeSec;
				alpha -= toOpaque * deltaTime;
			}

			if ( alpha <= 0.0f )
			{
				alpha = 0.0f;
				partIcon.Stop();
				partString.Stop();
				return;
			}
		}

		timer += deltaTime;

		partIcon.Update( deltaTime );
		partString.Update( deltaTime );
	}
	void LoadPart::DrawIfActive( float drawDepth )
	{
		if ( !active && alpha <= 0.0f ) { return; }
		// else

		const float oldDepth = Donya::Sprite::GetDrawDepth();
		Donya::Sprite::SetDrawDepth( drawDepth );
		Donya::Sprite::DrawRect
		(
			Common::HalfScreenWidthF(),	Common::HalfScreenWidthF(),
			Common::ScreenWidthF(),		Common::ScreenWidthF(),
			maskColor.x, maskColor.y, maskColor.z, alpha
		);
		Donya::Sprite::SetDrawDepth( oldDepth );

		partIcon.Draw	( drawDepth, alpha );
		partString.Draw	( drawDepth, alpha );
	}
	void LoadPart::Start( const Donya::Vector2 &ssBasePos, const Donya::Color::Code &color )
	{
		timer		= 0.0f;
		alpha		= 1.0f;
		active		= true;
		maskColor	= Donya::Color::MakeColor( color );

		partIcon.Start( ssBasePos );
		partString.Start( ssBasePos );
	}
	void LoadPart::Stop()
	{
		active = false;

		// The parts is still update until completely fade-outed(alpha <= 0.0f)
	}
}
