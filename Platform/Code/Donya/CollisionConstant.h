#ifndef INCLUDED_DONYA_COLLISION_CONSTANT_H_
#define INCLUDED_DONYA_COLLISION_CONSTANT_H_

#include "Vector.h" // Donya::Vector3

namespace Donya
{
	namespace Collision
	{
		// Interaction type with other
		enum class InteractionType
		{
			Dynamic,	// Movable. Push other and be pushed by other.
			Kinematic,	// Movable. Push other and do not be pushed by other. Kinematic vs Kinematic is not occur any push/be pushed(works like Sensor).
			Sensor,		// Movable. Do not push nor be pushed with other.
		};

		// Collider volume type
		enum class Shape
		{
			Empty,		// Position.xyz, Do not intersect to anything
			Point,		// Position.xyz
			AABB,		// Position.xyz + HalfSize.xyz
			Sphere,		// Position.xyz + Radius
		};

		// Collision test result, you must check "isHit" is true.
		// It depends on: http://noonat.github.io/intersect/
		struct HitResult
		{
			bool			isHit = false;	// Result of collision test, other members of HitResult are valid only when this flag is true
			Donya::Vector3	contactPoint;	// Contact point between two objects. This is there on the B's shape if the collision test was done as: Test( A, B );
			Donya::Vector3	surfaceNormal;	// Surface's normal at the contact point. This is there on the B's shape if the collision test was done as: Test( A, B );
			Donya::Vector3	resolveVector;	// Overlap amount between two objects, you can resolve the collision by add this vector
		};
	}
}

#endif // !INCLUDED_DONYA_COLLISION_CONSTANT_H_
