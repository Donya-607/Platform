#pragma once

#include <cereal/types/polymorphic.hpp>

#include "ObjectBase.h"

class Tile : public Solid
{
private:
	using Solid::pos;
	using Solid::posRemainder;
	using Solid::hitBox;
	Donya::Int2 tsCoord;	// Tile space. 0-based.
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<Solid>( this ),
			CEREAL_NVP( tsCoord )
		);
		if ( 1 <= version )
		{
			// archive();
		}
	}
public:

};
CEREAL_CLASS_VERSION( Tile )
CEREAL_REGISTER_TYPE( Tile )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Solid, Tile )
