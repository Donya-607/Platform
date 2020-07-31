#include "SuperBallMachine.h"

#include "../Parameter.h"

namespace Enemy
{
	namespace Parameter
	{
		static ParamOperator<SuperBallMachineParam> superBallMachineParam{ "SuperBallMachine", "Enemy/" };
		const SuperBallMachineParam &GetSuperBallMachine()
		{
			return superBallMachineParam.Get();
		}

		namespace Impl
		{
			void LoadSuperBallMachine()
			{
				superBallMachineParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateSuperBallMachine( const std::string &nodeCaption )
			{
				superBallMachineParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	void SuperBallMachine::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
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

		const auto &data = Parameter::GetSuperBallMachine();

	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		AssignMyBody( body.pos );
	#endif // USE_IMGUI

	}
	Kind SuperBallMachine::GetKind() const { return Kind::SuperBallMachine; }
	Definition::Damage SuperBallMachine::GetTouchDamage() const
	{
		return Parameter::GetSuperBallMachine().basic.touchDamage;
	}
	int  SuperBallMachine::GetInitialHP() const
	{
		return Parameter::GetSuperBallMachine().basic.hp;
	}
	void SuperBallMachine::AssignMyBody( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetSuperBallMachine().basic;
		body.pos		= wsPos;
		body.offset		= data.hitBoxOffset;
		body.size		= data.hitBoxSize;
		hurtBox.pos		= wsPos;
		hurtBox.offset	= data.hurtBoxOffset;
		hurtBox.size	= data.hurtBoxSize;
	}
#if USE_IMGUI
	bool SuperBallMachine::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else

		Base::ShowImGuiNode( u8"Šî’ê•”•ª" );

		ImGui::TextDisabled( u8"”h¶•”•ª" );
		// if ( ImGui::TreeNode( u8"”h¶•”•ª" ) )
		// {
		// 
		// 	ImGui::TreePop();
		// }

		ImGui::TreePop();
		return true;
	}
	void SuperBallMachineParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"”Ä—pİ’è" );
		fireDesc.ShowImGuiNode( u8"”­Ë’eİ’è" );

		ImGui::DragFloat3( u8"õ“G”ÍˆÍ",						&capturingArea.x,		0.01f	);
		ImGui::DragFloat ( u8"”­ËŠÔŠui•bj",				&fireIntervalSecond,	0.1f	);
		ImGui::DragFloat ( u8"”­ËŠp“xiDegreeE‰EŒü‚«j",	&fireDegree,			0.1f	);

		capturingArea.x		= std::max( 0.01f, capturingArea.x		);
		capturingArea.y		= std::max( 0.01f, capturingArea.y		);
		capturingArea.z		= std::max( 0.01f, capturingArea.z		);
		fireIntervalSecond	= std::max( 0.01f, fireIntervalSecond	);
	}
#endif // USE_IMGUI
}
