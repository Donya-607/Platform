#include "SkullBullet.h"

#include "../Parameter.h"

namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<SkullBusterParam> skullBusterParam{ "SkullBuster" };

		const SkullBusterParam &GetSkullBuster()
		{
			return skullBusterParam.Get();
		}

		namespace Impl
		{
			void LoadSkullBuster()
			{
				skullBusterParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateSkullBuster( const std::string &nodeCaption )
			{
				skullBusterParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	void SkullBuster::Uninit() {} // No op
	Kind SkullBuster::GetKind() const
	{
		return Kind::SkullBuster;
	}
	Definition::Damage SkullBuster::GetDamage() const
	{
		return Parameter::GetSkullBuster().damage;
	}
	void SkullBuster::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetSkullBuster();
		body.pos	= wsPos;
		body.offset	= data.hitBoxOffset;
		body.size	= data.hitBoxSize;
	}
#if USE_IMGUI
	void SkullBusterParam::ShowImGuiNode()
	{
		ImGui::DragFloat3( u8"当たり判定・オフセット",			&hitBoxOffset.x,	0.01f );
		ImGui::DragFloat3( u8"当たり判定・サイズ（半分を指定）",	&hitBoxSize.x,		0.01f );
		hitBoxSize.x = std::max( 0.0f, hitBoxSize.x );
		hitBoxSize.y = std::max( 0.0f, hitBoxSize.y );
		hitBoxSize.z = std::max( 0.0f, hitBoxSize.z );
		damage.ShowImGuiNode( u8"基本ダメージ設定" );
	}
#endif // USE_IMGUI
}
