#ifndef INCLUDED_DONYA_COLLISION_H_
#define INCLUDED_DONYA_COLLISION_H_

#include <algorithm>	// Use std::remove_if
#include <cstdint>		// Use for std::uint32_t
#include <functional>
#include <vector>

#undef max
#undef min
#include <cereal/cereal.hpp>

#include "Quaternion.h"
#include "UniqueId.h"
#include "Vector.h"

namespace Donya
{
	namespace Collision
	{
		// Interaction type with other
		enum class InteractionType
		{
			Dynamic,	// Movable. Push other and be pushed by other.
			Kinematic,	// Movable. Push other and do not be pushed by other.
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

		// Shape of collision.
		// This class has a interaction type, a positions, and a size(in derived class).
		// And this class provides detecting an intersection method(impl in derived class).
		class ShapeBase
		{
		public:
			InteractionType	type = InteractionType::Dynamic;
			Donya::Vector3	position;	// The origin, or owner's position
			Donya::Vector3	offset;		// Offset from origin
		public:
			ShapeBase()									= default;
			ShapeBase( const ShapeBase & )				= default;
			ShapeBase( ShapeBase && )					= default;
			ShapeBase &operator = ( const ShapeBase & )	= default;
			ShapeBase &operator = ( ShapeBase && )		= default;
			virtual ~ShapeBase()						= default;
		public:
			virtual std::shared_ptr<ShapeBase> Clone() const = 0;
		public:
			// Interaction type
			InteractionType GetType() const { return type; }
			// The world space position(origin + offset) of shape
			Donya::Vector3 GetPosition() const { return position + offset; }
			// The origin of shape
			Donya::Vector3 GetOrigin() const { return position; }
			// The offset of shape
			Donya::Vector3 GetOffset() const { return offset; }
			// For branch a collision detection method
			virtual Shape GetShapeKind() const = 0;
			// Minimum side xyz of AABB that wraps the myself
			virtual Donya::Vector3 GetAABBMin() const = 0;
			// Maximum side xyz of AABB that wraps the myself
			virtual Donya::Vector3 GetAABBMax() const = 0;
		public:
			// Distance between myself and point("pt")
			virtual float CalcDistanceTo( const Donya::Vector3 &pt ) const = 0;
			// The closest point to "pt" within my shape
			virtual Donya::Vector3 FindClosestPointTo( const Donya::Vector3 &pt ) const = 0;
			// 
			virtual HitResult IntersectTo( const ShapeBase *pOtherShape ) const = 0;
		};

		// Signature of callback of triggering an intersection(call it instead of CONTINUE version), use like this:
		// void OnHitEnter( DONYA_CALLBACK_ON_HIT_ENTER );
		// It callback will be called before resolving(after call it, resolve will works).
	#define DONYA_CALLBACK_ON_HIT_ENTER \
			const Donya::Collision::Substance &hitOther, /* Intersected target */ \
			const std::shared_ptr<Donya::Collision::ShapeBase> &pHitOtherShape, /* Intersected shape of target's (not nullptr) */ \
			const Donya::Collision::Substance &hitMyself, /* Intersected substance of myself */ \
			const std::shared_ptr<Donya::Collision::ShapeBase> &pHitMyselfShape, /* Intersected shape of myself (not nullptr) */ \
			const Donya::Collision::HitResult &hitParam /* Intersect description */

		// Signature of callback of continuing an intersection, use like this:
		// void OnHitContinue( DONYA_CALLBACK_ON_HIT_CONTINUE );
		// It callback will be called before resolving(after call it, resolve will works).
	#define DONYA_CALLBACK_ON_HIT_CONTINUE \
			DONYA_CALLBACK_ON_HIT_ENTER // Same signature

		// Signature of callback of end an intersection, use like this:
		// void OnHitExit( DONYA_CALLBACK_ON_HIT_EXIT );
	#define DONYA_CALLBACK_ON_HIT_EXIT \
			const Donya::Collision::Substance &leaveOther, /* Previously intersected target */ \
			const std::shared_ptr<Donya::Collision::ShapeBase> &pLeaveOtherShape, /* Previously intersected shape of target's (not nullptr) */ \
			const Donya::Collision::Substance &leaveMyself, /* Previously intersected substance of myself */ \
			const std::shared_ptr<Donya::Collision::ShapeBase> &pLeaveMyselfShape /* Previously intersected shape of myself (not nullptr) */

