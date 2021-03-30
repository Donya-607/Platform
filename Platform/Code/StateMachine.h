#pragma once

#include <memory>

template<class StateOwnerT>
class IStateMachine
{

public:
	IStateMachine() = default;
	virtual ~IStateMachine() = default;
public:
	virtual void Init( StateOwnerT &ownerInstance ) = 0;
	/// <summary>
	/// It will be called when after MakeNextStateOrNull()
	/// </summary>
	virtual void Uninit( StateOwnerT &ownerInstance ) = 0;
	virtual void Update( StateOwnerT &ownerInstance, float elapsedTime ) = 0;
	virtual void PhysicUpdate( StateOwnerT &ownerInstance, float elapsedTime ) = 0;
	virtual void Draw( StateOwnerT &ownerInstance ) = 0;
public:
	/// <summary>
	/// Returns nullptr means: do not change now
	/// </summary>
	virtual std::unique_ptr<IStateMachine<StateOwnerT>> MakeNextStateOrNull( StateOwnerT &ownerInstance ) = 0;
};

template<class StateOwnerT>
void ChangeStateTo( std::unique_ptr<IStateMachine<StateOwnerT>> *pOperatee, StateOwnerT &ownerInstance )
{
	if ( !pOperatee ) { return; }
	// else

	using Ptr = std::unique_ptr<IStateMachine<StateOwnerT>>;
	Ptr &operatee = *pOperatee;

	Ptr nextState = operatee->GetNextStateOrNull( ownerInstance );
	if ( !nextState ) { return; }
	// else

	operatee->Uninit( ownerInstance );
	operatee.reset();

	operatee = std::move( nextState );
	operatee->Init( ownerInstance );
}

