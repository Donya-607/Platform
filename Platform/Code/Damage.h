#pragma once

#include <Windows.h>	// Use DEFINE_ENUM_FLAG_OPERATORS

#include "Donya/Serializer.h"

namespace Definition
{
	struct Damage
	{
	public:
		/// <summary>
		/// You can use bitwise operation.
		/// </summary>
		enum class Type
		{
			None	= 0,
			Buster	= 1 << 0,
		};
	public:
		int		amount	= 0;
		Type	type	= Type::None;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( amount	),
				CEREAL_NVP( type	)
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	};
}
DEFINE_ENUM_FLAG_OPERATORS( Definition::Damage::Type )
CEREAL_CLASS_VERSION( Definition::Damage, 0 )
