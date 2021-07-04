#include "Collision.h"

#include <array>
#include <algorithm>	// Use std::find()
#include <deque>
#include <limits>		// Use std::numeric_limits<int>::max()

#include "Constant.h"
#include "Useful.h"		// Use IsZero()

#undef max
#undef min

namespace Donya
{
	namespace Collision
	{
		void Substance::RegisterShape( const std::shared_ptr<ShapeBase> &pShape )
		{
			if ( !pShape )
			{
				_ASSERT_EXPR( 0, L"Error: Shape is nullptr!" );
				return;
			}
			// else

			// Store the clone.
			// It be able to reuse same params, like this:
			// shared_ptr<ShapeXXX> ptr = XXX::Generate(...); // Make some shape
			// substance1.RegisterShape( ptr );
			// substance2.RegisterShape( ptr );
			shapePtrs.emplace_back( pShape->Clone() );
		}
		void Substance::RemoveAllShapes()
		{
			shapePtrs.clear();
		}
		void Substance::ResolveIntersectionWith( Substance *pOther )
		{
			// Update callbacks brancher "hitSubstanceIds"
			AcceptIdRequests();


			// Validation and prepare variables
			if ( !pOther ) { return; }
			// else
			const std::vector<std::shared_ptr<ShapeBase>> &otherShapePtrs = *pOther->GetRegisteredShapePointers();
			const size_t selfShapeCount = shapePtrs.size();
			const size_t otherShapeCount = otherShapePtrs.size();
			if ( !selfShapeCount || !otherShapeCount )
			{
				_ASSERT_EXPR( 0, L"Error: There is no shape!" );
				return;
			}
			// else
			const float massSum = GetMass() + pOther->GetMass();
			if ( massSum <= 0.0f )
			{
				_ASSERT_EXPR( 0, L"Error: The \"mass\" must be over than zero! [mass > 0.0]" );
				return;
			}
			// else
			const float selfMassPercent = 1.0f - ( GetMass() / massSum );
			const float otherMassPercent = 1.0f - selfMassPercent;


			// Do intersection between all patterns
			HitResult hitResult;
			std::shared_ptr<ShapeBase> selfShape;
			std::shared_ptr<ShapeBase> otherShape;
			for ( size_t i = 0; i < selfShapeCount; ++i )
			{
				selfShape = shapePtrs[i];
				if ( !selfShape )
				{
					_ASSERT_EXPR( 0, L"Error: Shape is nullptr!" );
					continue;
				}
				// else

				for ( size_t j = 0; j < otherShapeCount; ++j )
				{
					otherShape = otherShapePtrs[j];
					if ( !otherShape )
					{
						_ASSERT_EXPR( 0, L"Error: Shape is nullptr!" );
						continue;
					}
					// else


					// TODO: Consider shape type

					hitResult = selfShape->IntersectTo( otherShape.get() );
					InvokeCallbacks( *pOther, otherShape, hitResult );


					// HACK: I should consider a resolving order of shapes
					if ( hitResult.isHit )
					{
						this->position   += hitResult.resolveVector * selfMassPercent;
						pOther->position -= hitResult.resolveVector * otherMassPercent;

						// Apply the resolved position to all shapes
						this->SetPosition( this->position );
						pOther->SetPosition( pOther->position );
					}
				}
			}
		}
		void Substance::RegisterCallback_OnHitEnter( const EventFuncT_OnHitEnter &function )
		{
			onHitHandlers_Enter.push_back( function );
		}
		void Substance::RegisterCallback_OnHitContinue( const EventFuncT_OnHitContinue &function )
		{
			onHitHandlers_Continue.push_back( function );
		}
		void Substance::RegisterCallback_OnHitExit( const EventFuncT_OnHitExit &function )
		{
			onHitHandlers_Exit.push_back( function );
		}
		void Substance::InvokeCallbacks( DONYA_CALLBACK_ON_HIT_ENTER ) const
		{
			const UniqueIdType otherId = hitOther.GetId();
			const bool isKnown = hitSubstanceIds.count( otherId );

			if ( hitParam.isHit )
			{
				if ( isKnown )
				{
					// Continue
					
					for ( auto &OnHitContinue : onHitHandlers_Continue )
					{
						OnHitContinue( hitOther, pOtherShape, hitParam );
					}
				}
				else
				{
					// Trigger timing

					insertRequestIds.insert( otherId );
					for ( auto &OnHitEnter : onHitHandlers_Enter )
					{
						OnHitEnter( hitOther, pOtherShape, hitParam );
					}
				}
			}
			else if ( isKnown )
			{
				// Leave timing
				
				eraseRequestIds.erase( otherId );

				for ( auto &OnHitExit : onHitHandlers_Exit )
				{
					OnHitExit( otherId );
				}
			}

			// No hit, and No exit
		}
		void Substance::SetMass( float newMass )
		{
			if ( newMass <= 0.0f )
			{
				_ASSERT_EXPR( 0, L"Error: The \"mass\" must be over than zero! [mass > 0.0]" );
				return;
			}
			// else

			mass = newMass;
		}
		void Substance::SetPosition( const Donya::Vector3 &newPosition )
		{
			// Validation
			for ( auto &pShape : shapePtrs )
			{
				if ( !pShape )
				{
					_ASSERT_EXPR( 0, L"Error: Shape is nullptr!" );
					return;
				}
				// else
			}

			position = newPosition;
			for ( auto &pShape : shapePtrs )
			{
				pShape->position = newPosition;
			}
		}
		void Substance::OverwriteId( UniqueIdType newId )
		{
			// Use constructor for assign
			id = UniqueIdType{ newId };
		}
		float Substance::GetMass() const
		{
			return mass;
		}
		Donya::Vector3 Substance::GetPosition() const
		{
			return position;
		}
		UniqueIdType Substance::GetId() const
		{
			return id.Get();
		}
		const std::vector<std::shared_ptr<ShapeBase>> *Substance::GetRegisteredShapePointers() const
		{
			return &shapePtrs;
		}
		void Substance::AcceptIdRequests()
		{
			for ( const auto &id : insertRequestIds )
			{
				hitSubstanceIds.insert( id );
			}
			insertRequestIds.clear();

			for ( const auto &id : eraseRequestIds )
			{
				hitSubstanceIds.erase( id );
			}
			eraseRequestIds.clear();
		}


