#include "CollisionBody.h"

#include "CollisionCallback.h" // Enable the macros

namespace Donya
{
	namespace Collision
	{
		void Body::RegisterShape( const std::shared_ptr<ShapeBase> &pShape )
		{
			if ( !pShape )
			{
				_ASSERT_EXPR( 0, L"Error: Shape is nullptr!" );
				return;
			}
			// else

			// Store the clone.
			// It be able to reuse same params, like this:
			// shared_ptr<ShapeXXX> ptr = XXX::Generate(...); // Make some shape
			// body1.RegisterShape( ptr );
			// body2.RegisterShape( ptr );
			shapePtrs.emplace_back( pShape->Clone() );
		}
		void Body::RemoveAllShapes()
		{
			shapePtrs.clear();
		}
		void Body::ResolveIntersectionWith( Body *pOther )
		{
			// Update "hitBodyIds", brancher of callbacks
			this->AcceptIdRequests();
			if ( !pOther ) { return; }
			// else
			pOther->AcceptIdRequests();


			// Ignore intersection if requested
			if ( ignoreIntersection || pOther->ignoreIntersection ) { return; }
			// else


			// Validation and prepare variables
			const std::vector<std::shared_ptr<ShapeBase>> &otherShapePtrs = *pOther->GetRegisteredShapePointers();
			const size_t selfShapeCount = shapePtrs.size();
			const size_t otherShapeCount = otherShapePtrs.size();
			if ( !selfShapeCount || !otherShapeCount ) { return; }
			// else
			const float massSum = GetMass() + pOther->GetMass();
			if ( massSum <= 0.0f )
			{
				_ASSERT_EXPR( 0, L"Error: The \"mass\" must be over than zero! [mass > 0.0]" );
				return;
			}
			// else
			const float selfMassPercent  = 1.0f - ( GetMass() / massSum );
			const float otherMassPercent = 1.0f - selfMassPercent;
			constexpr float slightlyMagnifier = 1.0001f; // Collision will react even if edgeA == edgeB, so it more resolves to be edgeA != edgeB


			// Do intersection between all patterns
			HitResult hitResult;
			HitResult invHitResult; // Inverse the intersection viewer, for other's callback
			std::shared_ptr<ShapeBase> selfShape;
			std::shared_ptr<ShapeBase> otherShape;
			bool isSensor			= false; // True if either is sensor
			bool isKinematicSelf	= false;
			bool isKinematicOther	= false;
			bool dontResolve		= false; // True when either is sensor, or both are kinematic.
			for ( size_t i = 0; i < selfShapeCount; ++i )
			{
				selfShape = shapePtrs[i];
				if ( !selfShape )
				{
					_ASSERT_EXPR( 0, L"Error: Shape is nullptr!" );
					continue;
				}
				// else
				if ( selfShape->GetShapeKind() == Shape::Empty ) { continue; }
				// else

				isSensor		= ( selfShape->GetType() == InteractionType::Sensor );
				isKinematicSelf	= ( selfShape->GetType() == InteractionType::Kinematic );


				for ( size_t j = 0; j < otherShapeCount; ++j )
				{
					otherShape = otherShapePtrs[j];
					if ( !otherShape )
					{
						_ASSERT_EXPR( 0, L"Error: Shape is nullptr!" );
						continue;
					}
					// else
					if ( otherShape->GetShapeKind() == Shape::Empty ) { continue; }
					// else

					isSensor			= isSensor || ( otherShape->GetType() == InteractionType::Sensor );
					isKinematicOther	= ( otherShape->GetType() == InteractionType::Kinematic );


					// Calc the intersection resolver
					hitResult = selfShape->CalcIntersectionWith( otherShape.get() );
					hitResult.resolveVector *= slightlyMagnifier;
					invHitResult = hitResult;
					invHitResult.surfaceNormal *= -1.0f;
					invHitResult.resolveVector *= -1.0f;


					// Invoke callbacks before resolve(even in the case of do not resolve)
					this->InvokeCallbacks( *pOther, otherShape, *this, selfShape, hitResult );
					pOther->InvokeCallbacks( *this, selfShape, *pOther, otherShape, invHitResult );


					// Early return if I do not need to resolve
					dontResolve = isSensor || ( isKinematicSelf && isKinematicOther ) || !hitResult.isHit;
					if ( dontResolve ) { continue; }
					// else


					// Resolve them
					// Note: I do not consider an order of resolving.
					// So resolving order is depending on registered order of shapes.

					if ( isKinematicSelf )
					{
						// Other receives whole resolver
						pOther->position -= hitResult.resolveVector;
						pOther->SetPosition( pOther->position );
					}
					else
					if ( isKinematicOther )
					{
						// Self receives whole resolver
						this->position   += hitResult.resolveVector;
						this->SetPosition( this->position );
					}
					else
					{
						// Each other receives resolver calced by mass ratio

						this->position   += hitResult.resolveVector * selfMassPercent;
						this->SetPosition( this->position );

						pOther->position -= hitResult.resolveVector * otherMassPercent;
						pOther->SetPosition( pOther->position );
					}
				}
			}
		}
		void Body::RegisterCallback_OnHitEnter( const EventFuncT_OnHitEnter &function )
		{
			onHitHandlers_Enter.push_back( function );
		}
		void Body::RegisterCallback_OnHitContinue( const EventFuncT_OnHitContinue &function )
		{
			onHitHandlers_Continue.push_back( function );
		}
		void Body::RegisterCallback_OnHitExit( const EventFuncT_OnHitExit &function )
		{
			onHitHandlers_Exit.push_back( function );
		}
		void Body::InvokeCallbacks( DONYA_CALLBACK_ON_HIT_ENTER ) const
		{
			const UniqueIdType otherId = hitOther.GetId();
			const auto memorizedOthers = hitBodies.equal_range( otherId );

			HitValue nowHit{};
			nowHit.myselfShape = pHitMyselfShape;
			nowHit.otherShape  = pHitOtherShape;

			bool isKnown = false;
			// Check the other is known pair
			for ( auto itr = memorizedOthers.first; itr != hitBodies.cend() && itr != memorizedOthers.second; ++itr )
			{
				if ( itr->second.IsEqual( nowHit ) )
				{
					isKnown = true;
					break;
				}
			}

			// Branch to Enter or Continue or Exit
			if ( hitParam.isHit )
			{
				if ( isKnown )
				{
					// Continue
					
					for ( auto &OnHitContinue : onHitHandlers_Continue )
					{
						OnHitContinue( hitOther, pHitOtherShape, hitMyself, pHitMyselfShape, hitParam );
					}
				}
				else
				{
					// Trigger timing

					insertRequests.insert( std::make_pair( otherId, nowHit ) );
					for ( auto &OnHitEnter : onHitHandlers_Enter )
					{
						OnHitEnter( hitOther, pHitOtherShape, hitMyself, pHitMyselfShape, hitParam );
					}
				}
			}
			else if ( isKnown )
			{
				// Leave timing
				
				eraseRequests.insert( std::make_pair( otherId, nowHit ) );

				for ( auto &OnHitExit : onHitHandlers_Exit )
				{
					OnHitExit( hitOther, pHitOtherShape, hitMyself, pHitMyselfShape );
				}
			}

			// No hit, and No exit
		}
		void Body::UnregisterCallback_OnHitEnter()
		{
			onHitHandlers_Enter.clear();
		}
		void Body::UnregisterCallback_OnHitContinue()
		{
			onHitHandlers_Continue.clear();
		}
		void Body::UnregisterCallback_OnHitExit()
		{
			onHitHandlers_Exit.clear();
		}
		void Body::UnregisterCallback_All()
		{
			UnregisterCallback_OnHitEnter();
			UnregisterCallback_OnHitContinue();
			UnregisterCallback_OnHitExit();
		}
		void Body::SetMass( float newMass )
		{
			if ( newMass <= 0.0f )
			{
				_ASSERT_EXPR( 0, L"Error: The \"mass\" must be over than zero! [mass > 0.0]" );
				return;
			}
			// else

			mass = newMass;
		}
		void Body::SetPosition( const Donya::Vector3 &newPosition )
		{
			// Validation
			for ( auto &pShape : shapePtrs )
			{
				if ( !pShape )
				{
					_ASSERT_EXPR( 0, L"Error: Shape is nullptr!" );
					return;
				}
				// else
			}

			// Updation
			position = newPosition;
			for ( auto &pShape : shapePtrs )
			{
				pShape->position = newPosition;
			}
		}
		void Body::SetIgnoringIntersection( bool shouldIgnore )
		{
			ignoreIntersection = shouldIgnore;
		}
		void Body::OverwriteId( UniqueIdType newId )
		{
			id.OverwriteBy( newId );
		}
		bool Body::NowIgnoringIntersection() const
		{
			return ignoreIntersection;
		}
		float Body::GetMass() const
		{
			return mass;
		}
		Donya::Vector3 Body::GetPosition() const
		{
			return position;
		}
		UniqueIdType Body::GetId() const
		{
			return id.Get();
		}
		const std::vector<std::shared_ptr<ShapeBase>> *Body::GetRegisteredShapePointers() const
		{
			return &shapePtrs;
		}
		std::shared_ptr<ShapeBase> Body::FindShapePointerByExtraIdOrNullptr( int lookingExtraId ) const
		{
			for ( const auto &pIt : shapePtrs )
			{
				if ( !pIt ) { continue; }
				// else

				if ( pIt->extraId == lookingExtraId )
				{
					return pIt;
				}
			}

			return nullptr;
		}
		void Body::AcceptIdRequests()
		{
			for ( const auto &pair : insertRequests )
			{
				hitBodies.insert( pair );
			}
			insertRequests.clear();

			for ( const auto &pair : eraseRequests )
			{
				// Erase the same values to "pair"

				// Loop of the bodies that id is same to pair's
				const auto foundRange = hitBodies.equal_range( pair.first );
				for ( auto itr = foundRange.first; itr != hitBodies.end() && itr != foundRange.second; /* advance "itr" in inner loop */ )
				{
					// Erase one by one completely same(UniqueId and HitValue are same) body
					if ( itr->second.IsEqual( pair.second ) )
					{
						itr = hitBodies.erase( itr );
						continue;
					}
					// else
					++itr;
				}
			}
			eraseRequests.clear();
		}
	}
}
