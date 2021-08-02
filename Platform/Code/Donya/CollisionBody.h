#ifndef INCLUDED_DONYA_COLLISION_BODY_H_
#define INCLUDED_DONYA_COLLISION_BODY_H_

#include <functional>			// For callback
#include <memory>
#include <unordered_map>		// unordered_multimap
#include <vector>

#undef max
#undef min
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/cereal.hpp>

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
		// It serializes(save/load) only members set by SetMass(), SetPosition(), AddShape(), and SetIgnoringIntersection(),
		// other members(UniqueId<Body>, registered callbacks, records of intersected bodies) will not changed.
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
			Donya::Vector3		position;		// Origin of shapes
			std::vector<std::shared_ptr<ShapeBase>>
								shapePtrs;		// Registered shape list.
			bool				ignoreIntersection = false; // Forcely ignoring the intersection(do not hit regardless of the type)
		protected:
			struct HitValue
			{
				std::shared_ptr<ShapeBase> myselfShape = nullptr;
				std::shared_ptr<ShapeBase> otherShape  = nullptr;
			public:
				static bool IsEqual( const HitValue &lhs, const HitValue &rhs )
				{
					return	lhs.myselfShape == rhs.myselfShape &&
							lhs.otherShape  == rhs.otherShape;
				}
			};
			// Key:Id, Value:Hit shape pair
			using HitBodyMap = std::unordered_multimap<UniqueIdType, HitValue>; // For to be copyable, I do not specify as const to the value.
			HitBodyMap								hitBodies;		// Store the intersected body's id and intersected shape pair(myself's and other's). For branch the calling methods when hit. OnHitEnter() or OnHitContinue() or OnHitExit()
			mutable HitBodyMap						insertRequests;	// Use for delay the inserting until next intersection
			mutable HitBodyMap						eraseRequests;	// Use for delay the erasing until next intersectionon
			std::vector<EventFuncT_OnHitEnter>		onHitHandlers_Enter;
			std::vector<EventFuncT_OnHitContinue>	onHitHandlers_Continue;
			std::vector<EventFuncT_OnHitExit>		onHitHandlers_Exit;
		public:
			Body()								= default;
			Body( const Body & )				= default;
			Body( Body && )						= default;
			Body &operator = ( const Body & )	= default;
			Body &operator = ( Body && )		= default;
			virtual ~Body()						= default;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( mass				),
					CEREAL_NVP( position			),
					CEREAL_NVP( shapePtrs			),
					CEREAL_NVP( ignoreIntersection	)
				);
				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		public:
			// Register the clone of "pShape", so you can reuse the same shape.
			void AddShape( const std::shared_ptr<ShapeBase> &pShape );
			// Remove all registered shapes
			void RemoveAllShapes();
		public:
			// First, check a intersection with "pOther",
			// Then, invoke the callbacks with above result,
			// Finally, resolve the penetration of each other.
			void ResolveIntersectionWith( Body *pOther );
		public:
			// The function will be called before resolving(after call it, resolve will works).
			void RegisterCallback_OnHitEnter		( const EventFuncT_OnHitEnter		&function );
			// The function will be called before resolving(after call it, resolve will works).
			void RegisterCallback_OnHitContinue		( const EventFuncT_OnHitContinue	&function );
			// The function will be called before resolving(after call it, resolve will works).
			void RegisterCallback_OnHitExit			( const EventFuncT_OnHitExit		&function );
			// It erases all callbacks registered by RegisterCallback_OnHitEnter()
			void UnregisterCallback_OnHitEnter		();
			// It erases all callbacks registered by RegisterCallback_OnHitContinue()
			void UnregisterCallback_OnHitContinue	();
			// It erases all callbacks registered by RegisterCallback_OnHitExit()
			void UnregisterCallback_OnHitExit		();
			// It calls: UnregisterCallback_OnHitEnter(), UnregisterCallback_OnHitContinue(), UnregisterCallback_OnHitExit().
			void UnregisterCallback_All				();
		private:
			// Invoke the callbacks branched with arguments
			void InvokeCallbacks( DONYA_CALLBACK_ON_HIT_ENTER ) const;
		public:
			// Overwrite the callbacks registered by RegisterCallback_OnHitEnter(), RegisterCallback_OnHitContinue(), RegisterCallback_OnHitExit() by "source".
			// The mine will be discarded, even if "source" have no callback or less than mine, the mine will be empty or lost than before.
			void OverwriteCallbacksBy( const Body &source );
			// Overwrite a records that used for branch the invoking of callback by "source".
			// The mine will be discarded, even if "source" have no callback or less than mine, the mine will be empty or lost than before.
			void OverwriteIntersectionRecordsBy( const Body &source );
		public:
			// Requirement:[mass > 0.0f]
			void SetMass( float mass );
			// Origin of shapes
			void SetPosition( const Donya::Vector3 &position );
			// If it is true, it forcely ignoring the intersection(regardless of the shape's type), the callbacks be not called also.
			void SetIgnoringIntersection( bool shouldIgnore );
			// It changes only myself, so it may break the uniquely identify system.
			void OverwriteId( UniqueIdType newId );
		public: 
			// If it is true, it forcely ignoring the intersection(regardless of the shape's type), the callbacks be not called also.
			bool			NowIgnoringIntersection() const;
			// [mass > 0.0f]
			float			GetMass() const;
			// Origin of shapes
			Donya::Vector3	GetPosition() const;
			// Body's id. Do not compare it to other class's.
			UniqueIdType	GetId() const;
		public:
			// Return the pointer of list added by AddShape()
			const std::vector<std::shared_ptr<ShapeBase>> *GetAddedShapePointers() const;
			// Find a shape by an extra id that usage is defined by user, or nullptr if not found.
			std::shared_ptr<ShapeBase> FindShapePointerByExtraIdOrNullptr( int lookingExtraId ) const;
		private:
			// Process the "insertRequests" and "eraseRequests"
			void AcceptIdRequests();
		};
	}
}

CEREAL_CLASS_VERSION( Donya::Collision::Body, 0 );

#endif // !INCLUDED_DONYA_COLLISION_BODY_H_