		// Why I choose std::deque:
		// I can loop all element fastly(cf: https://qiita.com/sileader/items/a40f9acf90fbda16af51#%E8%A6%81%E7%B4%A0%E3%81%AE%E8%B5%B0%E6%9F%BB%E7%AF%84%E5%9B%B2for).
		// Collider can take a reference directly(emplace_back() kills an iterator, but does not kills a directly reference-pointer. cf: https://cpprefjp.github.io/reference/deque/deque/emplace_back.html).
		static std::deque<Substance> bodies;

		void Collider::Generate( Collider *pOut )
		{
			if ( !pOut ) { return; }
			// else

			bodies.emplace_back();
			pOut->pReference = &bodies.back();
		}
		void Collider::Resolve()
		{
			auto itrEnd = bodies.end();
			for ( auto itrI = bodies.begin(); itrI != itrEnd; ++itrI )
			{
				Substance &a = *itrI;
				for ( auto itrJ = itrI + 1; itrJ != itrEnd; ++itrJ )
				{
					Substance &b = *itrJ;

					a.ResolveIntersectionWith( &b );
				}
			}
		}

		Collider::~Collider()
		{
			Destroy();
		}
		void Collider::Destroy()
		{
			if ( !pReference ) { return; }
			// else

			// Find referencing index
			const size_t invalidIndex = bodies.size();
			size_t index = 0;
			for ( const auto &it : bodies )
			{
				if ( &it == pReference )
				{
					break;
				}
				// else
				index++;
			}

			// Erase it
			if ( index < invalidIndex )
			{
				bodies.erase( bodies.begin() + index );
			}

			// Signify it has removed
			pReference = nullptr;
		}
		void Collider::RegisterShape( const std::shared_ptr<ShapeBase> &pShape )
		{
			if ( !pReference ) { return; }
			// else
			pReference->RegisterShape( pShape );
		}
		void Collider::RemoveAllShapes()
		{
			if ( !pReference ) { return; }
			// else
			pReference->RemoveAllShapes();
		}
		void Collider::SetMass( float mass )
		{
			// Error handling will process in Substance::SetMass()

			if ( !pReference ) { return; }
			// else
			pReference->SetMass( mass );
		}
		void Collider::SetPosition( const Donya::Vector3 &position )
		{
			if ( !pReference ) { return; }
			// else
			pReference->SetPosition( position );
		}
		float Collider::GetMass() const
		{
			if ( !pReference ) { return EPSILON; } // Not 0.0f for safe
			// else
			return pReference->GetMass();
		}
		Donya::Vector3 Collider::GetPosition() const
		{
			if ( !pReference ) { return Donya::Vector3::Zero(); }
			// else
			return pReference->GetPosition();
		}
		UniqueIdType Collider::GetColliderId() const
		{
			if ( !pReference ) { return scast<UniqueIdType>( 0 ); }
			// else
			return pReference->GetId();
		}
		const std::vector<std::shared_ptr<ShapeBase>> *Collider::GetRegisteredShapePointers() const
		{
			if ( !pReference ) { return nullptr; }
			// else
			return pReference->GetRegisteredShapePointers();
		}
	}



	namespace Collision
	{
		IDType GetUniqueID()
		{
			static IDType source = 0;
			constexpr int secondLargestValue = std::numeric_limits<int>::max() - 1;
			if ( secondLargestValue <= source ) { source = 0; }
			return source++;
		}

		bool IsInIgnoreList( const std::vector<IgnoreElement> &list, IDType id )
		{
			const auto result = std::find_if
			(
				list.begin(), list.end(),
				[&]( const IgnoreElement &element )
				{
					return element.ignoreID == id;
				}
			);
			return ( result != list.end() );
		}
		template<typename ColliderT, typename ColliderU>
		bool ShouldIgnore( const ColliderT &a, const ColliderU &b )
		{
			if ( a.ownerID != invalidID && a.ownerID == b.id ) { return true; }
			if ( b.ownerID != invalidID && b.ownerID == a.id ) { return true; }
			if ( IsInIgnoreList( a.ignoreList, b.id ) ) { return true; }
			if ( IsInIgnoreList( b.ignoreList, a.id ) ) { return true; }
			// else
			return false;
		}

		constexpr bool Within( int v, int min, int max )
		{
			return ( min <= v && v <= max );
		}

		template<typename Box, typename Coord>
		bool IsHitBox( const Box &a, const Coord &b, unsigned int dimension )
		{
			const auto aMin = a.Min();
			const auto aMax = a.Max();
			for ( unsigned int i = 0; i < dimension; ++i )
			{
				if ( !Within( b[i], aMin[i], aMax[i] ) )
				{
					return false;
				}
			}
			return true;
		}
		template<typename Box>
		bool IsHitBox( const Box &a, const Box &b, unsigned int dimension )
		{
			if ( ShouldIgnore( a, b ) ) { return false; }
			// else

			// Judge by "AABB of extended by other size" vs "position of other".
			Box ext = a;
			ext.size += b.size;
			return IsHitBox( ext, b.WorldPosition(), dimension );
		}

