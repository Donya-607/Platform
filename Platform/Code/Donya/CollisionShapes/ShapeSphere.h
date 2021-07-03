#pragma once

#include "../Collision.h"

namespace Donya
{
	namespace Collision
	{
		class ShapeSphere : public ShapeBase
		{
		public:
			float radius = 0.0f;
		public:
			Shape GetShape() const override { return Shape::Sphere; }
			Donya::Vector3 GetAABBMin() const override;
			Donya::Vector3 GetAABBMax() const override;
			Donya::Vector3 FindClosestPointTo( const Donya::Vector3 &pt ) const override;
			HitResult IntersectTo( const ShapeBase *pOtherShape ) const override;
		};
	}
}
