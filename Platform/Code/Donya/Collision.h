#ifndef INCLUDED_DONYA_COLLISION_H_
#define INCLUDED_DONYA_COLLISION_H_

#include <cstdint>	// Use for std::uint32_t
#include <vector>

#undef max
#undef min
#include <cereal/cereal.hpp>

#include "Vector.h"
#include "Quaternion.h"

namespace Donya
{
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
				T		pos;	// Center position. World position is pos + offset.
				T		offset;	// World position is pos + offset.
				T		size;	// Half size.
				bool	exist;	// Used for ignore a collision.
			public:
				IDType id;
				IDType ownerID;	// It will be the invalidID if do not has an owner
				std::vector<IgnoreElement> ignoreList;
			public:
				Box() : pos(), offset(), size(), exist( true ), id( 0 ), ownerID( invalidID ), ignoreList()
				{}
				/// <summary>
				/// The offset will be default.
				/// </summary>
				Box( const T &pos, const T &size, bool exist = true )
					: pos( pos ), offset(), size( size ), exist( exist ), id( 0 ), ownerID( invalidID ), ignoreList()
				{}
				Box( const T &pos, const T &offset, const T &size, bool exist = true )
					: pos( pos ), offset( offset ), size( size ), exist( exist ), id( 0 ), ownerID( invalidID ), ignoreList()
				{}
			public:
				T WorldPosition() const { return pos + offset; }
				T Min() const { return WorldPosition() - size; }
				T Max() const { return WorldPosition() + size; }
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
				CoordT	pos;	// Center position. World position is pos + offset.
				CoordT	offset;	// World position is pos + offset.
				RadiusT	radius;	// Half size.
				bool	exist;	// Used for ignore a collision.
			public:
				IDType id;
				IDType ownerID;
				std::vector<IgnoreElement> ignoreList;
			public:
				Sphere() : pos(), offset(), radius(), exist( true ), id( 0 ), ownerID( invalidID ), ignoreList()
				{}
				/// <summary>
				/// The offset will be zero.
				/// </summary>
				Sphere( const CoordT &pos, const RadiusT &radius, bool exist = true )
					: pos( pos ), offset(), radius( radius ), exist( exist ), id( 0 ), ownerID( invalidID ), ignoreList()
				{}
				Sphere( const CoordT &pos, const CoordT &offset, const RadiusT &radius, bool exist = true )
					: pos( pos ), offset(), radius( radius ), exist( exist ), id( 0 ), ownerID( invalidID ), ignoreList()
				{}
			public:
				CoordT WorldPosition() const { return pos + offset; }
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
