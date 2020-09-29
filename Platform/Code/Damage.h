#pragma once

#include <string>
#include <Windows.h>			// Use DEFINE_ENUM_FLAG_OPERATORS

#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"		// Use USE_IMGUI macro

namespace Definition
{
	class Damage
	{
	public:
		/// <summary>
		/// You can use bitwise operation.
		/// </summary>
		enum class Type
		{
			None		= 0,
			Buster		= 1 << 0,
			Pierce		= 1 << 1,	// If an other is broken by damage, a breaker object is not receive an impact.
			ForcePierce = 1 << 2,	// Always pierce an other even if can not break that.
			Protection	= 1 << 3,	// Protect an other bullet
		};
		static constexpr int TypeCount = 4; // Except the None
	public:
		/// <summary>
		/// GetContainName( Buster | XXX ) returns "[Buster][XXX]".
		/// </summary>
		static std::string GetContainName( Type value );
		static Type Add( Type lhs, Type rhs );
		static Type Subtract( Type lhs, Type rhs );
		static bool Contain( Type value, Type verify );
	#if USE_IMGUI
		static void ShowImGuiNode( const std::string &nodeCaption, Type *p, bool useTreeNode = true );
	#endif // USE_IMGUI
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
	public:
		void Combine( const Damage &addition );
	#if USE_IMGUI
	public:
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
}
DEFINE_ENUM_FLAG_OPERATORS( Definition::Damage::Type )
CEREAL_CLASS_VERSION( Definition::Damage, 0 )
