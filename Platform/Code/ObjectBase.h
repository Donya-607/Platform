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
	Donya::Int2				pos;
	Donya::Collision::Box2	hitBox;	// The "pos" acts as an offset.
public:
	virtual void Move( const Donya::Int2 &movement );
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
	Donya::Collision::Box2	hitBox;	// The "pos" acts as an offset.
public:
	/// <summary>
	/// The "relativeActors" will be pushed(or carried) if colliding.
	/// </summary>
	void Move( const Donya::Int2 &movement, const std::vector<Actor *> &affectedActorPtrs );
public:
	Donya::Int2				GetPosition()	const;
	Donya::Collision::Box2	GetHitBox()		const;
public:
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Quaternion &rotation = Donya::Quaternion::Identity(), const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
};

