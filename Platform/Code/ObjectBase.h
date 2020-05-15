#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/ModelPolygon.h"
#include "Donya/Vector.h"

#include "Renderer.h"


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
public:
	template<typename OnCollisionTriggeredMethod>
	void MoveX( float movement, const std::vector<Donya::Collision::Box2> &solids, OnCollisionTriggeredMethod &OnCollisionMethod )
	{
		constexpr int dimension = 0;
		const int collideIndex  = MoveAxis( this, dimension, movement, solids );
		if ( collideIndex != -1 )
		{
			OnCollisionMethod();
		}
	}
	template<typename OnCollisionTriggeredMethod>
	void MoveY( float movement, const std::vector<Donya::Collision::Box2> &solids, OnCollisionTriggeredMethod &OnCollisionMethod )
	{
		constexpr int dimension = 1;
		const int collideIndex  = MoveAxis( this, dimension, movement, solids );
		if ( collideIndex != -1 )
		{
			OnCollisionMethod();
		}
	}
public:
	virtual bool IsRiding( const Donya::Collision::Box2 &onto ) const;
	/// <summary>
	/// Will call when pushed by Solid.
	/// </summary>
	virtual void Squish() {}
public:
	virtual Donya::Int2				GetPosition()	const;
	virtual Donya::Collision::Box2	GetHitBox()		const;
public:
	virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Quaternion &rotation = Donya::Quaternion::Identity(), const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
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
	Donya::Int2				GetPosition()	const;
	Donya::Collision::Box2	GetHitBox()		const;
public:
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Quaternion &rotation = Donya::Quaternion::Identity(), const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
};

