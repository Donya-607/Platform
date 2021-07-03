#include "ShapeAABB.h"

#include "Useful.h"		// SignBitF()

#include "ShapePoint.h"
#include "ShapeSphere.h"

namespace Donya
{
	namespace Collision
	{
		Donya::Vector3 ShapeAABB::GetAABBMin() const
		{
			return position - size;
		}
		Donya::Vector3 ShapeAABB::GetAABBMax() const
		{
			return position + size;
		}
		Donya::Vector3 ShapeAABB::FindClosestPointTo( const Donya::Vector3 &pt ) const
		{
			const Donya::Vector3 min = GetAABBMin();
			const Donya::Vector3 max = GetAABBMax();

			Donya::Vector3 closest;
			closest.x = std::max( min.x, std::min( max.x, pt.x ) );
			closest.y = std::max( min.y, std::min( max.y, pt.y ) );
			closest.z = std::max( min.z, std::min( max.z, pt.z ) );
			return closest;
		}


		HitResult IntersectToImpl( const ShapeAABB *pBox, const ShapePoint *pP )
		{
			HitResult result = pP->IntersectTo( pBox );

			// These vectors are seen from point,
			// so reinterpret them to seen from box.
			result.surfaceNormal *= -1.0f;
			result.resolveVector *= -1.0f;

			return result;
		}
		HitResult IntersectToImpl( const ShapeAABB *pA, const ShapeAABB *pB )
		{
			// By http://noonat.github.io/intersect/#:~:text=time%3B%0A%20%20%20%20return%20hit%3B%0A%20%20%7D-,AABB%20VS%20AABB,-This%20test%20uses
			
			// AABB vs AABB is very similar to Point vs AABB,
			// so it uses method is mainly Point vs AABB version.

			ShapeAABB magnifiedA = *pA;
			magnifiedA.size += pB->size;
			ShapePoint pointB;
			pointB.position = pB->position;

			HitResult result = magnifiedA.IntersectTo( &pointB );
			if ( result.isHit )
			{
				// Now, the contact point is there on magnifiedA's edge.
				// So we back it to *pB's edge.
				const Donya::Vector3 excessAddition = result.surfaceNormal.Product( pB->size );
				result.contactPoint -= excessAddition;
			}

			return result;
		}
		HitResult IntersectToImpl( const ShapeAABB *pBox, const ShapeSphere *pS )
		{
			// Ref https://www.gamedev.net/forums/topic/552068-solved-penetration-depth-for-sphereaabb-and-sphereobb-intersection-correct-but/552068/

			HitResult result;
			result.isHit = false;

			// Compare the length between sphere radius and the closest point to sphere center within box
			const Donya::Vector3 closest = pBox->FindClosestPointTo( pS->position );
			const Donya::Vector3 deltaSP = closest - pS->position; // Sphere center -> Cloest point
			if ( deltaSP.LengthSq() <= pS->radius * pS->radius )
			{
				result.isHit = true;
				// Contact point is there on the other's edge
				result.contactPoint = pS->position + ( deltaSP.Unit() * pS->radius );

				// Normal is at contact surface,
				result.surfaceNormal = deltaSP;
				// but we should calc it if sphere center is there on the edge.
				if ( deltaSP.IsZero() )
				{
					// Sphere center -> Box center
					result.surfaceNormal = pBox->position - pS->position;
				}
				result.surfaceNormal.Normalize();

				// Resolver is the distance except for the radius
				const float penetration = ( pBox->position - pS->position ).Length() - pS->radius;
				result.resolveVector = result.surfaceNormal * penetration;
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
		HitResult ShapeAABB::IntersectTo( const ShapeBase *pOther ) const
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
