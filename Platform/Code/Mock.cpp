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

		StateMachine::ChangeIfNeeded<Owner>( &pBase, *this );
	}
};


using namespace StateMachine;
#include "Donya/Template.h"

class Idle : IState<Owner>
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
	std::unique_ptr<IState<Owner>> MakeNextStateOrNull( Owner &instance ) override
	{
		return nullptr;
	}
};
class Attack : IState<Owner>
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
	std::unique_ptr<IState<Owner>> MakeNextStateOrNull( Owner &instance ) override
	{
		// Donya::TypeDetective<Idle> hogeT;
		// return std::make_unique<Owner>();
		// return std::make_unique<Idle>();
		
		return std::make_unique<IState<Owner>>();
		return std::make_unique<Idle>();

		// return std::make_unique<Idle>();
		return nullptr;
	}
};


void Owner::Init()
{
	UpdateState();

	pBase->Init( *this );
}