		template<typename Sphere, typename Coord>
		bool IsHitSphere( const Sphere &a, const Coord &b )
		{
			const Coord diff = b - a.WorldPosition();
			return diff.LengthSq() < ( a.radius * a.radius );
		}
		template<typename Sphere>
		bool IsHitSphere( const Sphere &a, const Sphere &b )
		{
			if ( ShouldIgnore( a, b ) ) { return false; }
			// else

			Sphere ext =  a;
			ext.radius += b.radius;
			return IsHitSphere( ext, b.WorldPosition() );
		}

		bool IsHit( const Donya::Int2 &a, const Box2 &b, bool consider )
		{
			if ( consider && !b.exist ) { return false; }
			// else
			return IsHitBox( b, a, 2U );
		}
		bool IsHit( const Box2 &a, const Donya::Int2 &b, bool consider )
		{
			if ( consider && !a.exist ) { return false; }
			// else
			return IsHitBox( a, b, 2U );
		}
		bool IsHit( const Box2 &a, const Box2 &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBox( a, b, 2U );
		}
		bool IsHit( const Donya::Int3 &a, const Box3 &b, bool consider )
		{
			if ( consider && !b.exist ) { return false; }
			// else
			return IsHitBox( b, a, 3U );
		}
		bool IsHit( const Box3 &a, const Donya::Int3 &b, bool consider )
		{
			if ( consider && !a.exist ) { return false; }
			// else
			return IsHitBox( a, b, 3U );
		}
		bool IsHit( const Box3 &a, const Box3 &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBox( a, b, 3U );
		}
		bool IsHit( const Donya::Vector2 &a, const Box2F &b, bool consider )
		{
			if ( consider && !b.exist ) { return false; }
			// else
			return IsHitBox( b, a, 2U );
		}
		bool IsHit( const Box2F &a, const Donya::Vector2 &b, bool consider )
		{
			if ( consider && !a.exist ) { return false; }
			// else
			return IsHitBox( a, b, 2U );
		}
		bool IsHit( const Box2F &a, const Box2F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBox( a, b, 2U );
		}
		bool IsHit( const Donya::Vector3 &a, const Box3F &b, bool consider )
		{
			if ( consider && !b.exist ) { return false; }
			// else
			return IsHitBox( b, a, 3U );
		}
		bool IsHit( const Box3F &a, const Donya::Vector3 &b, bool consider )
		{
			if ( consider && !a.exist ) { return false; }
			// else
			return IsHitBox( a, b, 3U );
		}
		bool IsHit( const Box3F &a, const Box3F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBox( a, b, 3U );
		}
		bool IsHit( const Donya::Int2 &a, const Sphere2 &b, bool consider )
		{
			if ( consider && !b.exist ) { return false; }
			// else
			return IsHitSphere( b, a );
		}
		bool IsHit( const Sphere2 &a, const Donya::Int2 &b, bool consider )
		{
			if ( consider && !a.exist ) { return false; }
			// else
			return IsHitSphere( a, b );
		}
		bool IsHit( const Sphere2 &a, const Sphere2 &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitSphere( a, b );
		}
		bool IsHit( const Donya::Int3 &a, const Sphere3 &b, bool consider )
		{
			if ( consider && !b.exist ) { return false; }
		// else
			return IsHitSphere( b, a );
		}
		bool IsHit( const Sphere3 &a, const Donya::Int3 &b, bool consider )
		{
			if ( consider && !a.exist ) { return false; }
			// else
			return IsHitSphere( a, b );
		}
		bool IsHit( const Sphere3 &a, const Sphere3 &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitSphere( a, b );
		}
		bool IsHit( const Donya::Vector2 &a, const Sphere2F &b, bool consider )
		{
			if ( consider && !b.exist ) { return false; }
		// else
			return IsHitSphere( b, a );
		}
		bool IsHit( const Sphere2F &a, const Donya::Vector2 &b, bool consider )
		{
			if ( consider && !a.exist ) { return false; }
			// else
			return IsHitSphere( a, b );
		}
		bool IsHit( const Sphere2F &a, const Sphere2F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitSphere( a, b );
		}
		bool IsHit( const Donya::Vector3 &a, const Sphere3F &b, bool consider )
		{
			if ( consider && !b.exist ) { return false; }
		// else
			return IsHitSphere( b, a );
		}
		bool IsHit( const Sphere3F &a, const Donya::Vector3 &b, bool consider )
		{
			if ( consider && !a.exist ) { return false; }
			// else
			return IsHitSphere( a, b );
		}
		bool IsHit( const Sphere3F &a, const Sphere3F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitSphere( a, b );
		}

		template<typename Box, typename Sphere>
		bool IsHitBoxSphere( const Box &a, const Sphere &b )
		{
			if ( ShouldIgnore( a, b ) ) { return false; }
			// else

			const auto wsPosB = b.WorldPosition();
			const auto closestPoint = FindClosestPoint( a, wsPosB );
			const auto diff   = wsPosB - closestPoint;
			const auto lenSq  = diff.LengthSq();
			const auto radSq  = b.radius * b.radius;
			return ( lenSq   <= radSq );
		}

		bool IsHit( const Box2 &a, const Sphere2 &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBoxSphere( a, b );
		}
		bool IsHit( const Sphere2 &a, const Box2 &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBoxSphere( b, a );
		}
		bool IsHit( const Box3 &a, const Sphere3 &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBoxSphere( a, b );
		}
		bool IsHit( const Sphere3 &a, const Box3 &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBoxSphere( b, a );
		}
		bool IsHit( const Box2F &a, const Sphere2F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBoxSphere( a, b );
		}
		bool IsHit( const Sphere2F &a, const Box2F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBoxSphere( b, a );
		}
		bool IsHit( const Box3F &a, const Sphere3F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBoxSphere( a, b );
		}
		bool IsHit( const Sphere3F &a, const Box3F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsHitBoxSphere( b, a );
		}

