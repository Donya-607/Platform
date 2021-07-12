#pragma once

#include "../Collision.h"

namespace Donya
{
	namespace Collision
	{
		class ShapeSphere : public ShapeBase
		{
		public:
			static std::shared_ptr<ShapeBase> Generate
			(
				InteractionType interactionType,
				float radius,
				const Donya::Vector3 &posOffset = { 0.0f, 0.0f, 0.0f }
			);
		public:
			float radius = 0.0f;
		public:
			std::shared_ptr<ShapeBase> Clone() const override;
			Shape GetShapeKind() const override { return Shape::Sphere; }
			float GetRadius() const { return radius; }
			Donya::Vector3 GetAABBMin() const override;
			Donya::Vector3 GetAABBMax() const override;
			float CalcDistanceTo( const Donya::Vector3 &pt ) const override;
			Donya::Vector3 FindClosestPointTo( const Donya::Vector3 &pt ) const override;
		protected:
			HitResult IntersectTo( const ShapeBase *pOtherShape ) const override;
		};
	}
}
