#include "StateMachine.h"

class Owner
{
public:
	using StateBaseT = StateMachine::IState<Owner>;
public:
	int hp = 0;
	int max = 0;
	std::unique_ptr<StateBaseT> pBase;
public:
	void Init();
public:
	void UpdateState()
	{
		if ( !pBase ) { return; }
		// else

		float elapsedTime = 0.016666f;
		pBase->Update( *this, elapsedTime );
		StateMachine::ChangeIfNeeded( &pBase, *this );
	}
};


using namespace StateMachine;
#include "Donya/Template.h"

class Idle : public Owner::StateBaseT
{
	void Init( Owner &instance ) override
	{
		instance.hp = 1;
		instance.max = 2;
	}
	void Uninit( Owner &instance ) override
	{

	}
	void Update( Owner &instance, float elapsedTime ) override
	{

	}
	void PhysicUpdate( Owner &instance, float elapsedTime ) override
	{

	}
	void Draw( Owner &instance ) override
	{

	}
public:
	std::unique_ptr<Owner::StateBaseT> MakeNextStateOrNull( Owner &instance ) override
	{
		return nullptr;
	}
};
class Attack : public Owner::StateBaseT
{
	void Init( Owner &instance ) override
	{
		instance.hp = 65535;
		instance.max = 1001001001;
	}
	void Uninit( Owner &instance ) override
	{

	}
	void Update( Owner &instance, float elapsedTime ) override
	{

	}
	void PhysicUpdate( Owner &instance, float elapsedTime ) override
	{

	}
	void Draw( Owner &instance ) override
	{

	}
public:
	std::unique_ptr<Owner::StateBaseT> MakeNextStateOrNull( Owner &instance ) override
	{
		return std::make_unique<Idle>();
	}
};


void Owner::Init()
{
	UpdateState();

	pBase->Init( *this );
}
