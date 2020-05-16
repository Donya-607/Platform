#include "ObjectBase.h"

#include "Donya/Constant.h"	// Use scast macro
#include "Donya/Useful.h"	// Use ZeroEqual(), SignBit()
#include "Donya/Sprite.h"	// Drawing a rectangle


namespace
{
	int FindCollidingIndex( const Donya::Collision::Box2 &input, const std::vector<Donya::Collision::Box2> &solids )
	{
		const int count = scast<int>( solids.size() );
		for ( int i = 0; i < count; ++i )
		{
			if ( Donya::Collision::IsHit( input, solids[i] ) )
			{
				return i;
			}
		}

		return -1;
	}

	// A drawing origin will be regarded as a center. Returns drawing result.
	bool DrawHitBoxImpl( const Donya::Collision::Box2 &drawBox, const Donya::Vector2 &posRemainder, const Donya::Vector4 &color )
	{
		Donya::Collision::Box2F drawBoxF{};
		drawBoxF.pos  = drawBox.pos.Float() + posRemainder;
		drawBoxF.size = drawBox.size.Float() + ( posRemainder * 0.5f /* To half size */ );

		return Donya::Sprite::DrawRect
		(
			drawBoxF.pos.x,
			drawBoxF.pos.y,
			drawBoxF.size.x * 2.0f /* To whole size*/,
			drawBoxF.size.y * 2.0f /* To whole size*/,
			color.x, color.y, color.z, color.w, 0.0f,
			Donya::Sprite::Origin::CENTER
		);
	}
}


int Actor::MoveAxis( Actor *p, int axis, float sourceMovement, const std::vector<Donya::Collision::Box2> &solids )
{
	if ( !p ) { return -1; }
	// else

	float &remainder = p->posRemainder[axis];
	remainder += sourceMovement;

	float movement = std::round( remainder );
	if ( ZeroEqual( movement ) ) { return -1; }
	// else

	remainder -= movement;
	const int	moveSign  = Donya::SignBit( movement );
	const float	moveSignF = scast<float>( movement );

	Donya::Collision::Box2 movedBody{};
	while ( ZeroEqual( movement ) ) // Moves 1 pixel at a time.
	{
		// Verify some solid is there at destination first.
		movedBody = p->GetWorldHitBox();
		movedBody.pos[axis] += moveSign;
		const int collideIndex = FindCollidingIndex( movedBody, solids );

		// I will stop before move if some solid is there.
		if ( collideIndex != -1 ) { return collideIndex; }
		// else

		p->pos[axis] += moveSign;
		movement -= moveSignF; // The movement is float, but this was made by round(), that will be zero and then break this loop.
	}

	return -1;
}

bool Actor::IsRiding( const Donya::Collision::Box2 &onto ) const
{
	const auto body  = GetWorldHitBox();
	const int  foot  = body.pos.y + body.size.y;
	const int  floor = onto.pos.y - onto.size.y;

	return ( foot + 1 == floor );
}
Donya::Int2 Actor::GetPosition() const
{
	return pos + hitBox.pos;
}
Donya::Vector2 Actor::GetFloatPosition() const
{
	return GetPosition().Float() + posRemainder;
}
Donya::Collision::Box2 Actor::GetWorldHitBox() const
{
	Donya::Collision::Box2 tmp;
	tmp.pos		= GetPosition();
	tmp.size	= hitBox.size;
	tmp.exist	= hitBox.exist;
	return tmp;
}
bool Actor::DrawHitBox( const Donya::Vector4 &color ) const
{
	const auto drawBox = GetWorldHitBox();
	return DrawHitBoxImpl( drawBox, posRemainder, color );
}


void Solid::Move( const Donya::Int2		&sourceMovement, const std::vector<Actor *> &affectedActorPtrs, const std::vector<Donya::Collision::Box2> &solids )
{
	// Call float version.
	Move( sourceMovement.Float(), affectedActorPtrs, solids );
}
void Solid::Move( const Donya::Vector2	&sourceMovement, const std::vector<Actor *> &affectedActorPtrs, const std::vector<Donya::Collision::Box2> &solids )
{
	enum Dimension { X = 0, Y = 1 };

	const Donya::Collision::Box2 oldBody = GetWorldHitBox();

	// Store riding actors. This process must do before the move(if do it after the move, we may be out of riding range).
	std::vector<Actor *> ridingActorPtrs{};
	for ( const auto &it : affectedActorPtrs )
	{
		if ( !it ) { continue; }
		if ( !it->IsRiding( oldBody ) ) { continue; }
		// else

		ridingActorPtrs.emplace_back( it );
	}
	auto IsRidingActor = [&]( const Actor *actorAddress )
	{
		for ( const auto &it : ridingActorPtrs )
		{
			if ( it == actorAddress )
			{
				return true;
			}
		}

		return false;
	};

	auto MovePerAxis = [&]( int axis )
	{
		float &remainder = posRemainder[axis];
		remainder += sourceMovement[axis];

		const float movement = std::round( remainder );
		if ( ZeroEqual( movement ) ) { return; }
		// else

		remainder -= movement;
		pos[axis] += scast<int>( movement );

		const Donya::Collision::Box2 movedBody = GetWorldHitBox();

		// Makes an actor that moved by me will ignore me.
		// so the actor will not consider to collide to me.
		hitBox.exist = false;

		// Pushing and Carrying actors
		Donya::Collision::Box2 actorBody{};
		for ( auto &it : affectedActorPtrs )
		{
			if ( !it ) { continue; }
			// else

			auto MoveActor = [&]( Actor &actor, float movement, auto OnCollisionMethod )
			{
				switch ( axis )
				{
				case Dimension::X: actor.MoveX( movement, solids, OnCollisionMethod ); return;
				case Dimension::Y: actor.MoveY( movement, solids, OnCollisionMethod ); return;
				default: return;
				}
			};

			actorBody = it->GetWorldHitBox();
			if ( Donya::Collision::IsHit( actorBody, movedBody ) )
			{
				// Push the actor

				const auto minAct = actorBody.Min();
				const auto maxAct = actorBody.Max();
				const auto minMe  = movedBody.Min();
				const auto maxMe  = movedBody.Max();

				const int pushAmount = ( 0.0f < movement )
				? maxMe[axis] - minAct[axis]	// e.g. this.right - actor.left
				: minMe[axis] - maxAct[axis];	// e.g. this.left  - actor.right

				MoveActor( *it, scast<float>( pushAmount ), [&]() { it->Squish(); } );
			}
			else
			if ( IsRidingActor( it ) )
			{
				MoveActor( *it, movement, []() {} );
			}
		}

		hitBox.exist = true;
	};

	MovePerAxis( Dimension::X );
	MovePerAxis( Dimension::Y );
}
Donya::Int2 Solid::GetPosition() const
{
	return pos + hitBox.pos;
}
Donya::Vector2 Solid::GetFloatPosition() const
{
	return GetPosition().Float() + posRemainder;
}
Donya::Collision::Box2 Solid::GetWorldHitBox() const
{
	Donya::Collision::Box2 tmp;
	tmp.pos		= GetPosition();
	tmp.size	= hitBox.size;
	tmp.exist	= hitBox.exist;
	return tmp;
}
bool Solid::DrawHitBox( const Donya::Vector4 &color ) const
{
	const auto drawBox = GetWorldHitBox();
	return DrawHitBoxImpl( drawBox, posRemainder, color );
}
