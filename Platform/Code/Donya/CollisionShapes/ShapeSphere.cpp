#include "ShapeSphere.h"

#include "Useful.h"		// SignBitF()

#include "ShapeAABB.h"
#include "ShapePoint.h"

namespace Donya
{
	namespace Collision
	{
		std::shared_ptr<ShapeBase> ShapeSphere::Generate( InteractionType interactionType, float radius, const Donya::Vector3 &posOffset )
		{
			std::shared_ptr<ShapeSphere> tmp = std::make_shared<ShapeSphere>();
			tmp->type	= interactionType;
			tmp->offset	= posOffset;
			tmp->radius	= radius;

			// Up-cast it
			return tmp;
		}
		std::shared_ptr<ShapeBase> ShapeSphere::Clone() const
		{
			std::shared_ptr<ShapeSphere> tmp = std::make_shared<ShapeSphere>( *this );
			// Up-cast it
			return tmp;
		}

		Donya::Vector3 ShapeSphere::GetAABBMin() const
		{
			return GetPosition() - GetRadius();
		}
		Donya::Vector3 ShapeSphere::GetAABBMax() const
		{
			return GetPosition() + GetRadius();
		}
		float ShapeSphere::CalcDistanceTo( const Donya::Vector3 &pt ) const
		{
			return ( pt - GetPosition() ).Length() - radius;
		}
		Donya::Vector3 ShapeSphere::FindClosestPointTo( const Donya::Vector3 &pt ) const
		{
			const Donya::Vector3 dir = ( pt - GetPosition() ).Unit();
			return dir * radius;
		}


		bool IsOverlappingToImpl( const ShapeSphere *pS, const ShapePoint *pP )
		{
			return pP->IsOverlappingWith( pS );
		}
		bool IsOverlappingToImpl( const ShapeSphere *pS, const ShapeAABB *pBox )
		{
			return pBox->IsOverlappingWith( pS );
		}
		bool IsOverlappingToImpl( const ShapeSphere *pA, const ShapeSphere *pB )
		{
			// Sphere vs Sphere is the same as: the Sphere that magnified by B's radius vs B's Point

			ShapeSphere magnifiedA = *pA;
			magnifiedA.radius += pB->radius;
			ShapePoint pointB;
			pointB.CopyBaseParameters( pB ); // For id management, we must use this method when copy

			return IsOverlappingToImpl( &magnifiedA, &pointB );
		}
		HitResult IntersectToImpl( const ShapeSphere *pS, const ShapePoint *pP )
		{
			HitResult result = pP->CalcIntersectionWith( pS );

			// These vectors are seen from point,
			// so reinterpret them to seen from box.
			result.surfaceNormal *= -1.0f;
			result.resolveVector *= -1.0f;

			return result;
		}
		HitResult IntersectToImpl( const ShapeSphere *pS, const ShapeAABB *pBox )
		{
			HitResult result = pBox->CalcIntersectionWith( pS );

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
			pointB.CopyBaseParameters( pB ); // For id management, we must use this method when copy

			HitResult result = IntersectToImpl( &magnifiedA, &pointB );
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
		bool ShapeSphere::IsOverlappingTo( const ShapeBase *pOther ) const
		{
			bool result = false;

			switch ( pOther->GetShapeKind() )
			{
			case Shape::Empty:
				// Can not intersect
				break;
			case Shape::Point:
				{
					const ShapePoint *pPoint = DownCastWithAssert<ShapePoint>( pOther );
					if ( pPoint )
					{
						result = IsOverlappingToImpl( this, pPoint );
					}
				}
				break;
			case Shape::AABB:
				{
					const ShapeAABB *pAABB = DownCastWithAssert<ShapeAABB>( pOther );
					if ( pAABB )
					{
						result = IsOverlappingToImpl( this, pAABB );
					}
				}
				break;
			case Shape::Sphere:
				{
					const ShapeSphere *pSphere = DownCastWithAssert<ShapeSphere>( pOther );
					if ( pSphere )
					{
						result = IsOverlappingToImpl( this, pSphere );
					}
				}
				break;
			default:
				_ASSERT_EXPR( 0, L"Failed: Not supported collision!" );
				break;
			}

			return result;
		}
		HitResult ShapeSphere::IntersectTo( const ShapeBase *pOther ) const
		{
			HitResult result;
			result.isHit = false;

			switch ( pOther->GetShapeKind() )
			{
			case Shape::Empty:
				// Can not intersect
				break;
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
