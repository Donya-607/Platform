#include "ObjectBase.h"


void Actor::Move( const Donya::Int2 &movement )
{

}
bool Actor::IsRiding( const Donya::Collision::Box2 &onto ) const
{

}
Donya::Int2 Actor::GetPosition() const
{

}
Donya::Collision::Box2 Actor::GetHitBox() const
{

}
void Actor::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Quaternion &rotation, const Donya::Vector4 &color ) const
{

}


void Solid::Move( const Donya::Int2 &movement, const std::vector<Actor *> &affectedActorPtrs )
{

}
Donya::Int2 Solid::GetPosition() const
{

}
Donya::Collision::Box2 Solid::GetHitBox() const
{

}
void Solid::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Quaternion &rotation, const Donya::Vector4 &color ) const
{

}