		// Substance of collision body.
		class Substance
		{
		public:
			// Function pointer of callback of triggering an intersection(call it instead of CONTINUE version).
			// It function will be called before resolving(after call it, resolve will works).
			using EventFuncT_OnHitEnter		= std::function<void( DONYA_CALLBACK_ON_HIT_ENTER )>;
			// Function pointer of callback of continuing an intersection.
			// It function will be called before resolving(after call it, resolve will works).
			using EventFuncT_OnHitContinue	= std::function<void( DONYA_CALLBACK_ON_HIT_CONTINUE )>;
			// Function pointer of callback of end an intersection.
			using EventFuncT_OnHitExit		= std::function<void( DONYA_CALLBACK_ON_HIT_EXIT )>;
		private: // HACK: Should "mass" be member of ShapeBase?
			float				mass = 1.0f;	// Requirement:[mass > 0.0f] Use only when push/be pushed
			Donya::Vector3		position;		// Origin of a shapes
			UniqueId<Substance>	id;				// For identify the instance of substance
			std::vector<std::shared_ptr<ShapeBase>>
								shapePtrs;		// Registered shape list.
		private:
			std::set<UniqueIdType>					hitSubstanceIds;	// For branch the calling methods when hit. OnHitEnter() or OnHitContinue() or OnHitExit()
			mutable std::set<UniqueIdType>			insertRequestIds;	// Use for delay the inserting until next intersection
			mutable std::set<UniqueIdType>			eraseRequestIds;	// Use for delay the erasing until next intersectionon
			std::vector<EventFuncT_OnHitEnter>		onHitHandlers_Enter;
			std::vector<EventFuncT_OnHitContinue>	onHitHandlers_Continue;
			std::vector<EventFuncT_OnHitExit>		onHitHandlers_Exit;
		public:
			// Register the clone of "pShape", so you can reuse the same shape.
			void RegisterShape( const std::shared_ptr<ShapeBase> &pShape );
			// Remove all registered shapes
			void RemoveAllShapes();
		public:
			void ResolveIntersectionWith( Substance *pOther );
			void RegisterCallback_OnHitEnter	( const EventFuncT_OnHitEnter		&function );
			void RegisterCallback_OnHitContinue	( const EventFuncT_OnHitContinue	&function );
			void RegisterCallback_OnHitExit		( const EventFuncT_OnHitExit		&function );
			void InvokeCallbacks( DONYA_CALLBACK_ON_HIT_ENTER ) const;
		public:
			// Requirement:[mass > 0.0f]
			void SetMass( float mass );
			void SetPosition( const Donya::Vector3 &position );
			// It changes only myself, so it may break the uniquely identify system.
			void OverwriteId( UniqueIdType newId );
		public:
			float			GetMass() const;
			Donya::Vector3	GetPosition() const;
			UniqueIdType	GetId() const;
			const std::vector<std::shared_ptr<ShapeBase>> *GetRegisteredShapePointers() const;
		private:
			void AcceptIdRequests();
		};

		// ( min <= v <= max )
		constexpr bool Within( float v, float min, float max )
		{
			return ( min <= v && v <= max );
		}




		// Handle of collision body, like proxy.
		class Collider
		{
		public:
			// Generate new collision body instance, then assign the reference to "pOut"(it does not alloc memory).
			// And register an initial shape as "pShape".
			// YOU MUST NOT CALL IT IN CALLBACK METHODS OF Substance, because these callbacks are called in iterating the Substances.
			static void Generate( Collider *pOut, const std::shared_ptr<ShapeBase> &pShape );
			// Resolve all collision body instance.
			static void Resolve();
		private:
			Substance *pReference = nullptr; // No ownership
		public:
			Collider()									= default;
			Collider( const Collider & )				= default;
			Collider( Collider && )						= default;
			Collider &operator = ( const Collider & )	= default;
			Collider &operator = ( Collider && )		= default;
			~Collider();
		public:
			// Destroy the collision body instance.
			// After that, this class will take nothing.
			// If already dead, it will do nothing.
			void Destroy();
		public:
			// Register the clone of "pShape", so you can reuse the same shape.
			void RegisterShape( const std::shared_ptr<ShapeBase> &pShape );
			// Remove all registered shapes
			void RemoveAllShapes();
		public:
			void RegisterCallback_OnHitEnter	( const Substance::EventFuncT_OnHitEnter	&function );
			void RegisterCallback_OnHitContinue	( const Substance::EventFuncT_OnHitContinue	&function );
			void RegisterCallback_OnHitExit		( const Substance::EventFuncT_OnHitExit		&function );
		public:
			// Requirement:[mass > 0.0f]
			void SetMass( float mass );
			void SetPosition( const Donya::Vector3 &position );
		public:
			// If already dead, returns FLT_EPSILON.
			float			GetMass() const;
			// If already dead, returns (0,0,0).
			Donya::Vector3	GetPosition() const;
			// If already dead, returns (0).
			UniqueIdType	GetColliderId() const;
			// If already dead, returns nullptr.
			const std::vector<std::shared_ptr<ShapeBase>> *GetRegisteredShapePointers() const;
		};
	}

