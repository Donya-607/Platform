#ifndef INCLUDED_DONYA_COLLISION_BODY_H_
#define INCLUDED_DONYA_COLLISION_BODY_H_

#include <functional>			// For callback
#include <memory>
#include <unordered_map>		// unordered_multimap
#include <vector>

#include "CollisionCallback.h"	// DONYA_CALLBACK_ON_HIT_XXX
#include "CollisionConstant.h"	// HitResult
#include "CollisionShape.h"		// ShapeBase
#include "UniqueId.h"			// UniqueId<>
#include "Vector.h"				// Donya::Vector3

namespace Donya
{
	namespace Collision
	{
		// Substance of collision body.
		class Body
		{
		public:
			// Function pointer of callback of triggering an intersection(call it instead of CONTINUE version).
			// It function will be called before resolving(after call it, resolve will works).
			using EventFuncT_OnHitEnter		= std::function<void( DONYA_CALLBACK_ON_HIT_ENTER )>;
			// Function pointer of callback of continuing an intersection.
			// It function will be called before resolving(after call it, resolve will works).
			using EventFuncT_OnHitContinue	= std::function<void( DONYA_CALLBACK_ON_HIT_CONTINUE )>;
			// Function pointer of callback of end an intersection.
			using EventFuncT_OnHitExit		= std::function<void( DONYA_CALLBACK_ON_HIT_EXIT )>;
		private: // HACK: Should "mass" be member of ShapeBase?
			UniqueId<Body>		id;				// For identify the instance of body
			float				mass = 1.0f;	// Requirement:[mass > 0.0f] Use only when push/be pushed
			Donya::Vector3		position;		// Origin of a shapes
			std::vector<std::shared_ptr<ShapeBase>>
								shapePtrs;		// Registered shape list.
			bool				ignoreIntersection = false; // Forcely ignoring the intersection(do not hit regardless of the type)
		private:
			struct HitValue
			{
				std::shared_ptr<ShapeBase> myselfShape = nullptr;
				std::shared_ptr<ShapeBase> otherShape  = nullptr;
			public:
				bool IsEqual( const HitValue &rhs ) const
				{
					return	myselfShape == rhs.myselfShape &&
							otherShape == rhs.otherShape;
				}
			};
			// Key:Id, Value:Hit shape pair
			using HitBodyMap = std::unordered_multimap<UniqueIdType, const HitValue>;
			HitBodyMap								hitBodies;		// Store the intersected body's id and intersected shape pair(myself's and other's). For branch the calling methods when hit. OnHitEnter() or OnHitContinue() or OnHitExit()
			mutable HitBodyMap						insertRequests;	// Use for delay the inserting until next intersection
			mutable HitBodyMap						eraseRequests;	// Use for delay the erasing until next intersectionon
			std::vector<EventFuncT_OnHitEnter>		onHitHandlers_Enter;
			std::vector<EventFuncT_OnHitContinue>	onHitHandlers_Continue;
			std::vector<EventFuncT_OnHitExit>		onHitHandlers_Exit;
		public:
			// Register the clone of "pShape", so you can reuse the same shape.
			void RegisterShape( const std::shared_ptr<ShapeBase> &pShape );
			// Remove all registered shapes
			void RemoveAllShapes();
		public:
			void ResolveIntersectionWith( Body *pOther );
			void RegisterCallback_OnHitEnter		( const EventFuncT_OnHitEnter		&function );
			void RegisterCallback_OnHitContinue		( const EventFuncT_OnHitContinue	&function );
			void RegisterCallback_OnHitExit			( const EventFuncT_OnHitExit		&function );
			void InvokeCallbacks( DONYA_CALLBACK_ON_HIT_ENTER ) const;
			void UnregisterCallback_OnHitEnter		();
			void UnregisterCallback_OnHitContinue	();
			void UnregisterCallback_OnHitExit		();
			// It calls: UnregisterCallback_OnHitEnter(), UnregisterCallback_OnHitContinue(), UnregisterCallback_OnHitExit().
			void UnregisterCallback_All				();
		public:
			// Requirement:[mass > 0.0f]
			void SetMass( float mass );
			void SetPosition( const Donya::Vector3 &position );
			// If it is true, it forcely ignoring the intersection(regardless of the shape's type), the callbacks be not called also.
			void SetIgnoringIntersection( bool shouldIgnore );
			// It changes only myself, so it may break the uniquely identify system.
			void OverwriteId( UniqueIdType newId );
		public:
			bool			NowIgnoringIntersection() const;
			float			GetMass() const;
			Donya::Vector3	GetPosition() const;
			// Body's id. Do not compare it to other class's.
			UniqueIdType	GetId() const;
			const std::vector<std::shared_ptr<ShapeBase>> *GetRegisteredShapePointers() const;
			// Find a shape by an extra id that usage is defined by user, or nullptr if not found.
			std::shared_ptr<ShapeBase> FindShapePointerByExtraIdOrNullptr( int lookingExtraId ) const;
		private:
			void AcceptIdRequests();
		};
	}
}

#endif // !INCLUDED_DONYA_COLLISION_BODY_H_
