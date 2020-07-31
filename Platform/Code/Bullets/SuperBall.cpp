#include "SuperBall.h"

#include "../Parameter.h"

namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<SuperBallParam> skullBusterParam{ "SuperBall", "Bullet/" };

		const SuperBallParam &GetSuperBall()
		{
			return skullBusterParam.Get();
		}

		namespace Impl
		{
			void LoadSuperBall()
			{
				skullBusterParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateSuperBall( const std::string &nodeCaption )
			{
				skullBusterParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}


	void SuperBall::Uninit() {} // No op
	Kind SuperBall::GetKind() const
	{
		return Kind::SuperBall;
	}
	Definition::Damage SuperBall::GetDamageParameter() const
	{
		return Parameter::GetSuperBall().basic.damage;
	}
	void SuperBall::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetSuperBall().basic;
		body.pos	= wsPos;
		body.offset	= data.hitBoxOffset;
		body.size	= data.hitBoxSize;
	}
#if USE_IMGUI
	void SuperBallParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"汎用設定" );
		ImGui::DragInt  ( u8"加速回数上限",	&accelerateCount );
		ImGui::DragFloat( u8"加速倍率",		&acceleratePercent, 0.01f );
		ImGui::Text( u8"（加速は反射時に行います）" );
		accelerateCount		= std::max(    0, accelerateCount   );
		acceleratePercent	= std::max( 0.0f, acceleratePercent );
	}
#endif // USE_IMGUI

}
