#ifndef INCLUDED_DONYA_COLLISION_SHAPE_BASE_H_
#define INCLUDED_DONYA_COLLISION_SHAPE_BASE_H_

#undef max
#undef min
#include <cereal/cereal.hpp>

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
		// It serializes(save/load) all members.
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
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( type				),
					CEREAL_NVP( position			),
					CEREAL_NVP( offset				),
					CEREAL_NVP( extraId				),
					CEREAL_NVP( ignoreIntersection	)
				);
				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		public:
			// Copy the all Base members(interaction-type, position, offset, extra id, and ignoring intersection).
			void CopyBaseParameters( const ShapeBase *pShape )
			{
				if ( !pShape || pShape == this ) { return; }
				// else

				type				= pShape->type;
				position			= pShape->position;
				offset				= pShape->offset;
				extraId				= pShape->extraId;
				ignoreIntersection	= pShape->ignoreIntersection;
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

CEREAL_CLASS_VERSION( Donya::Collision::ShapeBase, 0 )
CEREAL_REGISTER_TYPE( Donya::Collision::ShapeBase )
// If you inherit the ShapeBase, please follow below.
/*
Note: How to register a derived shape class to Cereal

1.Include cereal/types/polymorphic.hpp, like this:
#include <cereal/types/polymorphic.hpp>

2.In cereal's serialize(),
  include cereal::base_class<ShapeBase> as first of passing arguments of archive(),
  like this:
archive
(
	cereal::base_class<Donya::Collision::ShapeBase>( this ),
	// some derived class's members here...
);

3.Register the relation by using macros, like this:
CEREAL_CLASS_VERSION( Donya::Collision::ShapeXXX, n )
CEREAL_REGISTER_TYPE( Donya::Collision::ShapeXXX )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Donya::Collision::ShapeBase, Donya::Collision::ShapeXXX )
*/

#endif // !INCLUDED_DONYA_COLLISION_SHAPE_BASE_H_
