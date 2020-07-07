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

	void Skull::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsRoomArea )
	{
		Base::Update( elapsedTime, wsTargetPos, wsRoomArea );

		if ( NowDead() ) { return; }
		// else

	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		{
			const auto &data = Parameter::GetSkull();
			body.offset		= data.hitBoxOffset;
			body.size		= data.hitBoxSize;
			hurtBox.offset	= data.hurtBoxOffset;
			hurtBox.size	= data.hurtBoxSize;
		}
	#endif // USE_IMGUI

		UpdateState( elapsedTime, wsTargetPos, wsRoomArea );
		UpdateMotion( elapsedTime, 0 );
	}
	float Skull::GetGravity() const
	{
		return Parameter::GetSkull().gravity;
	}
	Kind Skull::GetKind() const { return Kind::Skull; }
	Definition::Damage Skull::GetTouchDamage() const
	{
		return Parameter::GetSkull().touchDamage;
	}
	void Skull::DieMoment()
	{

	}
	void Skull::TransitionState( State nextState )
	{
		// Call XXXUninit() if needed
		switch ( nextState )
		{
		case Boss::Base::State::Appear:		break;
		case Boss::Base::State::Normal:		break;
		case Boss::Base::State::Die:		break;
		default: _ASSERT_EXPR( 0, L"Error: Unexpected state!" ); break;
		}

		status = nextState;
		switch ( nextState )
		{
		case Boss::Base::State::Appear:	/*AppearInit();*/	break;
		case Boss::Base::State::Normal:	NormalInit();	break;
		case Boss::Base::State::Die:		break;
		default: _ASSERT_EXPR( 0, L"Error: Unexpected state!" ); break;
		}
	}
	void Skull::UpdateState( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsRoomArea )
	{
		switch ( status )
		{
		case Boss::Base::State::Appear:	AppearUpdate( elapsedTime, wsTargetPos, wsRoomArea );	return;
		case Boss::Base::State::Normal:	NormalUpdate( elapsedTime, wsTargetPos, wsRoomArea );	return;
		case Boss::Base::State::Die:		return;
		default: _ASSERT_EXPR( 0, L"Error: Unexpected state!" ); return;
		}
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

	void Skull::NormalInit()
	{
		velocity = 0.0f;
	}
	void Skull::NormalUpdate( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsRoomArea )
	{

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
		ImGui::DragInt   ( u8"初期ＨＰ",							&hp							);
		ImGui::DragFloat ( u8"重力",								&gravity,			0.01f	);
		ImGui::DragFloat ( u8"ジャンプする高さ",					&jumpHeight,		0.01f	);
		ImGui::DragFloat ( u8"ジャンプにかける秒数",				&jumpTakeSeconds,	0.01f	);
		ImGui::DragFloat ( u8"走行速度",							&runSpeed,			0.01f	);
		ImGui::DragFloat3( u8"当たり判定・オフセット",			&hitBoxOffset.x,	0.01f	);
		ImGui::DragFloat3( u8"当たり判定・サイズ（半分を指定）",	&hitBoxSize.x,		0.01f	);
		ImGui::DragFloat3( u8"喰らい判定・オフセット",			&hurtBoxOffset.x,	0.01f	);
		ImGui::DragFloat3( u8"喰らい判定・サイズ（半分を指定）",	&hurtBoxSize.x,		0.01f	);
		touchDamage.ShowImGuiNode( u8"接触ダメージ設定" );
		gravity			= std::max( 0.001f,	gravity			);
		jumpHeight		= std::max( 0.001f,	jumpHeight		);
		jumpTakeSeconds	= std::max( 0.001f,	jumpTakeSeconds	);
		runSpeed		= std::max( 0.001f,	runSpeed		);
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
