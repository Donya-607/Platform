#include "ObjectBase.h"

#include "Donya/Constant.h"	// Use scast macro
#include "Donya/Useful.h"	// Use IsZero(), SignBit()
#include "Donya/Sprite.h"	// Drawing a rectangle


namespace
{
	template<typename Collision>
	int FindCollidingIndex( const Collision &input, const std::vector<Collision> &solids )
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

	Donya::Collision::Box2F ToFloat( const Donya::Collision::Box2 &intBox, const Donya::Vector2 &remainder )
	{
		Donya::Collision::Box2F tmp{};
		tmp.pos   = intBox.WorldPosition().Float() + remainder;
		tmp.size  = intBox.size.Float() + ( remainder * 0.5f /* To half size */ );
		tmp.exist = intBox.exist;
		return tmp;
	}

	Donya::Vector4x4 MakeWorldMatrix( const Donya::Collision::Box3F &box )
	{
		const auto wsPos = box.WorldPosition();
		Donya::Vector4x4 W{};
		W._11 = box.size.x * 2.0f;
		W._22 = box.size.y * 2.0f;
		W._33 = box.size.z * 2.0f;
		W._41 = wsPos.x;
		W._42 = wsPos.y;
		W._43 = wsPos.z;
		return W;
	}

	// A drawing origin will be regarded as a center. Returns drawing result.
	bool DrawHitBoxImpl( const Donya::Collision::Box2F &drawBoxF, const Donya::Vector4 &color )
	{
		const auto wsPos = drawBoxF.WorldPosition();
		return Donya::Sprite::DrawRect
		(
			wsPos.x,
			wsPos.y,
			drawBoxF.size.x * 2.0f /* To whole size*/,
			drawBoxF.size.y * 2.0f /* To whole size*/,
			color.x, color.y, color.z, color.w, 0.0f,
			Donya::Sprite::Origin::CENTER
		);
	}
	void DrawCube( RenderingHelper *pRenderer, const Donya::Vector4x4 &W, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
	{
		Donya::Model::Cube::Constant constant;
		constant.matWorld			= W;
		constant.matViewProj		= VP;
		constant.drawColor			= color;
		constant.lightDirection		= -Donya::Vector3::Up();
		pRenderer->ProcessDrawingCube( constant );
	}

	enum Dimension : int
	{
		X = 0,
		Y,
		Z
	};
}


int Actor2D::MoveAxis( Actor2D *p, int axis, float sourceMovement, const std::vector<Donya::Collision::Box2> &solids )
{
	if ( !p ) { return -1; }
	// else

	float &remainder = p->posRemainder[axis];
	remainder += sourceMovement;

	float movement = std::round( remainder );
	if ( IsZero( movement ) ) { return -1; }
	// else

	remainder -= movement;
	const int	moveSign  = Donya::SignBit( movement );
	const float	moveSignF = scast<float>( moveSign );

	Donya::Collision::Box2 movedBody{};
	while ( !IsZero( movement ) ) // Moves 1 pixel at a time.
	{
		// Verify some solid is there at destination first.
		movedBody = p->GetHitBox();
		movedBody.pos[axis] += moveSign;
		const int collideIndex = FindCollidingIndex( movedBody, solids );

		// I will stop before move if some solid is there.
		if ( collideIndex != -1 ) { return collideIndex; }
		// else

		p->body.pos[axis] += moveSign;
		movement -= moveSignF; // The movement is float, but this was made by round(), that will be zero and then break this loop.
	}

	return -1;
}
int Actor2D::MoveX( float movement, const std::vector<Donya::Collision::Box2> &solids )
{
	return MoveAxis( this, Dimension::X, movement, solids );
}
int Actor2D::MoveY( float movement, const std::vector<Donya::Collision::Box2> &solids )
{
	return MoveAxis( this, Dimension::Y, movement, solids );
}
bool Actor2D::IsRiding( const Donya::Collision::Box2 &onto ) const
{
	const auto body  = GetHitBox();
	const int  foot  = body.WorldPosition().y + body.size.y;
	const int  floor = onto.WorldPosition().y - onto.size.y;

	return ( foot + 1 == floor );
}
Donya::Int2 Actor2D::GetPosition() const
{
	return body.WorldPosition();
}
Donya::Vector2 Actor2D::GetPositionFloat() const
{
	return GetPosition().Float() + posRemainder;
}
Donya::Collision::Box2  Actor2D::GetHitBox() const
{
	return body;
}
Donya::Collision::Box2F Actor2D::GetHitBoxFloat() const
{
	return ToFloat( body, posRemainder );
}
bool Actor2D::DrawHitBox( const Donya::Vector4 &color ) const
{
	const auto drawBoxF = GetHitBoxFloat();
	return DrawHitBoxImpl( drawBoxF, color );
}


int Actor::MoveAxis( Actor *p, int axis, float movement, const std::vector<Donya::Collision::Box3F> &solids )
{
	if ( !p ) { return -1; }
	if ( IsZero( movement ) ) { return -1; }
	// else

	const int moveSign = Donya::SignBit( movement );

	Donya::Collision::Box3F wsMovedBody = p->GetHitBox();
	wsMovedBody.pos[axis] += movement;

	auto CalcPenetration	= [&p]( int axis, int moveSign, const Donya::Collision::Box3F &myself, const Donya::Collision::Box3F &other )
	{
		const float plusPenetration  = fabsf( ( myself.Max( p->orientation )[axis] ) - ( other.Min( p->orientation )[axis] ) );
		const float minusPenetration = fabsf( ( myself.Min( p->orientation )[axis] ) - ( other.Max( p->orientation )[axis] ) );
		const float penetration
					= ( moveSign < 0 ) ? minusPenetration
					: ( moveSign > 0 ) ? plusPenetration
					: 0.0f;
		return penetration;
	};
	auto CalcResolver		= []( int axis, int moveSign, float penetration )
	{
		// Prevent the two edges onto same place(the collision detective allows same(equal) value).
		constexpr float ERROR_MARGIN = 0.001f;
		const float resolver = ( penetration + ERROR_MARGIN ) * ( -moveSign );
		return resolver;
	};
	
	constexpr unsigned int MAX_LOOP_COUNT = 1000U;
	unsigned int loopCount{};
	int lastCollideIndex = -1;
	while ( ++loopCount <= MAX_LOOP_COUNT )
	{
		const int currentIndex = FindCollidingIndex( wsMovedBody, solids );
		if ( currentIndex < 0 ) { break; } // Does not detected a collision.
		// else

		lastCollideIndex = currentIndex;

		const Donya::Collision::Box3F *pOther = &solids[lastCollideIndex];

		// Store absolute value.
		const float penetration	= CalcPenetration( axis, moveSign, wsMovedBody, *pOther );
		const float resolver	= CalcResolver   ( axis, moveSign, penetration );

		wsMovedBody.pos[axis] += resolver;
	}

	const Donya::Vector3 diff = wsMovedBody.pos - p->body.pos;
	p->body.pos[axis] += diff[axis];

	return lastCollideIndex;
}
int Actor::MoveX( float movement, const std::vector<Donya::Collision::Box3F> &solids )
{
	return MoveAxis( this, Dimension::X, movement, solids );
}
int Actor::MoveY( float movement, const std::vector<Donya::Collision::Box3F> &solids )
{
	return MoveAxis( this, Dimension::Y, movement, solids );
}
int Actor::MoveZ( float movement, const std::vector<Donya::Collision::Box3F> &solids )
{
	return MoveAxis( this, Dimension::Z, movement, solids );
}
bool Actor::IsRiding( const Donya::Collision::Box3F &onto, float checkLength ) const
{
	const float foot  = body.WorldPosition( orientation ).y - body.size.y;
	const float floor = onto.WorldPosition( orientation ).y + onto.size.y;

	return ( floor <= foot && foot - checkLength <= floor );
}
Donya::Vector3 Actor::GetPosition() const
{
	return body.WorldPosition( orientation );
}
Donya::Collision::Box3F Actor::GetHitBox() const
{
	Donya::Collision::Box3F tmp = body;
	tmp.offset = orientation.RotateVector( tmp.offset );
	return tmp;
}
void Actor::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color ) const
{
	DrawCube( pRenderer, MakeWorldMatrix( body ), VP, color );
}


