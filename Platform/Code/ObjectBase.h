#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "Renderer.h"


// I'm referring to this: https://medium.com/@MattThorson/celeste-and-towerfall-physics-d24bd2ae0fc5


/// /// <summary>
/// This class is a Solids that interactable with other Solids.
/// </summary>
class Actor2D
{
public:
	/// <summary>
	/// Returns the index of solid if the target collided to a solid of the solids, or -1 if the target didn't collide to any solids.
	/// </summary>
	static int MoveAxis( Actor2D *pTarget, int moveDimension, float movement, const std::vector<Donya::Collision::Box2> &solids );
public:
	Donya::Int2				pos;
	Donya::Vector2			posRemainder;
	Donya::Collision::Box2	hitBox;			// The "pos" acts as an offset.
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( pos				),
			CEREAL_NVP( posRemainder	),
			CEREAL_NVP( hitBox			)
		);
		if ( 1 <= version )
		{
			// archive();
		}
	}
public:
	/// <summary>
	/// Returns the index of solid if the target collided to a solid of the solids, or -1 if the target didn't collide to any solids.
	/// </summary>
	int MoveX( float movement, const std::vector<Donya::Collision::Box2> &solids );
	/// <summary>
	/// Returns the index of solid if the target collided to a solid of the solids, or -1 if the target didn't collide to any solids.
	/// </summary>
	int MoveY( float movement, const std::vector<Donya::Collision::Box2> &solids );
public:
	virtual bool IsRiding( const Donya::Collision::Box2 &onto ) const;
	/// <summary>
	/// Will call when pushed by Solid.
	/// </summary>
	virtual void Squish() {}
public:
	virtual Donya::Int2				GetPosition()			const;
	virtual Donya::Vector2			GetPositionFloat()		const;
	virtual Donya::Collision::Box2	GetWorldHitBox()		const;
	virtual Donya::Collision::Box2F	GetWorldHitBoxFloat()	const;
public:
	/// <summary>
	/// A drawing origin will be regarded as a center. Returns drawing result.
	/// </summary>
	virtual bool DrawHitBox( const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
};
CEREAL_CLASS_VERSION( Actor2D, 0 );

/// /// <summary>
/// This class is a Solids that interactable with other Solids.
/// </summary>
class Actor
{
public:
	/// <summary>
	/// Returns the index of solid if the target collided to a solid of the solids, or -1 if the target didn't collide to any solids.
	/// </summary>
	static int MoveAxis( Actor *pTarget, int moveDimension, float movement, const std::vector<Donya::Collision::Box3F> &solids );
public:
	Donya::Vector3			pos;
	Donya::Collision::Box3F	hitBox;	// The "pos" acts as an offset.
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( pos		),
			CEREAL_NVP( hitBox	)
		);
		if ( 1 <= version )
		{
			// archive();
		}
	}
public:
	/// <summary>
	/// Returns the index of solid if the target collided to a solid of the solids, or -1 if the target didn't collide to any solids.
	/// </summary>
	int MoveX( float movement, const std::vector<Donya::Collision::Box3F> &solids );
	/// <summary>
	/// Returns the index of solid if the target collided to a solid of the solids, or -1 if the target didn't collide to any solids.
	/// </summary>
	int MoveY( float movement, const std::vector<Donya::Collision::Box3F> &solids );
	/// <summary>
	/// Returns the index of solid if the target collided to a solid of the solids, or -1 if the target didn't collide to any solids.
	/// </summary>
	int MoveZ( float movement, const std::vector<Donya::Collision::Box3F> &solids );
public:
	virtual bool IsRiding( const Donya::Collision::Box3F &onto, float checkLength = 0.001f ) const;
	/// <summary>
	/// Will call when pushed by Solid.
	/// </summary>
	virtual void Squish() {}
public:
	virtual Donya::Vector3			GetPosition()		const;
	virtual Donya::Collision::Box3F	GetWorldHitBox()	const;
public:
	/// <summary>
	/// A drawing origin will be regarded as a center. Returns drawing result.
	/// </summary>
	virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
};
CEREAL_CLASS_VERSION( Actor, 0 );


/// <summary>
/// This class is a movable hitbox. Solid2Ds don't interact with other Solid2Ds.
/// </summary>
class Solid2D
{
public:
	Donya::Int2				pos;
	Donya::Vector2			posRemainder;
	Donya::Collision::Box2	hitBox;			// The "pos" acts as an offset.
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( pos				),
			CEREAL_NVP( posRemainder	),
			CEREAL_NVP( hitBox			)
		);
		if ( 1 <= version )
		{
			// archive();
		}
	}
public:
	/// <summary>
	/// My move can be guaranteed to get there. The "relativeActors" will be pushed(or carried) if colliding. The "solids" will be used for the actors move.
	/// </summary>
	void Move( const Donya::Int2	&movement, const std::vector<Actor2D *> &affectedActorPtrs, const std::vector<Donya::Collision::Box2> &solids );
	/// <summary>
	/// My move can be guaranteed to get there. The "relativeActors" will be pushed(or carried) if colliding. The "solids" will be used for the actors move.
	/// </summary>
	void Move( const Donya::Vector2	&movement, const std::vector<Actor2D *> &affectedActorPtrs, const std::vector<Donya::Collision::Box2> &solids );
public:
	Donya::Int2				GetPosition()			const;
	Donya::Vector2			GetPositionFloat()		const;
	Donya::Collision::Box2	GetWorldHitBox()		const;
	Donya::Collision::Box2F	GetWorldHitBoxFloat()	const;
public:
	/// <summary>
	/// A drawing origin will be regarded as a center. Returns drawing result.
	/// </summary>
	bool DrawHitBox( const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
};
CEREAL_CLASS_VERSION( Solid2D, 0 );

/// <summary>
/// This class is a movable hitbox. Solids don't interact with other Solids.
/// </summary>
class Solid
{
public:
	Donya::Vector3			pos;
	Donya::Collision::Box3F	hitBox;	// The "pos" acts as an offset.
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( pos		),
			CEREAL_NVP( hitBox	)
		);
		if ( 1 <= version )
		{
			// archive();
		}
	}
public:
	/// <summary>
	/// My move can be guaranteed to get there. The "relativeActors" will be pushed(or carried) if colliding. The "solids" will be used for the actors move.
	/// </summary>
	void Move( const Donya::Vector3	&movement, const std::vector<Actor *> &affectedActorPtrs, const std::vector<Donya::Collision::Box3F> &solids );
public:
	Donya::Vector3			GetPosition()		const;
	Donya::Collision::Box3F	GetWorldHitBox()	const;
public:
	/// <summary>
	/// A drawing origin will be regarded as a center. Returns drawing result.
	/// </summary>
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
};
CEREAL_CLASS_VERSION( Solid, 0 );