		template<typename Box>
		bool IsFullyIncludeBox( const Box &a, const Box &b, unsigned int dimension )
		{
			// All a.minimum must be less-eq than b.minimum
			const auto minA = a.Min();
			const auto minB = b.Min();
			for ( unsigned int i = 0; i < dimension; ++i )
			{
				if ( minB[i] < minA[i] ) { return false; }
			}
			
			// All a.maximum must be greater-eq than b.maximum
			const auto maxA = a.Max();
			const auto maxB = b.Max();
			for ( unsigned int i = 0; i < dimension; ++i )
			{
				if ( maxA[i] < maxB[i] ) { return false; }
			}

			return true;
		}
		template<typename Sphere>
		bool IsFullyIncludeSphere( const Sphere &a, const Sphere &b )
		{
			// The a can not be include the b
			if ( a.radius < b.radius ) { return false; }
			// else

			// The b's outer point must be near to a.position than a's outer point
			const auto wsPosA = a.WorldPosition();
			const auto wsPosB = b.WorldPosition();
			const auto distSq = ( wsPosB - wsPosA ).LengthSq();
			const auto outerDistSqA = a.radius * a.radius;
			const auto outerDistSqB = distSq + ( b.radius * b.radius );
			return ( outerDistSqB <= outerDistSqA );
		}
		template<typename Box, typename Sphere>
		bool IsFullyIncludeBoxSphere( const Box &a, const Sphere &b, unsigned int dimension )
		{
			const auto wsPosA = a.WorldPosition();
			const auto wsPosB = b.WorldPosition();

			if ( wsPosA == wsPosB )
			{
				// The a's smallest size must be greater-eq than b's radius
				unsigned int smallestAxis = 0;
				float smallestSize = FLT_MAX;
				for ( unsigned int i = 0; i < dimension; ++i )
				{
					float size = a.size[i];
					if (  size < smallestSize )
					{
						smallestAxis = i;
						smallestSize = size;
					}
				}

				return ( b.radius <= a.size[smallestAxis] );
			}
			// else

			// The a must include the b's outer point
			const auto diff = wsPosB - wsPosA;
			const auto outerDist = diff.Length() + b.radius;
			const auto outerPoint = wsPosA + ( diff.Unit() * outerDist );
			return IsHitBox( a, outerPoint, dimension );
		}
		template<typename Sphere, typename Box>
		bool IsFullyIncludeSphereBox( const Sphere &a, const Box &b )
		{
			// The a must contain the all vertices of the b
			// All vertices can be represented by min and max

			const auto min = b.Min();
			if ( !IsHit( a, min ) ) { return false; }
			// else
			const auto max = b.Max();
			if ( !IsHit( a, max ) ) { return false; }
			// else
			return true;
		}

		bool IsFullyInclude( const Box3F &a, const Box3F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsFullyIncludeBox( a, b, 3U );
		}
		bool IsFullyInclude( const Box3F &a, const Sphere3F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsFullyIncludeBoxSphere( a, b, 3U );
		}
		bool IsFullyInclude( const Sphere3F &a, const Box3F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsFullyIncludeSphereBox( a, b );
		}
		bool IsFullyInclude( const Sphere3F &a, const Sphere3F &b, bool consider )
		{
			if ( consider && ( !a.exist || !b.exist ) ) { return false; }
			// else
			return IsFullyIncludeSphere( a, b );
		}
		
		template<typename Box, typename Coord>
		Coord FindClosestPointBox( const Box &from, const Coord &to, unsigned int dimension )
		{
			const Coord fromMin = from.Min();
			const Coord fromMax = from.Max();
			Coord point{};
			for ( unsigned int i = 0; i < dimension; ++i )
			{
				point[i] = std::max( fromMin[i], std::min( fromMax[i], to[i] ) );
			}
			return point;
		}
		template<typename Sphere, typename Coord>
		Coord FindClosestPointSphere( const Sphere &from, const Coord &to )
		{
			const Coord v = to - from.WorldPosition();
			return from.WorldPosition() + ( v.Unit() * from.radius );
		}

		Donya::Int2 FindClosestPoint( const Donya::Int2 &from, const Box2 &to )
		{
			return FindClosestPointBox( to, from, 2U );
		}
		Donya::Int2 FindClosestPoint( const Box2 &from, const Donya::Int2 &to )
		{
			return FindClosestPointBox( from, to, 2U );
		}
		Donya::Int3 FindClosestPoint( const Donya::Int3 &from, const Box3 &to )
		{
			return FindClosestPointBox( to, from, 3U );
		}
		Donya::Int3 FindClosestPoint( const Box3 &from, const Donya::Int3 &to )
		{
			return FindClosestPointBox( from, to, 3U );
		}
		Donya::Vector2 FindClosestPoint( const Donya::Vector2 &from, const Box2F &to )
		{
			return FindClosestPointBox( to, from, 2U );
		}
		Donya::Vector2 FindClosestPoint( const Box2F &from, const Donya::Vector2 &to )
		{
			return FindClosestPointBox( from, to, 2U );
		}
		Donya::Vector3 FindClosestPoint( const Donya::Vector3 &from, const Box3F &to )
		{
			return FindClosestPointBox( to, from, 3U );
		}
		Donya::Vector3 FindClosestPoint( const Box3F &from, const Donya::Vector3 &to )
		{
			return FindClosestPointBox( from, to, 3U );
		}
		Donya::Int2 FindClosestPoint( const Donya::Int2 &from, const Sphere2 &to )
		{
			return FindClosestPointSphere( to, from );
		}
		Donya::Int2 FindClosestPoint( const Sphere2 &from, const Donya::Int2 &to )
		{
			return FindClosestPointSphere( from, to );
		}
		Donya::Int3 FindClosestPoint( const Donya::Int3 &from, const Sphere3 &to )
		{
			return FindClosestPointSphere( to, from );
		}
		Donya::Int3 FindClosestPoint( const Sphere3 &from, const Donya::Int3 &to )
		{
			return FindClosestPointSphere( from, to );
		}
		Donya::Vector2 FindClosestPoint( const Donya::Vector2 &from, const Sphere2F &to )
		{
			return FindClosestPointSphere( to, from );
		}
		Donya::Vector2 FindClosestPoint( const Sphere2F &from, const Donya::Vector2 &to )
		{
			return FindClosestPointSphere( from, to );
		}
		Donya::Vector3 FindClosestPoint( const Donya::Vector3 &from, const Sphere3F &to )
		{
			return FindClosestPointSphere( to, from );
		}
		Donya::Vector3 FindClosestPoint( const Sphere3F &from, const Donya::Vector3 &to )
		{
			return FindClosestPointSphere( from, to );
		}
	}