void Solid2D::Move( const Donya::Int2		&sourceMovement, const std::vector<Actor2D *> &affectedActor2DPtrs, const std::vector<Donya::Collision::Box2> &solids )
{
	// Call float version.
	Move( sourceMovement.Float(), affectedActor2DPtrs, solids );
}
void Solid2D::Move( const Donya::Vector2	&sourceMovement, const std::vector<Actor2D *> &affectedActor2DPtrs, const std::vector<Donya::Collision::Box2> &solids )
{
	// Store riding actors. This process must do before the move(if do it after the move, we may be out of riding range).
	std::vector<Actor2D *> ridingActor2DPtrs{};
	for ( const auto &it : affectedActor2DPtrs )
	{
		if ( !it ) { continue; }
		if ( !it->IsRiding( body ) ) { continue; }
		// else

		ridingActor2DPtrs.emplace_back( it );
	}
	auto IsRidingActor2D = [&]( const Actor2D *actorAddress )
	{
		for ( const auto &it : ridingActor2DPtrs )
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
		if ( IsZero( movement ) ) { return; }
		// else

		remainder -= movement;
		body.pos[axis] += scast<int>( movement );

		const Donya::Collision::Box2 movedBody = body;

		// Makes an actor that moved by me will ignore me.
		// so the actor will not consider to collide to me.
		body.exist = false;

		// Pushing and Carrying actors
		Donya::Collision::Box2 actorBody{};
		for ( auto &it : affectedActor2DPtrs )
		{
			if ( !it ) { continue; }
			// else

			auto MoveActor2D = [&]( Actor2D &actor, float movement )
			{
				switch ( axis )
				{
				case Dimension::X: return actor.MoveX( movement, solids );
				case Dimension::Y: return actor.MoveY( movement, solids );
				default: return -1;
				}
			};

			actorBody = it->GetHitBox();
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

				const int collideIndex = MoveActor2D( *it, scast<float>( pushAmount ) );
				if ( collideIndex != -1 )
				{
					it->Squish();
				}
			}
			else
			if ( IsRidingActor2D( it ) )
			{
				MoveActor2D( *it, movement );
			}
		}

		body.exist = true;
	};

	MovePerAxis( Dimension::X );
	MovePerAxis( Dimension::Y );
}
Donya::Int2 Solid2D::GetPosition() const
{
	return body.WorldPosition();
}
Donya::Vector2 Solid2D::GetPositionFloat() const
{
	return GetPosition().Float() + posRemainder;
}
Donya::Collision::Box2  Solid2D::GetHitBox() const
{
	return body;
}
Donya::Collision::Box2F Solid2D::GetHitBoxFloat() const
{
	return ToFloat( body, posRemainder );
}
bool Solid2D::DrawHitBox( const Donya::Vector4 &color ) const
{
	const auto drawBoxF = GetHitBoxFloat();
	return DrawHitBoxImpl( drawBoxF, color );
}

