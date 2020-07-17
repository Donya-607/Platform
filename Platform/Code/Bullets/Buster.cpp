#include "Buster.h"

#include "../Parameter.h"

namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<BusterParam> busterParam{ "Buster" };

		const BusterParam &GetBuster()
		{
			return busterParam.Get();
		}

		namespace Impl
		{
			void LoadBuster()
			{
				busterParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateBuster( const std::string &nodeCaption )
			{
				busterParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	int  Buster::livingCount = 0;
	int  Buster::GetLivingCount()
	{
		return livingCount;
	}
	void Buster::Init( const FireDesc &parameter )
	{
		Base::Init( parameter );

		livingCount++;
	}
	void Buster::Uninit()
	{
		livingCount--;
	}
	Kind Buster::GetKind() const
	{
		return Kind::Buster;
	}
	Definition::Damage Buster::GetDamageParameter() const
	{
		return Parameter::GetBuster().damage;
	}
	void Buster::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetBuster();
		body.pos	= wsPos;
		body.offset	= data.hitBoxOffset;
		body.size	= data.hitBoxSize;
	}
#if USE_IMGUI
	void Buster::ShowImGuiNode( const std::string &nodeCaption )
	{
		Base::ShowImGuiNode( nodeCaption );

		/*
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		Base::ShowImGuiNode( u8"基底部分" );

		if ( ImGui::TreeNode( u8"派生部分" ) )
		{
			ImGui::TreePop();
		}

		ImGui::TreePop();
		*/
	}
	void BusterParam::ShowImGuiNode()
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