	void Box::Set			( float centerX, float centerY, float halfWidth, float halfHeight, bool isExist )
	{
		pos.x	= centerX;
		pos.y	= centerY;
		size.x	= halfWidth;
		size.y	= halfHeight;
		exist	= isExist;
	}
	bool Box::IsHitPoint	( const Box &L, const float &RX, const float &RY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		if	(
				L.pos.x - L.size.x	<= RX					&&	// X1 - W1	<= X2
				RX					<= L.pos.x + L.size.x	&&	// X2		<= X1 + W1
				L.pos.y - L.size.y	<= RY					&&	// Y1 - H1	<= Y2
				RY					<= L.pos.y + L.size.y		// Y2		<= Y1 + H2
			)
		{
			return true;
		}
		//else
		return false;
	}
	bool Box::IsHitPoint	( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const float &RX, const float &RY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		Box tmp
		{
			L.pos.x + LBoxScreenPosX,
			L.pos.y + LBoxScreenPosY,
			L.size.x,
			L.size.y,
			true
		};
		return Box::IsHitPoint( tmp, RX, RY );
	}
	bool Box::IsHitBox		( const Box &L, const Box &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		if	(
				L.pos.x - L.size.x <= R.pos.x + R.size.x	&&	// X1 - W1 < X2 + W2
				R.pos.x - R.size.x <= L.pos.x + L.size.x	&&	// X2 - W2 < X1 + W1
				L.pos.y - L.size.y <= R.pos.y + R.size.y	&&	// Y1 - H1 < Y2 + H2
				R.pos.y - R.size.y <= L.pos.y + L.size.y		// Y2 - H2 < Y1 + H1
			)
		{
			return true;
		}
		//else
		return false;
	}
	bool Box::IsHitBox		( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const Box &R, const float &RBoxScreenPosX, const float &RBoxScreenPosY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		Box tmpL
		{
			L.pos.x + LBoxScreenPosX,
			L.pos.y + LBoxScreenPosY,
			L.size.x,
			L.size.y,
			true
		};
		Box tmpR
		{
			R.pos.x + RBoxScreenPosX,
			R.pos.y + RBoxScreenPosY,
			R.size.x,
			R.size.y,
			true
		};
		return Box::IsHitBox( tmpL, tmpR );
	}
	bool Box::IsHitCircle	( const Box &L, const Circle &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		/*
		1.	Create a rectangle of magnification by the radius of the circle.
		2.	Judge the collision between that magnified rectangle and the center position of the circle.
		*/

		// VS The rectangle of vertically magnified.
		{
			Box tmp =
			{
				L.pos.x, 
				L.pos.y, 
				L.size.x, 
				L.size.y + R.radius, 
				true
			};
			if ( Box::IsHitPoint( tmp, R.pos.x, R.pos.y ) )
			{
				return true;
			}
		}
		// VS The rectangle of horizontally magnified.
		{
			Box tmp =
			{
				L.pos.x,
				L.pos.y,
				L.size.x + R.radius,
				L.size.y,
				true
			};
			if ( Box::IsHitPoint( tmp, R.pos.x, R.pos.y ) )
			{
				return true;
			}
		}
		// VS Center position of the circle from four-corners.
		{
			if	(
					Circle::IsHitPoint( R, L.pos.x - L.size.x,	L.pos.y - L.size.y )	||	// Left-Top
					Circle::IsHitPoint( R, L.pos.x - L.size.x,	L.pos.y + L.size.y )	||	// Left-Bottom
					Circle::IsHitPoint( R, L.pos.x + L.size.x,	L.pos.y - L.size.y )	||	// Right-Top
					Circle::IsHitPoint( R, L.pos.x + L.size.x,	L.pos.y + L.size.y )		// Right-Bottom
				)
			{
				return true;
			}
		}
		return false;
	}
	bool Box::IsHitCircle	( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const Circle &R, const float &RCircleScreenPosX, const float &RCircleScreenPosY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		Box tmpL
		{
			L.pos.x + LBoxScreenPosX,
			L.pos.y + LBoxScreenPosY,
			L.size.x,
			L.size.y,
			true
		};
		Circle tmpR
		{
			R.pos.x + RCircleScreenPosX,
			R.pos.y + RCircleScreenPosY,
			R.radius,
			true
		};
		return Box::IsHitCircle( tmpL, tmpR );
	}