	namespace Collision
	{
		/// <summary>
		/// You can assign the int type to it
		/// </summary>
		using  IDType = int;
		/// <summary>
		/// It will be used to recognize a ownerID is invalid.
		/// </summary>
		static constexpr IDType invalidID = -1;
		/// <summary>
		/// Returns only positive value.
		/// </summary>
		IDType GetUniqueID();

		/// <summary>
		/// Contains: ignoring identifier and ignoring the second. Please update it every frame.
		/// </summary>
		class  IgnoreElement
		{
		public:
			IDType	ignoreID		= 0;
			float	ignoreSecond	= FLT_MAX;
		public:
			void Update( float elapsedTime )
			{
				ignoreSecond -= elapsedTime;
			}
			bool ShouldRemove() const
			{
				return ( ignoreSecond <= 0.0f );
			}
		};
		/// <summary>
		/// Returns true if the "verifyID" is there in the "ignoreList".
		/// </summary>
		bool IsInIgnoreList( const std::vector<IgnoreElement> &ignoreList, IDType verifyID );

		namespace Base
		{
			/// <summary>
			/// Provides some rectangle for collision. That rectangle composed by center position and half size.<para></para>
			/// The T class must have a serialize method.
			/// </summary>
			template<typename T>
			class Box
			{
			public: // Serialize members
				T		pos;	// Center position. World position is pos + offset
				T		offset;	// World position is pos + offset
				T		size;	// Half size
				bool	exist;	// Used for ignore a collision
			public:
				IDType id;		// Default value is invalidID
				IDType ownerID;	// It will be the invalidID if do not has an owner
				std::vector<IgnoreElement> ignoreList;
			public:
				Box() : pos(), offset(), size(), exist( true ), id( invalidID ), ownerID( invalidID ), ignoreList()
				{}
				/// <summary>
				/// The offset will be default.
				/// </summary>
				Box( const T &pos, const T &size, bool exist = true )
					: pos( pos ), offset(), size( size ), exist( exist ), id( invalidID ), ownerID( invalidID ), ignoreList()
				{}
				Box( const T &pos, const T &offset, const T &size, bool exist = true )
					: pos( pos ), offset( offset ), size( size ), exist( exist ), id( invalidID ), ownerID( invalidID ), ignoreList()
				{}
			public:
				T WorldPosition() const { return pos + offset; }
				T Min() const { return WorldPosition() - size; }
				T Max() const { return WorldPosition() + size; }

				T WorldPosition( const Donya::Quaternion &orientation ) const
				{ return pos + orientation.RotateVector( offset ); }
				T Min( const Donya::Quaternion &orientation ) const
				{ return WorldPosition( orientation ) - size; }
				T Max( const Donya::Quaternion &orientation ) const
				{ return WorldPosition( orientation ) + size; }
			public:
				void UpdateIgnoreList( float elapsedTime )
				{
					for ( auto &it : ignoreList )
					{
						it.Update( elapsedTime );
					}

					auto result = std::remove_if
					(
						ignoreList.begin(), ignoreList.end(),
						[]( IgnoreElement &element )
						{
							return element.ShouldRemove();
						}
					);
					ignoreList.erase( result, ignoreList.end() );
				}
			public:
				static Box Nil() { return Box{ T{}, T{}, T{}, false }; }
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					archive
					(
						CEREAL_NVP( pos		),
						CEREAL_NVP( offset	),
						CEREAL_NVP( size	),
						CEREAL_NVP( exist	)
					);
					if ( 1 <= version )
					{
						// archive( CEREAL_NVP( x ) );
					}
				}
			};

