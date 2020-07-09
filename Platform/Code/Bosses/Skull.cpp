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

	void Skull::MoverBase::Init( Skull &inst ) {}
	void Skull::MoverBase::Uninit( Skull &inst ) {}
	void Skull::MoverBase::Update( Skull &inst, float elapsedTime, const Input &input ) {}
	void Skull::MoverBase::PhysicUpdate( Skull &inst, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		inst.Base::PhysicUpdate( elapsedTime, solids );
	}
	
	void Skull::AppearPerformance::Init( Skull &inst )
	{
		inst.AppearInit();
	}
	void Skull::AppearPerformance::Update( Skull &inst, float elapsedTime, const Input &input )
	{
		inst.AppearUpdate( elapsedTime, input );
	}
	void Skull::AppearPerformance::PhysicUpdate( Skull &inst, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		wasLanding = inst.AppearPhysicUpdate( elapsedTime, solids );
	}
	bool Skull::AppearPerformance::ShouldChangeMover( const Skull &inst ) const
	{
		return wasLanding; // For now, transition as immediately if the falling is finished
	}
	std::function<void()> Skull::AppearPerformance::GetChangeStateMethod( Skull &inst ) const
	{
		return [&]() { inst.AssignMover<DetectTargetAction>(); };
	}
#if USE_IMGUI
	std::string Skull::AppearPerformance::GetMoverName() const
	{
		return u8"�o��";
	}
#endif // USE_IMGUI
	
	void Skull::DetectTargetAction::Init( Skull &inst )
	{
		inst.velocity = 0.0f;
	}
	void Skull::DetectTargetAction::Update( Skull &inst, float elapsedTime, const Input &input )
	{
	#if DEBUG_MODE
		inst.velocity = ( input.wsTargetPos - inst.GetHitBox().WorldPosition() ).Unit();
	#endif // DEBUG_MODE
	}
	bool Skull::DetectTargetAction::ShouldChangeMover( const Skull &inst ) const
	{
		return false; // For now, do not transition to anything.
	}
	std::function<void()> Skull::DetectTargetAction::GetChangeStateMethod( Skull &inst ) const
	{
		return []() {}; // No op
	}
#if USE_IMGUI
	std::string Skull::DetectTargetAction::GetMoverName() const
	{
		return u8"�s�����m";
	}
#endif // USE_IMGUI

	void Skull::Shot::Init( Skull &inst )
	{
		inst.velocity = 0.0f;
	}
	void Skull::Shot::Update( Skull &inst, float elapsedTime, const Input &input )
	{
	#if DEBUG_MODE
		inst.velocity = ( input.wsTargetPos - inst.GetHitBox().WorldPosition() ).Unit();
	#endif // DEBUG_MODE
	}
	bool Skull::Shot::ShouldChangeMover( const Skull &inst ) const
	{
		return false; // For now, do not transition to anything.
	}
	std::function<void()> Skull::Shot::GetChangeStateMethod( Skull &inst ) const
	{
		return []() {}; // No op
	}
#if USE_IMGUI
	std::string Skull::Shot::GetMoverName() const
	{
		return u8"�V���b�g";
	}
#endif // USE_IMGUI

	void Skull::Jump::Init( Skull &inst )
	{
		inst.velocity = 0.0f;
	}
	void Skull::Jump::Update( Skull &inst, float elapsedTime, const Input &input )
	{
	#if DEBUG_MODE
		inst.velocity = ( input.wsTargetPos - inst.GetHitBox().WorldPosition() ).Unit();
	#endif // DEBUG_MODE
	}
	bool Skull::Jump::ShouldChangeMover( const Skull &inst ) const
	{
		return false; // For now, do not transition to anything.
	}
	std::function<void()> Skull::Jump::GetChangeStateMethod( Skull &inst ) const
	{
		return []() {}; // No op
	}
#if USE_IMGUI
	std::string Skull::Jump::GetMoverName() const
	{
		return u8"�W�����v";
	}
