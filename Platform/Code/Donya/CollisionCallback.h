#ifndef INCLUDED_DONYA_COLLISION_CALLBACK_H_
#define INCLUDED_DONYA_COLLISION_CALLBACK_H_

#include <memory>				// shared_ptr

#include "CollisionConstant.h"	// HitResult
#include "CollisionShape.h"		// ShapeBase


// Please include "CollisionBody.h" before this header.
// This header avoiding the circular including
// (it is safe until use because there is only #define in here).


// Signature of callback of triggering an intersection(call it instead of CONTINUE version), use like this:
// void OnHitEnter( DONYA_CALLBACK_ON_HIT_ENTER );
// It callback will be called before resolving(after call it, resolve will works).
// ENTER will be called when new pair of intersection was detected. It will also be called between another shape in the same bodies.
#define DONYA_CALLBACK_ON_HIT_ENTER \
	const Donya::Collision::Body &hitOther, /* Intersected target */ \
	const std::shared_ptr<Donya::Collision::ShapeBase> &pHitOtherShape, /* Intersected shape of target's (not nullptr) */ \
	const Donya::Collision::Body &hitMyself, /* Intersected body of myself */ \
	const std::shared_ptr<Donya::Collision::ShapeBase> &pHitMyselfShape, /* Intersected shape of myself (not nullptr) */ \
	const Donya::Collision::HitResult &hitParam /* Intersect description */

// Signature of callback of continuing an intersection, use like this:
// void OnHitContinue( DONYA_CALLBACK_ON_HIT_CONTINUE );
// It callback will be called before resolving(after call it, resolve will works).
// CONTINUE will be called when the detected intersection's pair is same. If intersected bodies are same but the shapes are not same, the callback will call ENTER version.
#define DONYA_CALLBACK_ON_HIT_CONTINUE \
	DONYA_CALLBACK_ON_HIT_ENTER // Same signature

// Signature of callback of end an intersection, use like this:
// void OnHitExit( DONYA_CALLBACK_ON_HIT_EXIT );
// EXIT will be called when a known intersection is not detected continuously. EXIT will also be called as the ended intersection's shape pair if intersecting shapes are changed in intersecting bodies still are same.
#define DONYA_CALLBACK_ON_HIT_EXIT \
	const Donya::Collision::Body &leaveOther, /* Previously intersected target */ \
	const std::shared_ptr<Donya::Collision::ShapeBase> &pLeaveOtherShape, /* Previously intersected shape of target's (not nullptr) */ \
	const Donya::Collision::Body &leaveMyself, /* Previously intersected body of myself */ \
	const std::shared_ptr<Donya::Collision::ShapeBase> &pLeaveMyselfShape /* Previously intersected shape of myself (not nullptr) */

#endif // !INCLUDED_DONYA_COLLISION_CALLBACK_H_
