#include "Meter.h"

#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"	// Use Donya::SignBit()

#include "FilePath.h"
#include "MeterParam.h"
#include "Music.h"
#include "Parameter.h"

namespace Meter
{
	namespace Parameter
	{
		void Load()
		{
			Impl::LoadMeter();
		}

	#if USE_IMGUI
		void Update( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			Impl::UpdateMeter( u8"Meter" );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI

		static ParamOperator<MeterParam> meterParam{ "Meter" };
		const MeterParam &GetMeter()
		{
			return meterParam.Get();
		}

		namespace Impl
		{
			void LoadMeter()
			{
				meterParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateMeter( const std::string &nodeCaption )
			{
				meterParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	namespace
	{
		static size_t meterSpriteID = NULL;
	}

	bool LoadResource()
	{
		Parameter::Load();

		meterSpriteID = Donya::Sprite::Load
		(
			GetSpritePath( SpriteAttribute::Meter ),
			GetSpriteInstanceCount( SpriteAttribute::Meter )
		);

		return ( meterSpriteID != NULL ) ? true : false;
	}


	void Drawer::Init( float max, float start, float dest )
	{
		current			= start;
		destination		= dest;
		maxAmount		= max;
		shakeTimer		= 0.0f;
		shakeRadian		= 0.0f;
		shakeAmpl		= 0.0f;
		shake			= Donya::Vector2::Zero();

		sprite.AssignSpriteID( meterSpriteID );
		sprite.origin	= { 0.0f, 0.0f };
		sprite.color	= { 1.0f, 1.0f, 1.0f };
	}
	void Drawer::Update( float elapsedTime )
	{
		ShakeUpdate( elapsedTime );

		const float diff = destination - current;
		if ( Donya::SignBit( diff ) == 0 ) { return; }
		// else

		// Reducing is immediately
		if ( diff < 0 )
		{
			current = destination;
			ShakeInit( diff );
			return;
		}
		// else

		// Recovering is incrementally
		current += Parameter::GetMeter().recoveryAmount * elapsedTime;

		if ( destination < current )
		{
			current = destination;
		}

		// Do not play the SE if the game pausing
		if ( !IsZero( elapsedTime ) )
		{
			// Make to do not magnify the volume by multiple playing in same time
			Donya::Sound::Stop( Music::RecoverHP );
			Donya::Sound::Play( Music::RecoverHP );
		}
	}
	void Drawer::Draw( float drawDepth ) const
	{
		const auto normalPos = sprite.pos;
		sprite.pos += shake;

		DrawGauge( drawDepth );
		DrawAmount( drawDepth );
		
		sprite.pos = normalPos;
	}
	void Drawer::DrawIcon( Icon kind, float drawDepth ) const
	{
		const auto registeredPos	= sprite.pos;
		const auto registeredColor	= sprite.color;
		const auto &data = Parameter::GetMeter();

		sprite.pos += shake;
		
		// Frame
		sprite.pos		+= data.iconFramePosOffset;
		sprite.texPos	=  data.iconFrameTexOrigin;
		sprite.texSize	=  data.iconFrameTexSize;
		sprite.DrawPart( drawDepth );

		// Picture
		sprite.pos		+= data.iconPicturePosOffset;
		sprite.texPos	=  data.iconPictureTexOrigin;
		sprite.texPos.x	+= data.iconPictureTexSize.x * scast<float>( kind );
		sprite.texSize	=  data.iconPictureTexSize;
		sprite.color	=  Donya::Vector3{ 1.0f, 1.0f, 1.0f };
		sprite.DrawPart( drawDepth );

		sprite.pos		= registeredPos;
		sprite.color	= registeredColor;
	}
	void Drawer::DrawRemains( FontAttribute font, int amount, float drawDepth ) const
	{
		const auto registeredPos = sprite.pos;
		const auto &data = Parameter::GetMeter();

		sprite.pos += shake;
		
		// Frame
		sprite.pos		+= data.remainFramePosOffset;
		sprite.texPos	=  data.remainFrameTexOrigin;
		sprite.texSize	=  data.remainFrameTexSize;
		sprite.DrawPart( drawDepth );

		const auto pFontRenderer = FontHelper::GetRendererOrNullptr( FontAttribute::Main );
		if ( !pFontRenderer )
		{
			_ASSERT_EXPR( 0, L"Error: Passed font is invalid!" );
			return;
		}
		// else

		// Number
		const Donya::Vector2 ssPos = sprite.pos + data.remainNumberPosOffset;
		const float oldDepth = Donya::Sprite::GetDrawDepth();
		Donya::Sprite::SetDrawDepth( drawDepth );
		pFontRenderer->DrawExt
		(
			std::to_wstring( amount ),
			ssPos, sprite.origin,
			data.remainNumberScale
		);
		Donya::Sprite::SetDrawDepth( oldDepth );

		sprite.pos = registeredPos;
	}
	void Drawer::SetDestination( float amount )
	{
		destination = std::max( 0.0f, std::min( maxAmount, amount ) );
	}
	void Drawer::SetDrawOption( const Donya::Vector2 &ssPos, const Donya::Vector3 &color, const Donya::Vector2 &scale )
	{
		sprite.pos		= ssPos;
		sprite.color	= color;
		sprite.scale	= scale;
	}
	bool Drawer::NowRecovering() const
	{
		const float diff = destination - current;
		return ( Donya::SignBit( diff ) == 1 ) ? true : false;
	}
	void Drawer::ShakeInit( float damage )
	{
		const auto &data = Parameter::GetMeter();
		
		shakeTimer	= data.shakeSecond;
		shakeRadian	= 0.0f;
		shake		= Donya::Vector2::Zero();

		const auto &range = data.shakeDamageInfluenceRange;
		if ( IsZero( range.y ) )
		{
			_ASSERT_EXPR( 0, L"Error: Division by zero!" );
			shakeAmpl = data.shakeBaseAmpl;
			return;
		}
		// else

		/*
		0		= damage <=	range.x
		0 ~ 1	=			range.x < damage <	range.y
		1		=								range.y <= damage
		*/

		float factor = fabsf( damage ) - range.x;		// "damage <= range.x" will be zero
		factor /= range.y;								// Scaling into "0 ~ 1"
		factor = Donya::Clamp( factor, 0.0f, 1.0f );	// Saturate

		shakeAmpl = data.shakeBaseAmpl + ( data.shakeDamageAmpl * factor );
	}
	void Drawer::ShakeUpdate( float elapsedTime )
	{
		shakeTimer -= elapsedTime;
		if ( shakeTimer <= 0.0f )
		{
			shakeTimer = 0.0f;
			shake = Donya::Vector2::Zero();
			return;
		}
		// else

		const auto &data = Parameter::GetMeter();
		if ( IsZero( data.shakeCycleSec ) || IsZero( data.shakeSecond ) )
		{
			_ASSERT_EXPR( 0, L"Error: Division by zero!" );
			shakeTimer = 0.0f; // To be end
			shake = Donya::Vector2::Zero();
			return;
		}
		// else

		const float rotAmount = PI / data.shakeCycleSec;
		shakeRadian += rotAmount * elapsedTime;

		const float ampl = cosf( shakeRadian ) * shakeAmpl;
		const float attenuateFactor = shakeTimer / data.shakeSecond;

		shake.x = ampl * attenuateFactor;
	}
	void Drawer::DrawGauge( float drawDepth ) const
	{
		const auto &data = Parameter::GetMeter();
		sprite.texPos  = data.gaugeTexOrigin;
		sprite.texSize = data.gaugeTexSize;
		
		sprite.DrawPart( drawDepth );
	}
	void Drawer::DrawAmount( float drawDepth ) const
	{
		if ( IsZero( maxAmount ) ) { return; }
		// else

		const auto &data = Parameter::GetMeter();
		sprite.texPos  = data.amountTexOrigin;
		sprite.texSize = data.amountTexSize;

		const float drawPercent = current / maxAmount;
		sprite.texSize.y *= drawPercent;

		const float reducedAmount = data.amountTexSize.y - sprite.texSize.y;
		sprite.texPos.y += reducedAmount;

		Donya::Vector2 drawOffset = data.amountPosOffset;
		drawOffset.y += reducedAmount;
		drawOffset = drawOffset.Product( sprite.scale );

		const auto registeredPos = sprite.pos;
		sprite.pos += drawOffset;
		sprite.DrawPart( drawDepth );
		sprite.pos = registeredPos;
	}

#if USE_IMGUI
	void MeterParam::ShowImGuiNode()
	{
		using Type = Donya::Vector2;
		auto ShowPart = []( const std::string &name, Type *posOffset, Type *texOrigin, Type *texSize )
		{
			if ( posOffset	) { ImGui::DragFloat2( ( name + u8"・描画位置オフセット"		).c_str(),		&posOffset->x	); }
			if ( texOrigin	) { ImGui::DragFloat2( ( name + u8"・テクスチャ原点"			).c_str(),		&texOrigin->x	); }
			if ( texSize	) { ImGui::DragFloat2( ( name + u8"・切り取りサイズ（全体）"	).c_str(),		&texSize->x		); }
		};

		ShowPart( u8"ゲージ",		nullptr,				&gaugeTexOrigin,		&gaugeTexSize		);
		ShowPart( u8"めもり",		&amountPosOffset,		&amountTexOrigin,		&amountTexSize		);
		ImGui::Text( "" );

		ShowPart( u8"アイコン：枠",	&iconFramePosOffset,	&iconFrameTexOrigin,	&iconFrameTexSize	);
		ShowPart( u8"アイコン：写真",	&iconPicturePosOffset,	&iconPictureTexOrigin,	&iconPictureTexSize	);
		ImGui::Text( "" );

		ShowPart( u8"残機：枠",	&remainFramePosOffset, &remainFrameTexOrigin, &remainFrameTexSize );
		ImGui::DragFloat2( u8"残機：数字・描画位置オフセット",	&remainNumberPosOffset.x	);
		ImGui::DragFloat2( u8"残機：数字・描画スケール",		&remainNumberScale.x		);

		ImGui::DragFloat( u8"１秒間の回復量", &recoveryAmount );
		recoveryAmount = std::max( 0.01f, recoveryAmount );

		if ( ImGui::TreeNode( u8"シェイク関連" ) )
		{
			ImGui::DragFloat( u8"全体秒数",				&shakeSecond,		0.01f );
			ImGui::DragFloat( u8"１周期の秒数",			&shakeCycleSec,		0.01f );
			ImGui::DragFloat( u8"基本増幅量",			&shakeBaseAmpl,		0.01f );
			ImGui::DragFloat( u8"量補正の増幅量",			&shakeDamageAmpl,	0.01f );
			ImGui::DragFloat2( u8"量補正の下限・上限",	&shakeDamageInfluenceRange.x );
			shakeSecond		= std::max( 0.01f, shakeSecond		);
			shakeCycleSec	= std::max( 0.01f, shakeCycleSec	);
			shakeBaseAmpl	= std::max( 0.0f,  shakeBaseAmpl	);
			shakeDamageAmpl	= std::max( 0.0f,  shakeDamageAmpl	);
			shakeDamageInfluenceRange.x = std::max( 1.0f,  shakeDamageInfluenceRange.x );
			shakeDamageInfluenceRange.y = std::max( 0.1f + shakeDamageInfluenceRange.x, shakeDamageInfluenceRange.y );

			ImGui::TreePop();
		}
	}
#endif // USE_IMGUI
}
