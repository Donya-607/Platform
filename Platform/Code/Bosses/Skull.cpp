#include "Skull.h"

#include "../Parameter.h"

namespace Boss
{
	namespace Parameter
	{
		static ParamOperator<SkullParam> skullParam{ "Skull" };
		const  SkullParam &GetSkull()
		{
			return skullParam.Get();
		}

		namespace Impl
		{
			void LoadSkull()
			{
				skullParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateSkull( const std::string &nodeCaption )
			{
				skullParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	void Skull::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos )
	{
		Base::Update( elapsedTime, wsTargetPos );

		if ( NowDead() ) { return; }
		// else

		const auto &data = Parameter::GetSkull();

	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		{
			body.offset		= data.hitBoxOffset;
			body.size		= data.hitBoxSize;
			hurtBox.offset	= data.hurtBoxOffset;
			hurtBox.size	= data.hurtBoxSize;
		}
	#endif // USE_IMGUI

	}
	Kind Skull::GetKind() const { return Kind::Skull; }
	Definition::Damage Skull::GetTouchDamage() const
	{
		return Parameter::GetSkull().touchDamage;
	}
	void Skull::DieMoment()
	{

	}
	int  Skull::GetInitialHP() const
	{
		return Parameter::GetSkull().hp;
	}
	void Skull::AssignMyBody( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetSkull();
		body.pos		= wsPos;
		body.offset		= data.hitBoxOffset;
		body.size		= data.hitBoxSize;
		hurtBox.pos		= wsPos;
		hurtBox.offset	= data.hurtBoxOffset;
		hurtBox.size	= data.hurtBoxSize;
	}
#if USE_IMGUI
	bool Skull::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else

		Base::ShowImGuiNode( u8"基底部分" );

		ImGui::TextDisabled( u8"派生部分" );
		// if ( ImGui::TreeNode( u8"派生部分" ) )
		// {
		// 
		// 	ImGui::TreePop();
		// }

		ImGui::TreePop();
		return true;
	}
	void SkullParam::ShowImGuiNode()
	{
		ImGui::DragFloat3( u8"当たり判定・オフセット",			&hitBoxOffset.x,	0.01f	);
		ImGui::DragFloat3( u8"当たり判定・サイズ（半分を指定）",	&hitBoxSize.x,		0.01f	);
		ImGui::DragFloat3( u8"喰らい判定・オフセット",			&hurtBoxOffset.x,	0.01f	);
		ImGui::DragFloat3( u8"喰らい判定・サイズ（半分を指定）",	&hurtBoxSize.x,		0.01f	);
		ImGui::DragInt   ( u8"初期ＨＰ",							&hp							);
		touchDamage.ShowImGuiNode( u8"接触ダメージ設定" );
		hitBoxSize.x	= std::max( 0.0f,	hitBoxSize.x	);
		hitBoxSize.y	= std::max( 0.0f,	hitBoxSize.y	);
		hitBoxSize.z	= std::max( 0.0f,	hitBoxSize.z	);
		hurtBoxSize.x	= std::max( 0.0f,	hurtBoxSize.x	);
		hurtBoxSize.y	= std::max( 0.0f,	hurtBoxSize.y	);
		hurtBoxSize.z	= std::max( 0.0f,	hurtBoxSize.z	);
		hp				= std::max( 1,		hp				);
	}
#endif // USE_IMGUI
}
