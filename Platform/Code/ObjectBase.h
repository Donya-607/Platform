#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"


// I'm referring to this: https://medium.com/@MattThorson/celeste-and-towerfall-physics-d24bd2ae0fc5


/// /// <summary>
/// This class is a Solids that interactable with other Solids.
/// </summary>
class Actor
{
public:
	/// <summary>
	/// Returns the index of solid if the target collided to a solid of the solids, or -1 if the target didn't collide to any solids.
	/// </summary>
	static int MoveAxis( Actor *pTarget, int moveDimension, float movement, const std::vector<Donya::Collision::Box2> &solids );
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


/// <summary>
/// This class is a movable hitbox. Solids don't interact with other Solids.
/// </summary>
class Solid
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
	void Move( const Donya::Int2	&movement, const std::vector<Actor *> &affectedActorPtrs, const std::vector<Donya::Collision::Box2> &solids );
	/// <summary>
	/// My move can be guaranteed to get there. The "relativeActors" will be pushed(or carried) if colliding. The "solids" will be used for the actors move.
	/// </summary>
	void Move( const Donya::Vector2	&movement, const std::vector<Actor *> &affectedActorPtrs, const std::vector<Donya::Collision::Box2> &solids );
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

CEREAL_CLASS_VERSION( Actor, 0 );
CEREAL_CLASS_VERSION( Solid, 0 );
