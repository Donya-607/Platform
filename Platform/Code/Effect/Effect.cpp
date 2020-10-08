#include "Effect.h"

#include <d3d11.h>
#include <unordered_map>
#include <vector>

#include <cereal/types/vector.hpp>

#include "Effekseer.h"

#include "../Donya/Constant.h"		// Use scast macro.
#include "../Donya/Serializer.h"
#include "../Donya/Useful.h"
#include "../Donya/UseImGui.h"

#include "EffectKind.h"
#include "EffectParam.h"
#include "EffectUtil.h"
#include "../FilePath.h"
#include "../Parameter.h"


namespace
{
	static constexpr size_t attrCount = scast<size_t>( Effect::Kind::KindCount );

	constexpr const char *GetEffectName( Effect::Kind attr )
	{
		switch ( attr )
		{
		case Effect::Kind::ChargeContinue:	return "Fire";
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
		return "ERROR_ATTR";
	}

	static ParamOperator<Effect::Param> effectParam{ "Effect" };
	const Effect::Param &FetchParameter()
	{
		return effectParam.Get();
	}
}

namespace Effect
{
#if USE_IMGUI
	void Param::ShowImGuiNode()
	{
		if ( effectScales.size() != attrCount )
		{
			constexpr float defaultScale = 1.0f;
			effectScales.resize( attrCount, defaultScale );
		}

		if ( ImGui::TreeNode( u8"スケール調整" ) )
		{
			ImGui::Text( u8"生成時に適用されます" );
			
			std::string caption{};
			for ( size_t i = 0; i < attrCount; ++i )
			{
				caption = GetEffectName( scast<Effect::Kind>( i ) );
				ImGui::DragFloat( caption.c_str(), &effectScales[i], 0.01f );
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI
}

namespace Effect
{

}
