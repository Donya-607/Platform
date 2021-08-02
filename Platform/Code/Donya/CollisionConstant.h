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
			Dynamic,	// Push other and be pushed by other.
			Kinematic,	// Push other and do not be pushed by other. Kinematic vs Kinematic is not occur any push/be pushed(works like Sensor).
			Sensor,		// Do not push nor be pushed with other.
		};
		constexpr const char *GetInteractionTypeName( InteractionType shape )
		{
			switch ( shape )
			{
			case InteractionType::Dynamic:
				return "Dynamic";
			case InteractionType::Kinematic:
				return "Kinematic";
			case InteractionType::Sensor:
				return "Sensor";
			default:
				break;
			}
			return "!ERROR_KIND!";
		}

		// Collider volume type
		enum class Shape
		{
			Empty,		// Position.xyz, Do not intersect to anything
			Point,		// Position.xyz
			AABB,		// Position.xyz + HalfSize.xyz
			Sphere,		// Position.xyz + Radius
		};
		constexpr const char *GetShapeName( Shape shape )
		{
			switch ( shape )
			{
			case Shape::Empty:
				return "Empty";
			case Shape::Point:
				return "Point";
			case Shape::AABB:
				return "AABB";
			case Shape::Sphere:
				return "Sphere";
			default:
				break;
			}
			return "!ERROR_KIND!";
		}

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
