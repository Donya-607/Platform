#pragma once

#include "../Collision.h"

namespace Donya
{
	namespace Collision
	{
		class ShapePoint : public ShapeBase
		{
		public:
			static std::shared_ptr<ShapeBase> Generate
			(
				Type interactionType,
				const Donya::Vector3 &posOffset = { 0.0f, 0.0f, 0.0f }
			);
		public:
			std::shared_ptr<ShapeBase> Clone() const override;
			Shape GetShapeKind() const override { return Shape::Point; }
			Donya::Vector3 GetAABBMin() const override;
			Donya::Vector3 GetAABBMax() const override;
			float CalcDistanceTo( const Donya::Vector3 &pt ) const override;
			Donya::Vector3 FindClosestPointTo( const Donya::Vector3 &pt ) const override;
			HitResult IntersectTo( const ShapeBase *pOtherShape ) const override;
		};
	}
}