#endif // USE_IMGUI

	void Skull::Shield::Init( Skull &inst )
	{
		inst.velocity = 0.0f;
	}
	void Skull::Shield::Update( Skull &inst, float elapsedTime, const Input &input )
	{
	#if DEBUG_MODE
		inst.velocity = ( input.wsTargetPos - inst.GetHitBox().WorldPosition() ).Unit();
	#endif // DEBUG_MODE
	}
	bool Skull::Shield::ShouldChangeMover( const Skull &inst ) const
	{
		return false; // For now, do not transition to anything.
	}
	std::function<void()> Skull::Shield::GetChangeStateMethod( Skull &inst ) const
	{
		return []() {}; // No op
	}
#if USE_IMGUI
	std::string Skull::Shield::GetMoverName() const
	{
		return u8"�V�[���h";
	}
#endif // USE_IMGUI

	void Skull::Run::Init( Skull &inst )
	{
		inst.velocity = 0.0f;
	}
	void Skull::Run::Update( Skull &inst, float elapsedTime, const Input &input )
	{
	#if DEBUG_MODE
		inst.velocity = ( input.wsTargetPos - inst.GetHitBox().WorldPosition() ).Unit();
	#endif // DEBUG_MODE
	}
	bool Skull::Run::ShouldChangeMover( const Skull &inst ) const
	{
		return false; // For now, do not transition to anything.
	}
	std::function<void()> Skull::Run::GetChangeStateMethod( Skull &inst ) const
	{
		return []() {}; // No op
	}
#if USE_IMGUI
	std::string Skull::Run::GetMoverName() const
	{
		return u8"���s";
	}
#endif // USE_IMGUI


	void Skull::Init( const InitializeParam &parameter, int roomID, const Donya::Collision::Box3F &wsRoomArea )
	{
		Base::Init( parameter, roomID, wsRoomArea );

		AssignMover<AppearPerformance>();
	}
	void Skull::Update( float elapsedTime, const Input &input )
	{
		Base::Update( elapsedTime, input );

		if ( NowDead() ) { return; }
		// else

		if ( !pMover )
		{
			_ASSERT_EXPR( 0, L"Error: Mover does not exist!" );
			return;
		}
		// else

		pMover->Update( *this, elapsedTime, input );
		if ( pMover->ShouldChangeMover( *this ) )
		{
			auto ChangeState = pMover->GetChangeStateMethod( *this );
			ChangeState();
		}

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
		
		// TODO: Set a current motion index
		UpdateMotion( elapsedTime, NULL );
	}
	void Skull::PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		if ( !pMover )
		{
			_ASSERT_EXPR( 0, L"Error: Mover does not exist!" );
			return;
		}
		// else

		pMover->PhysicUpdate( *this, elapsedTime, solids );
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

		Base::ShowImGuiNode( u8"��ꕔ��" );

		if ( ImGui::TreeNode( u8"�h������" ) )
		{
			ImGui::Text( u8"���݂̃X�e�[�g�F%s", ( pMover ) ? pMover->GetMoverName().c_str() : u8"ERROR_STATE" );

			ImGui::TreePop();
		}

		ImGui::TreePop();
		return true;
	}
	void SkullParam::ShowImGuiNode()
	{
		ImGui::DragInt   ( u8"�����g�o",							&hp							);
		ImGui::DragFloat ( u8"�d��",								&gravity,			0.01f	);
		ImGui::DragFloat ( u8"�W�����v���鍂��",					&jumpHeight,		0.01f	);
		ImGui::DragFloat ( u8"�W�����v�ɂ�����b��",				&jumpTakeSeconds,	0.01f	);
		ImGui::DragFloat ( u8"���s���x",							&runSpeed,			0.01f	);
		ImGui::DragFloat3( u8"�����蔻��E�I�t�Z�b�g",			&hitBoxOffset.x,	0.01f	);
		ImGui::DragFloat3( u8"�����蔻��E�T�C�Y�i�������w��j",	&hitBoxSize.x,		0.01f	);
		ImGui::DragFloat3( u8"��炢����E�I�t�Z�b�g",			&hurtBoxOffset.x,	0.01f	);
		ImGui::DragFloat3( u8"��炢����E�T�C�Y�i�������w��j",	&hurtBoxSize.x,		0.01f	);
		touchDamage.ShowImGuiNode( u8"�ڐG�_���[�W�ݒ�" );
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
