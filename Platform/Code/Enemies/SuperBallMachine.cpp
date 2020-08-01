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

	void SuperBallMachine::Uninit()
	{
		Base::Uninit();
		intervalTimer = 0.0f;
	}
	void SuperBallMachine::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
		Base::Update( elapsedTime, wsTargetPos, wsScreen );
		if ( NowWaiting() ) { return; }
		// else

	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		AssignMyBody( body.pos );
	#endif // USE_IMGUI

		ShotIfNeeded( elapsedTime, wsTargetPos );

		velocity.y -= Parameter::GetSuperBallMachine().gravity * elapsedTime;
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
	void SuperBallMachine::ShotIfNeeded( float elapsedTime, const Donya::Vector3 &wsTargetPos )
	{
		const auto &data = Parameter::GetSuperBallMachine();

		intervalTimer += elapsedTime;

		// Judging to shotable

		if ( intervalTimer < data.fireIntervalSecond ) { return; }
		// else

		Donya::Collision::Box3F capturingArea{};
		capturingArea.pos  = GetPosition();
		capturingArea.size = data.capturingArea;
		if ( !Donya::Collision::IsHit( wsTargetPos, capturingArea, /* considerExistFlag = */ false ) ) { return; }
		// else

		// Shot here

		intervalTimer = 0.0f;

		Bullet::FireDesc desc = data.fireDesc;
		
		float lookingSign = 1.0f;
		{
			const Donya::Vector3 front = orientation.LocalFront();
			const float dot = Donya::Dot( front, Donya::Vector3::Right() );
			lookingSign = Donya::SignBitF( dot );
		}
		desc.position.x		*= lookingSign;
		desc.position		+= GetPosition();
		desc.owner			=  hurtBox.id;
		
		const float shotRadian			= ToRadian( data.fireDegree );
		const float shotRadianLeftVer	= ToRadian( 180.0f ) - shotRadian;
		desc.direction.x	= cosf( ( Donya::SignBit( lookingSign ) == -1 ) ? shotRadianLeftVer : shotRadian );
		desc.direction.y	= sinf( shotRadian );
		desc.direction.z	= 0.0f;

		Bullet::Admin::Get().RequestFire( desc );
	}
#if USE_IMGUI
	bool SuperBallMachine::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else

		Base::ShowImGuiNode( u8"ξκͺ" );

		if ( ImGui::TreeNode( u8"hΆͺ" ) )
		{
			ImGui::DragFloat( u8"b^C}E­ΛΤu", &intervalTimer, 0.01f );

			ImGui::TreePop();
		}

		ImGui::TreePop();
		return true;
	}
	void SuperBallMachineParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"Δpέθ" );
		fireDesc.ShowImGuiNode( u8"­Λeέθ" );

		ImGui::DragFloat3( u8"υGΝΝiΌaj",				&capturingArea.x,		0.01f	);
		ImGui::DragFloat ( u8"­ΛΤuibj",				&fireIntervalSecond,	0.01f	);
		ImGui::DragFloat ( u8"­ΛpxiDegreeEEό«j",	&fireDegree,			0.1f	);
		ImGui::DragFloat ( u8"dΝ",							&gravity,				0.01f	);

		capturingArea.x		= std::max( 0.01f, capturingArea.x		);
		capturingArea.y		= std::max( 0.01f, capturingArea.y		);
		capturingArea.z		= std::max( 0.01f, capturingArea.z		);
		fireIntervalSecond	= std::max( 0.01f, fireIntervalSecond	);
	}
#endif // USE_IMGUI
}