			/// <summary>
			/// Provides some sphere for collision.<para></para>
			/// The CoordT class and RadiusT must have a serialize method.
			/// </summary>
			template<typename CoordT, typename RadiusT>
			class Sphere
			{
			public: // Serializer members
				CoordT	pos;	// Center position. World position is pos + offset
				CoordT	offset;	// World position is pos + offset
				RadiusT	radius;	// Half size
				bool	exist;	// Used for ignore a collision
			public:
				IDType id;		// Default value is invalidID
				IDType ownerID;	// It will be the invalidID if do not has an owner
				std::vector<IgnoreElement> ignoreList;
			public:
				Sphere() : pos(), offset(), radius(), exist( true ), id( invalidID ), ownerID( invalidID ), ignoreList()
				{}
				/// <summary>
				/// The offset will be zero.
				/// </summary>
				Sphere( const CoordT &pos, const RadiusT &radius, bool exist = true )
					: pos( pos ), offset(), radius( radius ), exist( exist ), id( invalidID ), ownerID( invalidID ), ignoreList()
				{}
				Sphere( const CoordT &pos, const CoordT &offset, const RadiusT &radius, bool exist = true )
					: pos( pos ), offset(), radius( radius ), exist( exist ), id( invalidID ), ownerID( invalidID ), ignoreList()
				{}
			public:
				CoordT WorldPosition() const
				{ return pos + offset; }
				CoordT WorldPosition( const Donya::Quaternion &orientation ) const
				{ return pos + orientation.RotateVector( offset ); }
			public:
				void UpdateIgnoreList( float elapsedTime )
				{
					for ( auto &it : ignoreList )
					{
						it.Update( elapsedTime );
					}

					auto result = std::remove_if
					(
						ignoreList.begin(), ignoreList.end(),
						[]( IgnoreElement &element )
						{
							return element.ShouldRemove();
						}
					);
					ignoreList.erase( result, ignoreList.end() );
				}
			public:
				static Sphere Nil() { return Sphere{ CoordT{}, CoordT{}, RadiusT{}, false }; }
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					archive
					(
						CEREAL_NVP( pos		),
						CEREAL_NVP( offset	),
						CEREAL_NVP( radius	),
						CEREAL_NVP( exist	)
					);
					if ( 1 <= version )
					{
						// archive();
					}
				}
			};

			template<typename T>
			static bool operator == ( const Base::Box<T> &a, const Base::Box<T> &b )
			{
				if ( a.pos		!= b.pos	) { return false; }
				if ( a.offset	!= b.offset	) { return false; }
				if ( a.size		!= b.size	) { return false; }
				if ( a.exist	!= b.exist	) { return false; }
				return true;
			}
			template<typename CoordT, typename RadiusT>
			static bool operator == ( const Base::Sphere<CoordT, RadiusT> &a, const Base::Sphere<CoordT, RadiusT> &b )
			{
				if ( a.pos		!= b.pos	) { return false; }
				if ( a.offset	!= b.offset	) { return false; }
				if ( a.radius	!= b.radius	) { return false; }
				if ( a.exist	!= b.exist	) { return false; }
				return true;
			}
			template<typename T>
			static bool operator != ( const Base::Box<T> &a, const Base::Box<T> &b )
			{
				return !( a == b );
			}
			template<typename CoordT, typename RadiusT>
			static bool operator != ( const Base::Sphere<CoordT, RadiusT> &a, const Base::Sphere<CoordT, RadiusT> &b )
			{
				return !( a == b );
			}
		}

		using Box2		= Base::Box<Donya::Int2>;
		using Box3		= Base::Box<Donya::Int3>;
		using Box2F		= Base::Box<Donya::Vector2>;
		using Box3F		= Base::Box<Donya::Vector3>;
		using Sphere2	= Base::Sphere<Donya::Int2, int>;
		using Sphere3	= Base::Sphere<Donya::Int3, int>;
		using Sphere2F	= Base::Sphere<Donya::Vector2, float>;
		using Sphere3F	= Base::Sphere<Donya::Vector3, float>;

