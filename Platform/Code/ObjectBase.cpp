#include "ObjectBase.h"

#include "Donya/Constant.h"	// Use scast macro
#include "Donya/Useful.h"	// Use ZeroEqual(), SignBit()


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
		movedBody = p->GetHitBox();
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
	const auto body  = GetHitBox();
	const int  foot  = body.pos.y + body.size.y;
	const int  floor = onto.pos.y - onto.size.y;

	return ( foot + 1 == floor );
}
Donya::Int2 Actor::GetPosition() const
{
	return pos + hitBox.pos;
}
Donya::Collision::Box2 Actor::GetHitBox() const
{
	Donya::Collision::Box2 tmp;
	tmp.pos		= GetPosition();
	tmp.size	= hitBox.size;
	tmp.exist	= hitBox.exist;
	return tmp;
}
void Actor::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Quaternion &rotation, const Donya::Vector4 &color ) const
{

}


void Solid::Move( const Donya::Int2 &movement, const std::vector<Actor *> &affectedActorPtrs )
{

}
Donya::Int2 Solid::GetPosition() const
{
	return pos + hitBox.pos;
}
Donya::Collision::Box2 Solid::GetHitBox() const
{
	Donya::Collision::Box2 tmp;
	tmp.pos		= GetPosition();
	tmp.size	= hitBox.size;
	tmp.exist	= hitBox.exist;
	return tmp;
}
void Solid::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Quaternion &rotation, const Donya::Vector4 &color ) const
{

}