	void Circle::Set		( float centerX, float centerY, float rad, bool isExist )
	{
		pos.x	= centerX;
		pos.y	= centerY;
		radius	= rad;
		exist	= isExist;
	}
	bool Circle::IsHitPoint	( const Circle &L, const float &RX, const float &RY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		return ( ( ( RX - L.pos.x ) * ( RX - L.pos.x ) ) + ( ( RY - L.pos.y ) * ( RY - L.pos.y ) ) < ( L.radius * L.radius ) );
	}
	bool Circle::IsHitPoint	( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const float &RX, const float &RY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		Circle tmp
		{
			L.pos.x + LCircleScreenPosX,
			L.pos.y + LCircleScreenPosY,
			L.radius,
			true
		};
		return Circle::IsHitPoint( tmp, RX, RY );
	}
	bool Circle::IsHitCircle( const Circle &L, const Circle &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		float dx  = R.pos.x - L.pos.x;
		float dy  = R.pos.y - L.pos.y;
		float rad = R.radius + L.radius;

		return ( ( dx * dx ) + ( dy * dy ) < rad * rad );
	}
	bool Circle::IsHitCircle( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const Circle &R, const float &RCircleScreenPosX, const float &RCircleScreenPosY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		Circle tmpL
		{
			L.pos.x + LCircleScreenPosX,
			L.pos.y + LCircleScreenPosY,
			L.radius,
			true
		};
		Circle tmpR
		{
			R.pos.x + RCircleScreenPosX,
			R.pos.y + RCircleScreenPosY,
			R.radius,
			true
		};
		return Circle::IsHitCircle( tmpL, tmpR );
	}
	bool Circle::IsHitBox	( const Circle &L, const Box &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		return Box::IsHitCircle( R, L );
	}
	bool Circle::IsHitBox	( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const Box &R, const float &RBoxScreenPosX, const float &RBoxScreenPosY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		return Box::IsHitCircle( R, RBoxScreenPosX, RBoxScreenPosY, L, LCircleScreenPosX, LCircleScreenPosY );
	}

	bool AABB::IsHitPoint	( const AABB &L, const Donya::Vector3 &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		constexpr size_t AXIS_COUNT = 3;

		const std::array<float, AXIS_COUNT> point{ R.x,			R.y,		R.z			};	// [0:X][1:Y][2:Z]
		const std::array<float, AXIS_COUNT> AABB { L.pos.x,		L.pos.y,	L.pos.z		};	// [0:X][1:Y][2:Z]
		const std::array<float, AXIS_COUNT> size { L.size.x,	L.size.y,	L.size.z	};	// [0:X][1:Y][2:Z]

		for ( size_t i = 0; i < AXIS_COUNT; ++i )
		{
			// If isn't [Min <= Point <= Max], judge to false.
			if ( point[i] < AABB[i] - size[i] ) { return false; }
			if ( point[i] > AABB[i] + size[i] ) { return false; }
		}

		return true;
	}
	bool AABB::IsHitAABB	( const AABB &L, const AABB &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		// Judge by "AABB of extended by size of other" vs "position of other".
	
		AABB extAABB = L;
		extAABB.size += R.size;	// If "R.size" is negative, maybe this method does not work.

		const Donya::Vector3 &point = R.pos;

		return IsHitPoint( extAABB, point );
	
	}
	bool AABB::IsHitSphere	( const AABB &L, const Sphere &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		auto CalcShortestDistanceSq = []( const AABB &L, const Donya::Vector3 &R )->float
		{
			// from http://marupeke296.com/COL_3D_No11_AABBvsPoint.html

			constexpr size_t AXIS_COUNT = 3;

			const std::array<float, AXIS_COUNT> point{ R.x,			R.y,		R.z			};	// [0:X][1:Y][2:Z]
			const std::array<float, AXIS_COUNT> AABB { L.pos.x,		L.pos.y,	L.pos.z		};	// [0:X][1:Y][2:Z]
			const std::array<float, AXIS_COUNT> size { L.size.x,	L.size.y,	L.size.z	};	// [0:X][1:Y][2:Z]

			std::array<float, AXIS_COUNT> distance{};

			float max{}, min{};
			for ( size_t i = 0; i < AXIS_COUNT; ++i )
			{
				max = AABB[i] + size[i];
				min = AABB[i] - size[i];

				if ( point[i] > max ) { distance[i] = point[i] - max; }
				if ( point[i] < min ) { distance[i] = point[i] - min; }
			}

			Donya::Vector3 v{ distance[0], distance[1], distance[2] };

			return v.LengthSq();
		};

		float distanceSq = CalcShortestDistanceSq( L, R.pos );
		return ( distanceSq < ( R.radius * R.radius ) );
	}

	bool Sphere::IsHitPoint	( const Sphere &L, const Donya::Vector3 &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		Donya::Vector3 d = R - L.pos;
		return ( d.LengthSq() < L.radius * L.radius );
	}
	bool Sphere::IsHitSphere( const Sphere &L, const Sphere &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		float r = L.radius + R.radius;
		Donya::Vector3 d = R.pos - L.pos;

		return ( d.LengthSq() < ( r * r ) );
	}
	bool Sphere::IsHitAABB	( const Sphere &L, const AABB &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		return AABB::IsHitSphere( R, L );
	}