		bool IsHit( const Donya::Int2 &a, const Box2 &b, bool considerExistFlag = true );
		bool IsHit( const Box2 &a, const Donya::Int2 &b, bool considerExistFlag = true );
		bool IsHit( const Box2 &a, const Box2 &b, bool considerExistFlag = true );
		bool IsHit( const Donya::Int3 &a, const Box3 &b, bool considerExistFlag = true );
		bool IsHit( const Box3 &a, const Donya::Int3 &b, bool considerExistFlag = true );
		bool IsHit( const Box3 &a, const Box3 &b, bool considerExistFlag = true );
		bool IsHit( const Donya::Vector2 &a, const Box2F &b, bool considerExistFlag = true );
		bool IsHit( const Box2F &a, const Donya::Vector2 &b, bool considerExistFlag = true );
		bool IsHit( const Box2F &a, const Box2F &b, bool considerExistFlag = true );
		bool IsHit( const Donya::Vector3 &a, const Box3F &b, bool considerExistFlag = true );
		bool IsHit( const Box3F &a, const Donya::Vector3 &b, bool considerExistFlag = true );
		bool IsHit( const Box3F &a, const Box3F &b, bool considerExistFlag = true );
		
		bool IsHit( const Donya::Int2 &a, const Sphere2 &b, bool considerExistFlag = true );
		bool IsHit( const Sphere2 &a, const Donya::Int2 &b, bool considerExistFlag = true );
		bool IsHit( const Sphere2 &a, const Sphere2 &b, bool considerExistFlag = true );
		bool IsHit( const Donya::Int3 &a, const Sphere3 &b, bool considerExistFlag = true );
		bool IsHit( const Sphere3 &a, const Donya::Int3 &b, bool considerExistFlag = true );
		bool IsHit( const Sphere3 &a, const Sphere3 &b, bool considerExistFlag = true );
		bool IsHit( const Donya::Vector2 &a, const Sphere2F &b, bool considerExistFlag = true );
		bool IsHit( const Sphere2F &a, const Donya::Vector2 &b, bool considerExistFlag = true );
		bool IsHit( const Sphere2F &a, const Sphere2F &b, bool considerExistFlag = true );
		bool IsHit( const Donya::Vector3 &a, const Sphere3F &b, bool considerExistFlag = true );
		bool IsHit( const Sphere3F &a, const Donya::Vector3 &b, bool considerExistFlag = true );
		bool IsHit( const Sphere3F &a, const Sphere3F &b, bool considerExistFlag = true );

		bool IsHit( const Box2 &a, const Sphere2 &b, bool considerExistFlag = true );
		bool IsHit( const Sphere2 &a, const Box2 &b, bool considerExistFlag = true );
		bool IsHit( const Box3 &a, const Sphere3 &b, bool considerExistFlag = true );
		bool IsHit( const Sphere3 &a, const Box3 &b, bool considerExistFlag = true );
		bool IsHit( const Box2F &a, const Sphere2F &b, bool considerExistFlag = true );
		bool IsHit( const Sphere2F &a, const Box2F &b, bool considerExistFlag = true );
		bool IsHit( const Box3F &a, const Sphere3F &b, bool considerExistFlag = true );
		bool IsHit( const Sphere3F &a, const Box3F &b, bool considerExistFlag = true );
		
		/// <summary>
		/// Return true if the "a" includes the "b" fully
		/// </summary>
		bool IsFullyInclude( const Box3F &a, const Box3F &b, bool considerExistFlag = true );
		/// <summary>
		/// Return true if the "a" includes the "b" fully
		/// </summary>
		bool IsFullyInclude( const Box3F &a, const Sphere3F &b, bool considerExistFlag = true );
		/// <summary>
		/// Return true if the "a" includes the "b" fully
		/// </summary>
		bool IsFullyInclude( const Sphere3F &a, const Box3F &b, bool considerExistFlag = true );
		/// <summary>
		/// Return true if the "a" includes the "b" fully
		/// </summary>
		bool IsFullyInclude( const Sphere3F &a, const Sphere3F &b, bool considerExistFlag = true );
		
		template<typename LHS, typename RHS>
		struct Solid
		{
			const LHS lhs;
			const RHS rhs;
		};

		namespace Impl
		{
			/// <summary>
			/// a VS ( b.lhs - b.rhs )
			/// </summary>
			template<typename HitBox, typename OtherLHS, typename OtherRHS>
			bool IsHitVSSubtracted( const HitBox &a, const Solid<OtherLHS, OtherRHS> &b, bool considerExistFlag = true )
			{
				// Requirement
				if ( !IsHit( a, b.lhs, considerExistFlag ) ) { return false; }
				// else

				// Subtract process is not necessary if the a does not touch to b's rhs
				if ( considerExistFlag && !b.rhs.exist		) { return true; }
				if ( !IsHit( a, b.rhs, considerExistFlag )	) { return true; }
				// else

				// "a" and "b" does not collide if the b.rhs(subtractor) fully includes the "a".
				if ( IsFullyInclude( b.rhs, a, /* considerExistFlag = */ false ) ) { return false; }
				// else

				return true;
			}
		}

