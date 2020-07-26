#include "Meter.h"

#include "Donya/Sprite.h"

#include "FilePath.h"
#include "MeterParam.h"
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


	void Drawer::Init( float max, float initial )
	{
		current			= initial;
		maxAmount		= max;

		sprite.AssignSpriteID( meterSpriteID );
		sprite.origin	= { 0.0f, 0.0f };
		sprite.color	= { 1.0f, 1.0f, 1.0f };
	}
	void Drawer::Update( float elapsedTime ) {}
	void Drawer::Draw( float drawDepth ) const
	{
		DrawGauge( drawDepth );
		DrawAmount( drawDepth );
	}
	void Drawer::SetCurrent( float amount )
	{
		current = std::max( 0.0f, std::min( maxAmount, amount ) );
	}
	void Drawer::SetDrawOption( const Donya::Vector2 &ssPos, const Donya::Vector3 &color, const Donya::Vector2 &scale )
	{
		sprite.pos		= ssPos;
		sprite.color	= color;
		sprite.scale	= scale;
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

		const auto oldPos = sprite.pos;
		sprite.pos += drawOffset;
		sprite.DrawPart( drawDepth );
		sprite.pos = oldPos;
	}

#if USE_IMGUI
	void MeterParam::ShowImGuiNode()
	{
		ImGui::DragFloat2( u8"ゲージ・テクスチャ原点",			&gaugeTexOrigin.x	);
		ImGui::DragFloat2( u8"ゲージ・切り取りサイズ（全体）",		&gaugeTexSize.x		);
		ImGui::DragFloat2( u8"めもり・描画位置オフセット",			&amountPosOffset.x	);
		ImGui::DragFloat2( u8"めもり・テクスチャ原点",			&amountTexOrigin.x	);
		ImGui::DragFloat2( u8"めもり・切り取りサイズ（全体）",		&amountTexSize.x	);
	}
#endif // USE_IMGUI
}
