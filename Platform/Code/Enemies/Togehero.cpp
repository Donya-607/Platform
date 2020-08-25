#include "Togehero.h"

#include "../Parameter.h"
#include "../Donya/Keyboard.h"

namespace Enemy
{
	namespace Parameter
	{
		static ParamOperator<TogeheroParam> togeheroParam{ "Togehero", "Enemy/" };
		const TogeheroParam &GetTogehero()
		{
			return togeheroParam.Get();
		}

		namespace Impl
		{
			void LoadTogehero()
			{
				togeheroParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateTogehero( const std::string &nodeCaption )
			{
				togeheroParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	void Togehero::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		Base::Update( elapsedTime, wsTargetPos, wsScreen );
		if ( NowWaiting() ) { return; }
		// else

	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		{
			AssignMyBody( body.pos );
		}
	#endif // USE_IMGUI

		const auto &data = Parameter::GetTogehero();

		// Move
		{
			// HACK: Should consider the offset of myself's hit-box?
			const auto  toTarget	= wsTargetPos - body.WorldPosition();
			const float dist		= toTarget.Length();

			const float speed		= std::min( dist, data.moveSpeed );
			velocity = toTarget.Unit() * speed;
		}

		// Rotate
		{
			const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( data.rotateSpeed ) );
			orientation.RotateBy( rotation );

			body.offset		= orientation.RotateVector( data.basic.hitBoxOffset		);
			hurtBox.offset	= orientation.RotateVector( data.basic.hurtBoxOffset	);
		}

		UpdateMotionIfCan( elapsedTime * data.animePlaySpeed, 0 );

	#if DEBUG_MODE
		if ( Donya::Keyboard::Trigger( VK_SPACE ) )
		{
			model.pose.UpdateTransformMatrices();
		}
	#endif // DEBUG_MODE
	}
	Kind Togehero::GetKind() const { return Kind::Togehero; }
	Definition::Damage Togehero::GetTouchDamage() const
	{
		return Parameter::GetTogehero().basic.touchDamage;
	}
	int  Togehero::GetInitialHP() const
	{
		return Parameter::GetTogehero().basic.hp;
	}
	void Togehero::AssignMyBody( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetTogehero().basic;
		body.pos		= wsPos;
		body.offset		= data.hitBoxOffset;
		body.size		= data.hitBoxSize;
		hurtBox.pos		= wsPos;
		hurtBox.offset	= data.hurtBoxOffset;
		hurtBox.size	= data.hurtBoxSize;

		body.offset		= orientation.RotateVector( body.offset		);
		hurtBox.offset	= orientation.RotateVector( hurtBox.offset	);
	}
#if USE_IMGUI
	bool Togehero::ShowImGuiNode( const std::string &nodeCaption )
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
	void TogeheroParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"汎用設定" );
		ImGui::DragFloat( u8"移動速度",				&moveSpeed,			0.1f	);
		ImGui::DragFloat( u8"回転速度[degree/s]",	&rotateSpeed,		1.0f	);
		ImGui::DragFloat( u8"モーション再生速度",		&animePlaySpeed,	0.01f	);
	}
#endif // USE_IMGUI
}
