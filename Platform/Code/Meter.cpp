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
	void Drawer::Update() {}
	void Drawer::Draw( float drawDepth ) const
	{
		DrawGauge( drawDepth );
		DrawAmount( drawDepth );
	}
	void Drawer::SetCurrent( float amount )
	{
		current = std::max( 0.0f, std::min( maxAmount, amount ) );
	}
	void Drawer::SetDrawOption( const Donya::Vector2 &ssPos, const Donya::Vector3 &color )
	{
		sprite.pos		= ssPos;
		sprite.color	= color;
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
		const auto &data = Parameter::GetMeter();
		sprite.texPos  = data.gaugeTexOrigin;
		sprite.texSize = data.gaugeTexSize;
		
		sprite.DrawPart( drawDepth );
	}
}
