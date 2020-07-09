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
		return u8"登場";
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
		return u8"行動検知";
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
		return u8"ショット";
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
		return u8"ジャンプ";
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
		return u8"シールド";
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
		return u8"走行";
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

		Base::ShowImGuiNode( u8"基底部分" );

		if ( ImGui::TreeNode( u8"派生部分" ) )
		{
			ImGui::Text( u8"現在のステート：%s", ( pMover ) ? pMover->GetMoverName().c_str() : u8"ERROR_STATE" );

			ImGui::TreePop();
		}

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
