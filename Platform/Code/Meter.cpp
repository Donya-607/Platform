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
	void Drawer::DrawIcon( Icon kind, float drawDepth ) const
	{
		const auto registeredPos	= sprite.pos;
		const auto registeredColor	= sprite.color;
		const auto &data = Parameter::GetMeter();
		
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
			if ( posOffset	) { ImGui::DragFloat2( ( name + u8"�E�`��ʒu�I�t�Z�b�g"		).c_str(),		&posOffset->x	); }
			if ( texOrigin	) { ImGui::DragFloat2( ( name + u8"�E�e�N�X�`�����_"			).c_str(),		&texOrigin->x	); }
			if ( texSize	) { ImGui::DragFloat2( ( name + u8"�E�؂���T�C�Y�i�S�́j"	).c_str(),		&texSize->x		); }
		};

		ShowPart( u8"�Q�[�W",		nullptr,				&gaugeTexOrigin,		&gaugeTexSize		);
		ShowPart( u8"�߂���",		&amountPosOffset,		&amountTexOrigin,		&amountTexSize		);
		ImGui::Text( "" );

		ShowPart( u8"�A�C�R���F�g",	&iconFramePosOffset,	&iconFrameTexOrigin,	&iconFrameTexSize	);
		ShowPart( u8"�A�C�R���F�ʐ^",	&iconPicturePosOffset,	&iconPictureTexOrigin,	&iconPictureTexSize	);
		ImGui::Text( "" );

		ShowPart( u8"�c�@�F�g",	&remainFramePosOffset, &remainFrameTexOrigin, &remainFrameTexSize );
		ImGui::DragFloat2( u8"�c�@�F�����E�`��ʒu�I�t�Z�b�g",	&remainNumberPosOffset.x	);
		ImGui::DragFloat2( u8"�c�@�F�����E�`��X�P�[��",		&remainNumberScale.x		);
	}
#endif // USE_IMGUI
}
