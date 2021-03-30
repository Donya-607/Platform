#include "StateMachine.h"

class Foo
{
public:
	using StateBaseT = IStateMachine<Foo>;
public:
	int hp = 0;
	int max = 0;
	std::unique_ptr<StateBaseT> pBase;
public:
};

class Hoge : Foo::StateBaseT
{
	void Init( Foo &instance ) override
	{
		instance.hp = 1;
		instance.max = 2;
	}
	void Uninit( Foo &instance ) override
	{

	}
	void Update( Foo &instance, float elapsedTime ) override
	{

	}
	void PhysicUpdate( Foo &instance, float elapsedTime ) override
	{

	}
	void Draw( Foo &instance ) override
	{

	}
public:
	std::unique_ptr<Foo::StateBaseT> MakeNextStateOrNull( Foo &instance ) override
	{

	}
};