		/// <summary>
		/// a VS ( b.lhs - b.rhs )
		/// </summary>
		template<typename OtherLHS, typename OtherRHS>
		bool IsHitVSSubtracted( const Box3F &a, const Solid<OtherLHS, OtherRHS> &b, bool considerExistFlag = true )
		{
			return Impl::IsHitVSSubtracted( a, b, considerExistFlag );
		}
		/// <summary>
		/// a VS ( b.lhs - b.rhs )
		/// </summary>
		template<typename OtherLHS, typename OtherRHS>
		bool IsHitVSSubtracted( const Sphere3F &a, const Solid<OtherLHS, OtherRHS> &b, bool considerExistFlag = true )
		{
			return Impl::IsHitVSSubtracted( a, b, considerExistFlag );
		}

		// TODO: Implement the IsFullyInclude() and IsHitVSSubtracted() in 2D type and interger type

		Donya::Int2 FindClosestPoint( const Donya::Int2 &from, const Box2 &to );
		Donya::Int2 FindClosestPoint( const Box2 &from, const Donya::Int2 &to );
		Donya::Int3 FindClosestPoint( const Donya::Int3 &from, const Box3 &to );
		Donya::Int3 FindClosestPoint( const Box3 &from, const Donya::Int3 &to );
		Donya::Vector2 FindClosestPoint( const Donya::Vector2 &from, const Box2F &to );
		Donya::Vector2 FindClosestPoint( const Box2F &from, const Donya::Vector2 &to );
		Donya::Vector3 FindClosestPoint( const Donya::Vector3 &from, const Box3F &to );
		Donya::Vector3 FindClosestPoint( const Box3F &from, const Donya::Vector3 &to );
		Donya::Int2 FindClosestPoint( const Donya::Int2 &from, const Sphere2 &to );
		Donya::Int2 FindClosestPoint( const Sphere2 &from, const Donya::Int2 &to );
		Donya::Int3 FindClosestPoint( const Donya::Int3 &from, const Sphere3 &to );
		Donya::Int3 FindClosestPoint( const Sphere3 &from, const Donya::Int3 &to );
		Donya::Vector2 FindClosestPoint( const Donya::Vector2 &from, const Sphere2F &to );
		Donya::Vector2 FindClosestPoint( const Sphere2F &from, const Donya::Vector2 &to );
		Donya::Vector3 FindClosestPoint( const Donya::Vector3 &from, const Sphere3F &to );
		Donya::Vector3 FindClosestPoint( const Sphere3F &from, const Donya::Vector3 &to );
	}

	class Circle;

	/// <summary>
	/// Hit-Box of rectangle.<para></para>
	/// Vector2 pos : Center-position. The calculate method regards the belong space is the same.<para></para>
	/// Vector2 size : Half-size(like radius). the left is pos.x - size.x. please set to only positive value.<para></para>
	/// bool exist : Is enable collision ?
	/// </summary>
	class Box
	{
	public:
		Donya::Vector2 pos;		// Position of center.
		Donya::Vector2 size;	// Half size.
		bool	exist;			// Is enable collision ?
	public:
		Box() : pos(), size(), exist( true ) {}
		Box
		(
			float centerX, float centerY,
			float halfWidth, float halfHeight,
			bool  isExist = true
		) :
			pos( centerX, centerY ),
			size( halfWidth, halfHeight ),
			exist( isExist )
		{}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( pos ),
				CEREAL_NVP( size ),
				CEREAL_NVP( exist )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		void Set				( float centerX, float centerY, float halfWidth, float halfHeight, bool isExist = true );
	public:
		static bool IsHitPoint	( const Box &L, const float &RX, const float &RY, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitPoint	( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const float &RX, const float &RY, bool ignoreExistFlag = false );
		static bool IsHitBox	( const Box &L, const Box &R, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitBox	( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const Box &R, const float &RBoxScreenPosX, const float &RBoxScreenPosY, bool ignoreExistFlag = false );
		static bool IsHitCircle	( const Box &L, const Circle &R, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitCircle	( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const Circle &R, const float &RCircleScreenPosX, const float &RCircleScreenPosY, bool ignoreExistFlag = false );
	public:
		static inline Box Nil()
		{
			return{ 0.0f, 0.0f, 0.0f, 0.0f, false };
		}
	};

	/// <summary>
	/// Hit-Box of circle.<para></para>
	/// Vector2 pos : Center-position. The calculate method regards the belong space is the same.<para></para>
	/// float radius : Specify radius.<para></para>
	/// bool exist :  Is enable collision ?
	/// </summary>
	class Circle
	{
	public:
		Donya::Vector2	pos;	// Position of center.
		float			radius;
		bool			exist;	// Is enable collision ?
	public:
		Circle() : pos(), radius(), exist( true ) {}
		Circle
		(
			float centerX, float centerY,
			float rad,
			bool  isExist = true
		) :
			pos( centerX, centerY ),
			radius( rad ),
			exist( isExist )
		{}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( pos ),
				CEREAL_NVP( radius ),
				CEREAL_NVP( exist )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		void Set				( float centerX, float centerY, float rad, bool isExist = true );
	public:
		static bool IsHitPoint	( const Circle &L, const float &RX, const float &RY, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitPoint	( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const float &RX, const float &RY, bool ignoreExistFlag = false );
		static bool IsHitCircle	( const Circle &L, const Circle &R, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitCircle	( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const Circle &R, const float &RCircleScreenPosX, const float &RCircleScreenPosY, bool ignoreExistFlag = false );
		static bool IsHitBox	( const Circle &L, const Box &R, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitBox	( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const Box &R, const float &RBoxScreenPosX, const float &RBoxScreenPosY, bool ignoreExistFlag = false );
	public:
		static inline Circle Nil()
		{
			return Circle{ 0, 0, 0, false };
		}
	};

	class Sphere;

	/// <summary>
	/// Hit-Box of AABB.<para></para>
	/// Vector3 pos : Center-position. The calculate method regards the belong space is the same.<para></para>
	/// Vector3 size : Half-size(like radius). the left is pos.x - size.x. please set to only positive value.<para></para>
	/// bool exist : Is enable collision ?
	/// </summary>
	class AABB
	{
	public:
		Donya::Vector3 pos{};	// Center-position. The calculate method regards the belong space is the same.
		Donya::Vector3 size{};	// Half-size(like radius). the left is pos.x - size.x. please set to only positive value.
		bool exist{ true };		// Is enable collision ?
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( pos ),
				CEREAL_NVP( size ),
				CEREAL_NVP( exist )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		/// <summary>
		/// AABB vs Point, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitPoint( const AABB &worldSpaceBox, const Donya::Vector3 &worldSpacePoint, bool ignoreExistFlag = false );
		/// <summary>
		/// AABB vs AABB, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitAABB( const AABB &worldSpaceBoxL, const AABB &worldSpaceBoxR, bool ignoreExistFlag = false );
		/// <summary>
		/// AABB vs Sphere, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitSphere( const AABB &worldSpaceBox, const Sphere &worldSpaceSphere, bool ignoreExistFlag = false );
	public:
		static AABB Nil()
		{
			return AABB
			{
				Donya::Vector3{ 0.0f, 0.0f, 0.0f },
				Donya::Vector3{ 0.0f, 0.0f, 0.0f },
				false
			};
		}
	};

	/// <summary>
	/// Hit-Box of Sphere.<para></para>
	/// Vector3 pos : Center-position. The calculate method regards the belong space is the same..<para></para>
	/// float radius : Radius of sphere in world-space. please set to only positive value.<para></para>
	/// bool exist : Is enable collision ?
	/// </summary>
	class Sphere
	{
	public:
		Donya::Vector3	pos{};			// Center-position. The calculate method regards the belong space is the same.
		float			radius{};		// Radius of sphere in world-space. please set to only positive value.
		bool			exist{ true };	// Is enable collision ?
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( pos ),
				CEREAL_NVP( radius ),
				CEREAL_NVP( exist )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		/// <summary>
		/// Sphere vs Point, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitPoint( const Sphere &worldSpaceSphere, const Donya::Vector3 &worldSpacePoint, bool ignoreExistFlag = false );
		/// <summary>
		/// Sphere vs Sphere, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitSphere( const Sphere &worldSpaceSphereL, const Sphere &worldSpaceSphereR, bool ignoreExistFlag = false );
		/// <summary>
		/// Sphere vs AABB, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitAABB( const Sphere &worldSpaceSphere, const AABB &worldSpaceBox, bool ignoreExistFlag = false );
	public:
		static Sphere Nil()
		{
			return Sphere
			{
				Donya::Vector3{ 0.0f, 0.0f, 0.0f },
				0.0f,
				false
			};
		}
	};

	/// <summary>
	/// Hit-Box of AABB.<para></para>
	/// Vector3 pos : Center-position. The calculate method regards the belong space is the same.<para></para>
	/// Vector3 size : Half-size(like radius). the left is pos.x - size.x. please set to only positive value.<para></para>
	/// Quaternion orientation : Represent a rotation. The "size" vector will rotate by "orientation" at the calculate method.<para></para>
	/// bool exist : Is enable collision ?
	/// </summary>
	class OBB
	{
	public:
		Donya::Vector3		pos{};			// Center-position. The calculate method regards the belong space is the same.
		Donya::Vector3		size{};			// Half-size(like radius). the left is pos.x - size.x. please set to only positive value.
		Donya::Quaternion	orientation{};	// Represent a rotation. The "size" vector will rotate by "orientation" at the calculate method.
		bool exist{ true };					// Is enable collision ?
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( pos ),
				CEREAL_NVP( size ),
				CEREAL_NVP( orientation ),
				CEREAL_NVP( exist )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		static bool IsHitOBB( const OBB &L, const OBB &R, bool ignoreExistFlag = false );
		static bool IsHitSphere( const OBB &L, const Sphere &R, bool ignoreExistFlag = false );
	public:
		static OBB Nil()
		{
			return OBB{ {}, {}, {}, false };
		}
	};

	struct Plane
	{
		float			distance;	// The shortest distance to the plane's any point from the origin.
		Donya::Vector3	normal;		// The plane's normal.
	public:
		Plane() : distance( 0.0f ), normal( 0.0f, 1.0f, 0.0f ) {}
		Plane( float distance, const Donya::Vector3 &normal ) : distance( distance ), normal( normal ) {}
	};

	struct RayIntersectResult
	{
		Donya::Vector3 intersection;
		Donya::Vector3 normal;
		bool isIntersect = false;
	};
	RayIntersectResult CalcIntersectionPoint( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, const AABB &box );
	/// <summary>
	/// If the ray places onto the plane, we returns: intersection = rayStart, normal = Zero, isIntersect = true.
	/// </summary>
	RayIntersectResult CalcIntersectionPoint( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, const Plane &plane );

	bool		operator == ( const Box &L,		const Box &R );
	static bool	operator != ( const Box &L,		const Box &R ) { return !( L == R ); }

	bool		operator == ( const Circle &L,	const Circle &R );
	static bool	operator != ( const Circle &L,	const Circle &R ) { return !( L == R ); }

	bool		operator == ( const AABB &L,	const AABB &R );
	static bool	operator != ( const AABB &L,	const AABB &R ) { return !( L == R ); }

	bool		operator == ( const Sphere &L,	const Sphere &R );
	static bool	operator != ( const Sphere &L,	const Sphere &R ) { return !( L == R ); }
	
	bool		operator == ( const OBB &L,		const OBB &R );
	static bool	operator != ( const OBB &L,		const OBB &R ) { return !( L == R ); }
}

CEREAL_CLASS_VERSION( Donya::Collision::Box2,		0 );
CEREAL_CLASS_VERSION( Donya::Collision::Box3,		0 );
CEREAL_CLASS_VERSION( Donya::Collision::Box2F,		0 );
CEREAL_CLASS_VERSION( Donya::Collision::Box3F,		0 );
CEREAL_CLASS_VERSION( Donya::Collision::Sphere2,	0 );
CEREAL_CLASS_VERSION( Donya::Collision::Sphere3,	0 );
CEREAL_CLASS_VERSION( Donya::Collision::Sphere2F,	0 );
CEREAL_CLASS_VERSION( Donya::Collision::Sphere3F,	0 );

CEREAL_CLASS_VERSION( Donya::Box,		1 );
CEREAL_CLASS_VERSION( Donya::Circle,	1 );
CEREAL_CLASS_VERSION( Donya::AABB,		0 );
CEREAL_CLASS_VERSION( Donya::Sphere,	0 );
CEREAL_CLASS_VERSION( Donya::OBB,		0 );

#endif // INCLUDED_DONYA_COLLISION_H_
