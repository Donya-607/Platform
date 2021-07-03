#pragma once

#include "../Collision.h"

namespace Donya
{
	namespace Collision
	{
		class ShapePoint : public ShapeBase
		{
		public:
			Shape GetShape() const override { return Shape::Point; }
			Donya::Vector3 GetAABBMin() const override;
			Donya::Vector3 GetAABBMax() const override;
			Donya::Vector3 FindClosestPointTo( const Donya::Vector3 &pt ) const override;
			HitResult IntersectTo( const ShapeBase *pOtherShape ) const override;
		};
	}
}