void Solid::Move( const Donya::Vector3 &sourceMovement, const std::vector<Actor *> &affectedActorPtrs, const std::vector<Donya::Collision::Box3F> &solids )
{
	// Store riding actors. This process must do before the move(if do it after the move, we may be out of riding range).
	std::vector<Actor *> ridingActorPtrs{};
	for ( const auto &it : affectedActorPtrs )
	{
		if ( !it ) { continue; }
		if ( !it->IsRiding( body ) ) { continue; }
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
		const float  movement = sourceMovement[axis];
		if ( IsZero( movement ) ) { return; }
		// else

		body.pos[axis] += movement;

		const Donya::Collision::Box3F movedBody = body;

		// Makes an actor that moved by me will ignore me.
		// so the actor will not consider to collide to me.
		body.exist = false;

		// Pushing and Carrying actors
		Donya::Collision::Box3F actorBody{};
		for ( auto &it : affectedActorPtrs )
		{
			if ( !it ) { continue; }
			// else

			auto MoveActor = [&]( Actor &actor, float movement )
			{
				switch ( axis )
				{
				case Dimension::X: return actor.MoveX( movement, solids );
				case Dimension::Y: return actor.MoveY( movement, solids );
				case Dimension::Z: return actor.MoveZ( movement, solids );
				default: return -1;
				}
			};

			actorBody = it->GetHitBox();
			if ( Donya::Collision::IsHit( actorBody, movedBody ) )
			{
				// Push the actor

				const auto minAct = actorBody.Min( orientation );
				const auto maxAct = actorBody.Max( orientation );
				const auto minMe  = movedBody.Min( orientation );
				const auto maxMe  = movedBody.Max( orientation );

				const float pushAmount = ( 0.0f < movement )
				? maxMe[axis] - minAct[axis]	// e.g. this.right - actor.left
				: minMe[axis] - maxAct[axis];	// e.g. this.left  - actor.right

				const int collideIndex = MoveActor( *it, pushAmount );
				if ( collideIndex != -1 )
				{
					it->Squish();
				}
			}
			else
			if ( IsRidingActor( it ) )
			{
				MoveActor( *it, movement );
			}
		}

		body.exist = true;
	};

	MovePerAxis( Dimension::X );
	MovePerAxis( Dimension::Y );
	MovePerAxis( Dimension::Z );
}
Donya::Vector3 Solid::GetPosition() const
{
	return body.WorldPosition( orientation );
}
Donya::Collision::Box3F Solid::GetHitBox() const
{
	Donya::Collision::Box3F tmp = body;
	tmp.offset = orientation.RotateVector( tmp.offset );
	return tmp;
}
void Solid::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color ) const
{
	DrawCube( pRenderer, MakeWorldMatrix( body ), VP, color );
}