	bool OBB::IsHitOBB( const OBB &L, const OBB &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		// see http://marupeke296.com/COL_3D_No13_OBBvsOBB.html

		/// <summary>
		/// Returns projecected line-segment length is half.
		/// </summary>
		auto CalcLengthOfProjectionLineSeg = []( const Donya::Vector3 &nSeparateAxis, const Donya::Vector3 &nDirX, const Donya::Vector3 &nDirY, const Donya::Vector3 *pDirZ/* If needed */ = nullptr )
		{
			float  projLengthX = fabsf( Dot( nSeparateAxis, nDirX ) );
			float  projLengthY = fabsf( Dot( nSeparateAxis, nDirY ) );
			float  projLengthZ = ( !pDirZ ) ? 0.0f : fabsf( Dot( nSeparateAxis, *pDirZ ) );
			return projLengthX + projLengthY + projLengthZ;
		};

		const Donya::Vector3 nDirLX = L.orientation.LocalRight(),	dirLX = nDirLX * L.size.x;
		const Donya::Vector3 nDirLY = L.orientation.LocalUp(),		dirLY = nDirLY * L.size.y;
		const Donya::Vector3 nDirLZ = L.orientation.LocalFront(),	dirLZ = nDirLZ * L.size.z;
		const Donya::Vector3 nDirRX = R.orientation.LocalRight(),	dirRX = nDirRX * R.size.x;
		const Donya::Vector3 nDirRY = R.orientation.LocalUp(),		dirRY = nDirRY * R.size.y;
		const Donya::Vector3 nDirRZ = R.orientation.LocalFront(),	dirRZ = nDirRZ * R.size.z;
		const Donya::Vector3 between = L.pos - R.pos;

		float projLenL{}, projLenR{}, distance{};
		auto IsOverlapping = [&projLenL, &projLenR, &distance]()->bool
		{
			return ( distance <= projLenL + projLenR ) ? true : false;
		};

		projLenL = dirLX.Length();
		projLenR = CalcLengthOfProjectionLineSeg( nDirLX, dirRX, dirRY, &dirRZ );
		distance = fabsf( Dot( between, nDirLX ) );
		if ( !IsOverlapping() ) { return false; }
		// else
		projLenL = dirLY.Length();
		projLenR = CalcLengthOfProjectionLineSeg( nDirLY, dirRX, dirRY, &dirRZ );
		distance = fabsf( Dot( between, nDirLY ) );
		if ( !IsOverlapping() ) { return false; }
		// else
		projLenL = dirLZ.Length();
		projLenR = CalcLengthOfProjectionLineSeg( nDirLZ, dirRX, dirRY, &dirRZ );
		distance = fabsf( Dot( between, nDirLZ ) );
		if ( !IsOverlapping() ) { return false; }
		// else

		projLenL = CalcLengthOfProjectionLineSeg( nDirRX, dirLX, dirLY, &dirLZ );
		projLenR = dirRX.Length();
		distance = fabsf( Dot( between, nDirRX ) );
		if ( !IsOverlapping() ) { return false; }
		// else
		projLenL = CalcLengthOfProjectionLineSeg( nDirRY, dirLX, dirLY, &dirLZ );
		projLenR = dirRY.Length();
		distance = fabsf( Dot( between, nDirRY ) );
		if ( !IsOverlapping() ) { return false; }
		// else
		projLenL = CalcLengthOfProjectionLineSeg( nDirRZ, dirLX, dirLY, &dirLZ );
		projLenR = dirRZ.Length();
		distance = fabsf( Dot( between, nDirRZ ) );
		if ( !IsOverlapping() ) { return false; }
		// else

		Donya::Vector3 cross{}; // Should I normalize this at every result?

		cross = Cross( nDirLX, nDirRX );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLY, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRY, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLX, nDirRY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLY, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLX, nDirRZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLY, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRY );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee

