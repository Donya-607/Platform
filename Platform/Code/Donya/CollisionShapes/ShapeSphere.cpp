#include "ShapeSphere.h"

#include "Useful.h"		// SignBitF()

#include "ShapeAABB.h"
#include "ShapePoint.h"

namespace Donya
{
	namespace Collision
	{
		Donya::Vector3 ShapeSphere::GetAABBMin() const
		{
			return position - radius;
		}
		Donya::Vector3 ShapeSphere::GetAABBMax() const
		{
			return position + radius;
		}
		Donya::Vector3 ShapeSphere::FindClosestPointTo( const Donya::Vector3 &pt ) const
		{
			const Donya::Vector3 dir = ( pt - position ).Unit();
			return dir * radius;
		}


		HitResult IntersectToImpl( const ShapeSphere *pS, const ShapePoint *pP )
		{
			HitResult result = pP->IntersectTo( pS );

			// These vectors are seen from point,
			// so reinterpret them to seen from box.
			result.surfaceNormal *= -1.0f;
			result.resolveVector *= -1.0f;

			return result;
		}
		HitResult IntersectToImpl( const ShapeSphere *pS, const ShapeAABB *pBox )
		{
			HitResult result = pBox->IntersectTo( pS );

			// These vectors are seen from point,
			// so reinterpret them to seen from box.
			result.surfaceNormal *= -1.0f;
			result.resolveVector *= -1.0f;

			return result;
		}
		HitResult IntersectToImpl( const ShapeSphere *pA, const ShapeSphere *pB )
		{
			// Sphere vs Sphere is very similar to Point vs Sphere,
			// so it uses method is mainly Point vs Sphere version.

			ShapeSphere magnifiedA = *pA;
			magnifiedA.radius += pB->radius;
			ShapePoint pointB;
			pointB.position = pB->position;

			HitResult result = magnifiedA.IntersectTo( &pointB );
			if ( result.isHit )
			{
				// Now, the contact point is there on magnifiedA's edge.
				// So we back it to *pB's edge.
				const Donya::Vector3 excessAddition = result.surfaceNormal * pB->radius;
				result.contactPoint -= excessAddition;
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
		HitResult ShapeSphere::IntersectTo( const ShapeBase *pOther ) const
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
