#ifndef INCLUDED_DONYA_COLLISION_SHAPE_BASE_H_
#define INCLUDED_DONYA_COLLISION_SHAPE_BASE_H_

#include "CollisionConstant.h"	// InteractionType, Shape, HitResult
#include "Vector.h"				// Donya::Vector3

namespace Donya
{
	namespace Collision
	{
		// Shape of collision.
		// This class has an interaction type, a positions, and a size(in derived class).
		// And this also has an extra id(default is zero), every code of library does not touch it, so an user can use it as freely(e.g. user's enum value, hash of shape name, etc.).
		// And this class provides detecting an intersection method(impl in derived class).
		class ShapeBase
		{
		public:
			InteractionType		type = InteractionType::Dynamic;
			Donya::Vector3		position;	// The origin, or owner's position
			Donya::Vector3		offset;		// Offset from origin
			int					extraId = 0;// User's space
			bool				ignoreIntersection = false; // Forcely ignoring the intersection(do not hit regardless of the type)
		public:
			ShapeBase()									= default;
			ShapeBase( const ShapeBase & )				= default;
			ShapeBase( ShapeBase && )					= default;
			ShapeBase &operator = ( const ShapeBase & )	= default;
			ShapeBase &operator = ( ShapeBase && )		= default;
			virtual ~ShapeBase()						= default;
		public:
			// Copy the Base members(interaction-type, position, offset, and extra id).
			void CopyBaseParameters( const ShapeBase *pShape )
			{
				if ( !pShape || pShape == this ) { return; }
				// else

				type		= pShape->type;
				position	= pShape->position;
				offset		= pShape->offset;
				extraId		= pShape->extraId;
			}
		public:
			// Interaction type
			InteractionType	GetType()		const { return type; }
			// The world space position(origin + offset) of shape
			Donya::Vector3	GetPosition()	const { return position + offset; }
			// The origin of shape
			Donya::Vector3	GetOrigin()		const { return position; }
			// The offset of shape
			Donya::Vector3	GetOffset()		const { return offset; }
			// Usage is defined by an user
			int				GetId()			const { return extraId; }
		public:
			// For branch a collision detection method
			virtual Shape			GetShapeKind()		const = 0;
			// Minimum side xyz of AABB that wraps the myself
			virtual Donya::Vector3	GetAABBMin()		const = 0;
			// Maximum side xyz of AABB that wraps the myself
			virtual Donya::Vector3	GetAABBMax()		const = 0;
			// 
			virtual std::shared_ptr<ShapeBase> Clone()	const = 0;
		public:
			// Distance between myself and point("pt")
			virtual float			CalcDistanceTo( const Donya::Vector3 &pt )		const = 0;
			// The closest point to "pt" within my shape
			virtual Donya::Vector3	FindClosestPointTo( const Donya::Vector3 &pt )	const = 0;
		protected:
			// Calc the resolver of intersection with "pShape"
			virtual HitResult		IntersectTo( const ShapeBase *pOtherShape )		const = 0;

			// TODO: Provide a collision detection method(just detection only, do not calc a resolver)
		public:
			// Calc the resolver of intersection with "pShape"
			HitResult CalcIntersectionWith( const ShapeBase *pOtherShape ) const
			{
				HitResult noHit;
				noHit.isHit = false;

				if ( !pOtherShape ) { return noHit; }
				// else

				const bool shouldIgnore = ( ignoreIntersection || pOtherShape->ignoreIntersection );
				return ( shouldIgnore ) ? noHit : IntersectTo( pOtherShape );
			}
		};
	}
}

#endif // !INCLUDED_DONYA_COLLISION_SHAPE_BASE_H_
