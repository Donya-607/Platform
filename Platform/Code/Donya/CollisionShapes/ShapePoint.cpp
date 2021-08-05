#include "ShapePoint.h"

#include "Useful.h"		// SignBitF()

#include "ShapeAABB.h"
#include "ShapeSphere.h"

namespace Donya
{
	namespace Collision
	{
		std::shared_ptr<ShapeBase> ShapePoint::Generate( InteractionType interactionType, const Donya::Vector3 &posOffset )
		{
			std::shared_ptr<ShapePoint> tmp = std::make_shared<ShapePoint>();
			tmp->type	= interactionType;
			tmp->offset	= posOffset;

			// Up-cast it
			return tmp;
		}
		std::shared_ptr<ShapeBase> ShapePoint::Clone() const
		{
			std::shared_ptr<ShapePoint> tmp = std::make_shared<ShapePoint>( *this );
			// Up-cast it
			return tmp;
		}

		Donya::Vector3 ShapePoint::GetAABBMin() const
		{
			return GetPosition();
		}
		Donya::Vector3 ShapePoint::GetAABBMax() const
		{
			return GetPosition();
		}
		float ShapePoint::CalcDistanceTo( const Donya::Vector3 &pt ) const
		{
			return ( pt - GetPosition() ).Length();
		}
		Donya::Vector3 ShapePoint::FindClosestPointTo( const Donya::Vector3 &pt ) const
		{
			return GetPosition();
		}


		bool IsOverlappingToImpl( const ShapePoint *pA, const ShapePoint *pB )
		{
			return ( pA->GetPosition() == pB->GetPosition() );
		}
		bool IsOverlappingToImpl( const ShapePoint *pP, const ShapeAABB *pBox )
		{
			// Consider in Box's local space
			Donya::Vector3 localPoint = pP->GetPosition() - pBox->GetPosition();
			if ( pBox->size.x < fabsf( localPoint.x ) ) { return false; }
			if ( pBox->size.y < fabsf( localPoint.y ) ) { return false; }
			if ( pBox->size.z < fabsf( localPoint.z ) ) { return false; }
			// else
			return true;
		}
		bool IsOverlappingToImpl( const ShapePoint *pP, const ShapeSphere *pS )
		{
			const float distSq   = ( pP->GetPosition() - pS->GetPosition() ).LengthSq();
			const float radiusSq = pS->radius * pS->radius;
			return ( distSq <= radiusSq );
		}
		HitResult IntersectToImpl( const ShapePoint *pA, const ShapePoint *pB )
		{
			HitResult result;
			result.isHit = false;

			if ( IsOverlappingToImpl( pA, pB ) )
			{
				result.isHit = true;
				result.contactPoint = pB->GetPosition();
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
			const Donya::Vector3 delta = pP->GetPosition() - pBox->GetPosition();
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
			result.contactPoint = pP->GetPosition();
			result.contactPoint[axis] = pBox->GetPosition()[axis] + ( pBox->size[axis] * sign );
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
			const Donya::Vector3 deltaSP = pP->GetPosition() - pS->GetPosition();
			const float distance = deltaSP.Length();
			if ( distance <= pS->radius )
			{
				result.isHit = true;
				// Contact point is there on the other's edge
				result.contactPoint  = pS->GetPosition() + ( deltaSP.Unit() * pS->radius );
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
		bool ShapePoint::IsOverlappingTo( const ShapeBase *pOther ) const
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
		HitResult ShapePoint::IntersectTo( const ShapeBase *pOther ) const
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
