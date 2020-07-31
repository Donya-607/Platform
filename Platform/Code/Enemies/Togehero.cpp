#include "Togehero.h"

#include "../Parameter.h"

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

		UpdateOutSideState( wsScreen );
		if ( NowWaiting() )
		{
			RespawnIfSpawnable();
			if ( NowWaiting() ) // Still inactive
			{
				return;
			}
		}

		const auto &data = Parameter::GetTogehero();

	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		{
			AssignMyBody( body.pos );
		}
	#endif // USE_IMGUI

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
		}
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
	}
#if USE_IMGUI
	bool Togehero::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else

		Base::ShowImGuiNode( u8"äÓíÍïîï™" );

		ImGui::TextDisabled( u8"îhê∂ïîï™" );
		// if ( ImGui::TreeNode( u8"îhê∂ïîï™" ) )
		// {
		// 
		// 	ImGui::TreePop();
		// }

		ImGui::TreePop();
		return true;
	}
	void TogeheroParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"îƒópê›íË" );
		ImGui::DragFloat( u8"à⁄ìÆë¨ìx",				&moveSpeed,		0.1f	);
		ImGui::DragFloat( u8"âÒì]ë¨ìx[degree/s]",	&rotateSpeed,	1.0f	);
	}
#endif // USE_IMGUI
}
