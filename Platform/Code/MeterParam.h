#pragma once

#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"

namespace Meter
{
	struct MeterParam
	{
	public:
		Donya::Vector2 gaugeTexOrigin		{  0.0f,  0.0f };
		Donya::Vector2 gaugeTexSize			{ 32.0f, 32.0f };

		Donya::Vector2 amountPosOffset		{  0.0f,  0.0f }; // From origin
		Donya::Vector2 amountTexOrigin		{  0.0f,  0.0f };
		Donya::Vector2 amountTexSize		{ 32.0f, 32.0f };


		Donya::Vector2 iconFramePosOffset	{  0.0f,  0.0f }; // From origin
		Donya::Vector2 iconFrameTexOrigin	{  0.0f,  0.0f };
		Donya::Vector2 iconFrameTexSize		{ 32.0f, 32.0f };

		Donya::Vector2 iconPicturePosOffset	{  0.0f,  0.0f }; // From "iconFramePosOffset"
		Donya::Vector2 iconPictureTexOrigin	{  0.0f,  0.0f };
		Donya::Vector2 iconPictureTexSize	{ 32.0f, 32.0f };


		Donya::Vector2 remainFramePosOffset	{  0.0f,  0.0f }; // From origin
		Donya::Vector2 remainFrameTexOrigin	{  0.0f,  0.0f };
		Donya::Vector2 remainFrameTexSize	{ 32.0f, 32.0f };

		Donya::Vector2 remainNumberPosOffset{  0.0f,  0.0f }; // From "remainFramePosOffset"
		Donya::Vector2 remainNumberScale	{  1.0f,  1.0f };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( gaugeTexOrigin	),
				CEREAL_NVP( gaugeTexSize	),
				CEREAL_NVP( amountPosOffset	),
				CEREAL_NVP( amountTexOrigin	),
				CEREAL_NVP( amountTexSize	)
			);

			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( iconFramePosOffset		),
					CEREAL_NVP( iconFrameTexOrigin		),
					CEREAL_NVP( iconFrameTexSize		),
					CEREAL_NVP( iconPicturePosOffset	),
					CEREAL_NVP( iconPictureTexOrigin	),
					CEREAL_NVP( iconPictureTexSize		),
					CEREAL_NVP( remainFramePosOffset	),
					CEREAL_NVP( remainFrameTexOrigin	),
					CEREAL_NVP( remainFrameTexSize		),
					CEREAL_NVP( remainNumberPosOffset	),
					CEREAL_NVP( remainNumberScale		),
				);
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Meter::MeterParam, 1 )
