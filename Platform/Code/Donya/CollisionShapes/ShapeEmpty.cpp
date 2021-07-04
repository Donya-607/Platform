#include "ShapeEmpty.h"

namespace Donya
{
	namespace Collision
	{
		std::shared_ptr<ShapeBase> ShapeEmpty::Generate( const Donya::Vector3 &posOffset )
		{
			std::shared_ptr<ShapeEmpty> tmp = std::make_shared<ShapeEmpty>();
			tmp->type	= InteractionType::Sensor;
			tmp->offset	= posOffset;

			// Up-cast it
			return tmp;
		}
		std::shared_ptr<ShapeBase> ShapeEmpty::Clone() const
		{
			std::shared_ptr<ShapeEmpty> tmp = std::make_shared<ShapeEmpty>( *this );
			// Up-cast it
			return tmp;
		}

		Donya::Vector3 ShapeEmpty::GetAABBMin() const
		{
			return GetPosition();
		}
		Donya::Vector3 ShapeEmpty::GetAABBMax() const
		{
			return GetPosition();
		}
		float ShapeEmpty::CalcDistanceTo( const Donya::Vector3 &pt ) const
		{
			return ( pt - GetPosition() ).Length();
		}
		Donya::Vector3 ShapeEmpty::FindClosestPointTo( const Donya::Vector3 &pt ) const
		{
			return GetPosition();
		}


		HitResult ShapeEmpty::IntersectTo( const ShapeBase *pOther ) const
		{
			HitResult result;
			result.isHit = false;
			return result;
		}
	}
}
