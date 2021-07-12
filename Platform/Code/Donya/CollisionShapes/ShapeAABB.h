#pragma once

#include "../Collision.h"

namespace Donya
{
	namespace Collision
	{
		class ShapeAABB : public ShapeBase
		{
		public:
			static std::shared_ptr<ShapeBase> Generate
			(
				InteractionType interactionType,
				const Donya::Vector3 &halfSize,
				const Donya::Vector3 &posOffset = { 0.0f, 0.0f, 0.0f }
			);
		public:
			Donya::Vector3 size; // HalfSize
		public:
			std::shared_ptr<ShapeBase> Clone() const override;
			Shape GetShapeKind() const override { return Shape::AABB; }
			Donya::Vector3 GetSize() const { return size; }
			Donya::Vector3 GetAABBMin() const override;
			Donya::Vector3 GetAABBMax() const override;
			float CalcDistanceTo( const Donya::Vector3 &pt ) const override;
			Donya::Vector3 FindClosestPointTo( const Donya::Vector3 &pt ) const override;
		protected:
			HitResult IntersectTo( const ShapeBase *pOtherShape ) const override;
		};
	}
}
