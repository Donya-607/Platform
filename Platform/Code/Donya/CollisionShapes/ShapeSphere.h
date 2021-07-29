#pragma once

#undef max
#undef min
#include <cereal/types/polymorphic.hpp>

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
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					cereal::base_class<ShapeBase>( this ),
					CEREAL_NVP( radius )
				);
				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
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

CEREAL_CLASS_VERSION( Donya::Collision::ShapeSphere, 0 )
CEREAL_REGISTER_TYPE( Donya::Collision::ShapeSphere )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Donya::Collision::ShapeBase, Donya::Collision::ShapeSphere )
