#include "ShapePoint.h"

#include "Useful.h"		// SignBitF()

#include "ShapeAABB.h"
#include "ShapeSphere.h"

namespace Donya
{
	namespace Collision
	{
		Donya::Vector3 ShapePoint::GetAABBMin() const
		{
			return position;
		}
		Donya::Vector3 ShapePoint::GetAABBMax() const
		{
			return position;
		}
		Donya::Vector3 ShapePoint::FindClosestPointTo( const Donya::Vector3 &pt ) const
		{
			return position;
		}


		HitResult IntersectToImpl( const ShapePoint *pA, const ShapePoint *pB )
		{
			HitResult result;
			result.isHit = false;

			if ( pA->position == pB->position )
			{
				result.isHit = true;
				result.contactPoint = pA->position;
				// Point does not have a normal, so assign temporary direction
				result.surfaceNormal = Donya::Vector3::Up();
				// Smallest amount that makes pA != pB
				result.resolveVector = result.surfaceNormal * EPSILON;
			}

			return result;
		}
		HitResult IntersectToImpl( const ShapePoint *pP, const ShapeAABB *pBox )
		{
			// By http://noonat.github.io/intersect/#:~:text=vs%20swept%20AABB.-,AABB%20VS%20POINT,-This%20test%20is

			HitResult result;
			result.isHit = false;


			// Find the overlap in all axis
			const Donya::Vector3 delta = pP->position - pBox->position;
			const Donya::Vector3 overlap = pBox->size - delta.Abs();
			if ( overlap.x <= 0.0f ) { return result; }
			if ( overlap.y <= 0.0f ) { return result; }
			if ( overlap.z <= 0.0f ) { return result; }
			// else


			// Collide here
			result.isHit = true;


			// Find smallest axis
			size_t axis = 0;
			if ( overlap.y < overlap.x && overlap.y < overlap.z )
			{
				axis = 1;
			}
			if ( overlap.z < overlap.x && overlap.z < overlap.y )
			{
				axis = 2;
			}


			// Make intersect parameters

			const float sign = SignBitF( delta[axis] );
			// Contact point will be the nearest edge of the other
			result.contactPoint = pP->position;
			result.contactPoint[axis] = pBox->position[axis] + ( pBox->size[axis] * sign );
			// Normal will be (1,0,0) or (0,1,0) or (0,0,1)
			result.surfaceNormal = Donya::Vector3::Zero();
			result.surfaceNormal[axis] = sign;
			// Resolver is overlapping area of normal axis
			result.resolveVector = Donya::Vector3::Zero();
			result.resolveVector[axis] = overlap[axis] * sign;

			return result;
		}
		HitResult IntersectToImpl( const ShapePoint *pP, const ShapeSphere *pS )
		{
			HitResult result;
			result.isHit = false;

			// Sphere center -> Point
			const Donya::Vector3 deltaSP = pP->position - pS->position;
			const float distance = deltaSP.Length();
			if ( distance <= pS->radius )
			{
				result.isHit = true;
				// Contact point is there on the other's edge
				result.contactPoint  = pS->position + ( deltaSP.Unit() * pS->radius );
				// Normal is surface's at contact point
				result.surfaceNormal = deltaSP.Unit();
				result.resolveVector = result.surfaceNormal * ( pS->radius - distance );
			}

			return result;
		}


		template<class ShapeType>
		const ShapeType *DownCastWithAssert( const ShapeBase *pBase )
		{
			const ShapeType *pDerived = dynamic_cast<const ShapeType *>( pBase );
			_ASSERT_EXPR( pDerived, L"Error: Invalid dynamic_cast!" );
			return pDerived;
		}
		HitResult ShapePoint::IntersectTo( const ShapeBase *pOther ) const
		{
			HitResult result;
			result.isHit = false;

			switch ( pOther->GetShape() )
			{
			case Shape::Point:
				{
					const ShapePoint *pPoint = DownCastWithAssert<ShapePoint>( pOther );
					if ( pPoint )
					{
						result = IntersectToImpl( this, pPoint );
					}
				}
				break;
			case Shape::AABB:
				{
					const ShapeAABB *pAABB = DownCastWithAssert<ShapeAABB>( pOther );
					if ( pAABB )
					{
						result = IntersectToImpl( this, pAABB );
					}
				}
				break;
			case Shape::Sphere:
				{
					const ShapeSphere *pSphere = DownCastWithAssert<ShapeSphere>( pOther );
					if ( pSphere )
					{
						result = IntersectToImpl( this, pSphere );
					}
				}
				break;
			default:
				_ASSERT_EXPR( 0, L"Failed: Not supported collision!" );
				break;
			}

			return result;
		}
	}
}