		cross = Cross( nDirLY, nDirRX );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRY, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLY, nDirRY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLY, nDirRZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRY );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee

		cross = Cross( nDirLZ, nDirRX );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRY, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLZ, nDirRY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLZ, nDirRZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRY );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee

		return true;
	}

	float CalcShortestDistance( const OBB &L, const Donya::Vector3 &point )
	{
		// see http://marupeke296.com/COL_3D_No12_OBBvsPoint.html

		enum IntAxis { X = 0, Y, Z, END };
		auto ExtractHalfLength  = []( const OBB &OBB, IntAxis axisIndex )->float
		{
			_ASSERT_EXPR( 0 <= axisIndex && axisIndex < END, L"Error : Passed index out of range !" );

			switch ( axisIndex )
			{
			case X: return OBB.size.x; // break;
			case Y: return OBB.size.y; // break;
			case Z: return OBB.size.z; // break;
			default: break;
			}

			return NULL;
		};
		auto ExtractRotatedAxis = []( const OBB &OBB, IntAxis axisIndex )->Donya::Vector3
		{
			_ASSERT_EXPR( 0 <= axisIndex && axisIndex < END, L"Error : Passed index out of range !" );

			constexpr std::array<Donya::Vector3, END> AXES
			{
				Donya::Vector3::Right(),
				Donya::Vector3::Up(),
				Donya::Vector3::Front()
			};

			Donya::Vector3 axis = AXES[axisIndex];
			return OBB.orientation.RotateVector( axis );
		};

		Donya::Vector3 protrudedSum{};

		for ( int i = 0; i < END; ++i )
		{
			float halfLength = ExtractHalfLength( L, static_cast<IntAxis>( i ) );
			if (  halfLength <= 0 ) { continue; } // This case can not calculate.
			// else

			Donya::Vector3 axis = ExtractRotatedAxis( L, static_cast<IntAxis>( i ) );

			float magni = Donya::Vector3::Dot( ( point - L.pos ), axis ) / halfLength;
			magni = fabsf( magni );

			if ( 1.0f < magni )
			{
				protrudedSum += ( 1.0f - magni ) * halfLength * axis;
			}
		}

		return protrudedSum.Length();
	}
	bool OBB::IsHitSphere( const OBB &L, const Sphere &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		float distance = CalcShortestDistance( L, R.pos );
		return ( distance <= R.radius ) ? true : false;
	}

	RayIntersectResult CalcIntersectionPoint( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, const AABB &box )
	{
		const auto &a  = rayStart;
		const auto &b  = rayEnd;
		// const auto dir = ( rayEnd - rayStart ).Unit();
		const auto dir = ( rayEnd - rayStart );
		const Donya::Vector3 invDir
		{
			1.0f / dir.x,
			1.0f / dir.y,
			1.0f / dir.z,
		};
		const bool isInf[]
		{
			std::isinf( invDir.x ),
			std::isinf( invDir.y ),
			std::isinf( invDir.z )
		};

		const auto boxMin = box.pos - box.size;
		const auto boxMax = box.pos + box.size;
		
		Donya::Vector3 tMin;
		Donya::Vector3 tMax;
		for ( int i = 0; i < 3; ++i )
		{
			tMin[i] = ( boxMin[i] - a[i] ) * invDir[i];
			tMax[i] = ( boxMax[i] - a[i] ) * invDir[i];

			if ( tMax[i] < tMin[i] )
			{
				auto tmp = tMax[i];
				tMax[i] = tMin[i];
				tMin[i] = tmp;
			}
		}

		float greatestMin = -FLT_MAX;
		float smallestMax = +FLT_MAX;
		for ( int i = 0; i < 3; ++i )
		{
			if ( isInf[i] ) { continue; }
			// else

			greatestMin = std::max( tMin[i], greatestMin );
			smallestMax = std::min( tMax[i], smallestMax );
		}

		bool isIntersect = true;
		for ( int i = 0; i < 3; ++i )
		{
			if ( tMax[i] < 0.0f			) { isIntersect = false; break; }
			if ( tMax[i] < tMin[i]		) { isIntersect = false; break; }
			if ( tMax[i] < greatestMin	) { isIntersect = false; break; }
			if ( smallestMax < tMin[i]	) { isIntersect = false; break; }
		}

		RayIntersectResult  result;
		result.isIntersect  = isIntersect;
		result.intersection = a + dir * greatestMin;
		if ( greatestMin < 0.0f )
		{
			// The ray start inside the box, so the first intersection is max.
			result.intersection = a + dir * smallestMax;
		}

		result.normal = 0.0f;
		Donya::Vector3 diff = result.intersection - box.pos;
		diff.x = fabsf( diff.x );
		diff.y = fabsf( diff.y );
		diff.z = fabsf( diff.z );
		if ( diff.y < diff.x && diff.z < diff.x ) { result.normal.x = scast<float>( Donya::SignBit( diff.x ) ); }
		if ( diff.x < diff.y && diff.z < diff.y ) { result.normal.y = scast<float>( Donya::SignBit( diff.y ) ); }
		if ( diff.x < diff.z && diff.y < diff.z ) { result.normal.z = scast<float>( Donya::SignBit( diff.z ) ); }

		return result;
	}
	RayIntersectResult CalcIntersectionPoint( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, const Plane &plane )
	{
		// see http://www.sousakuba.com/Programming/gs_plane_line_intersect.html

		const Donya::Vector3 planePoint = plane.distance * plane.normal;
		const Donya::Vector3 vPS = rayStart - planePoint;
		const Donya::Vector3 vPE = rayEnd   - planePoint;
		const float	dotPS  = Dot( vPS, plane.normal );
		const float	dotPE  = Dot( vPE, plane.normal );
		const int	signPS = Donya::SignBit( dotPS );
		const int	signPE = Donya::SignBit( dotPE );

		if ( signPS == signPE )
		{
			if ( signPS == 0 )
			{
				// The two edges place onto the plane, so I can't decide the intersection point.
				// We returns start point temporary.
				RayIntersectResult tmp;
				tmp.intersection	= rayStart;
				tmp.normal			= Donya::Vector3::Zero();
				tmp.isIntersect		= true;
				return tmp;
			}
			else
			{
				// The two edges place to the same side by the plane.
				RayIntersectResult notHit{};
				notHit.isIntersect = false;
				return notHit;
			}
		}
		// else

		RayIntersectResult result{};
		result.isIntersect = true;

		const float absDotPS = fabsf( dotPS );
		const float absDotPE = fabsf( dotPE );
		const float distPercent = absDotPS / ( absDotPS + absDotPE + EPSILON );
		const Donya::Vector3 sourceRay = rayEnd - rayStart;
		result.intersection = rayStart + ( sourceRay * distPercent );
		
		const float dotRN = Dot( sourceRay.Unit(), plane.normal );
		const float normalSign = ( dotRN < 0.0f ) ? -1.0f : 1.0f;
		result.normal = plane.normal * normalSign;

		return result;
	}

	bool operator == ( const Box	&L, const Box		&R )
	{
		if ( !IsZero( L.pos.x  - R.pos.x  ) )	{ return false; }
		if ( !IsZero( L.pos.y  - R.pos.y  ) )	{ return false; }
		if ( !IsZero( L.size.x - R.size.x ) )	{ return false; }
		if ( !IsZero( L.size.y - R.size.y ) )	{ return false; }
		if ( L.exist != R.exist )					{ return false; }
		// else
		return true;
	}
	bool operator == ( const Circle	&L, const Circle	&R )
	{
		if ( !IsZero( L.pos.x  - R.pos.x  ) )	{ return false; }
		if ( !IsZero( L.pos.y  - R.pos.y  ) )	{ return false; }
		if ( !IsZero( L.radius - R.radius ) )	{ return false; }
		if ( L.exist != R.exist )					{ return false; }
		// else
		return true;
	}
	bool operator == ( const AABB	&L, const AABB		&R )
	{
		if ( ! ( L.pos  - R.pos  ).IsZero() )		{ return false; }
		if ( ! ( L.size - R.size ).IsZero() )		{ return false; }
		if ( L.exist != R.exist )					{ return false; }
		// else
		return true;
	}
	bool operator == ( const Sphere	&L, const Sphere	&R )
	{
		if ( ! ( L.pos  - R.pos  ).IsZero() )		{ return false; }
		if ( !IsZero( L.radius - R.radius ) )	{ return false; }
		if ( L.exist != R.exist )					{ return false; }
		// else
		return true;
	}
	bool operator == ( const OBB	&L, const OBB		&R )
	{
		if ( !( L.pos  - R.pos  ).IsZero() ) { return false; }
		if ( !( L.size - R.size ).IsZero() ) { return false; }
		if ( !L.orientation.IsSameRotation( R.orientation ) ) { return false; }
		if ( L.exist != R.exist ) { return false; }
		// else
		return true;
	}
}
