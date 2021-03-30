#pragma once

#include <memory>

namespace StateMachine
{
	template<class StateOwnerT>
	class IState
	{
	public:
		IState() = default;
		virtual ~IState() = default;
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
		virtual std::unique_ptr<IState<StateOwnerT>> MakeNextStateOrNull( StateOwnerT &ownerInstance ) = 0;
	};

	template<class StateOwnerT>
	void ChangeIfNeeded( std::unique_ptr<IState<StateOwnerT>> *pOperatee, StateOwnerT &ownerInstance )
	{
		if ( !pOperatee ) { return; }
		// else

		using Ptr = std::unique_ptr<IState<StateOwnerT>>;
		Ptr &operatee = *pOperatee;

		Ptr nextState = operatee->MakeNextStateOrNull( ownerInstance );
		if ( !nextState ) { return; }
		// else

		operatee->Uninit( ownerInstance );
		operatee.reset();

		operatee = std::move( nextState );
		operatee->Init( ownerInstance );
	}
}
