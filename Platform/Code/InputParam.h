#pragma once

#include <vector>

#undef max
#undef min
#include "cereal/types/vector.hpp"

#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Input.h"
#include "UI.h"

namespace Input
{
	struct Parameter
	{
	public:
		struct Part
		{
			UIObject keyboard;
			UIObject controller;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( keyboard	),
					CEREAL_NVP( controller	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
	public:
		std::vector<Part>			texViews; // size() == Input::Type::TypeCount
		std::vector<Donya::Vector2>	notifyingStretches; // Used for bezier-curve
		float						notifyingSecond = 1.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( texViews ) );

			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( notifyingStretches	),
					CEREAL_NVP( notifyingSecond		)
				);
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Input::Parameter,			1 )
CEREAL_CLASS_VERSION( Input::Parameter::Part,	0 )
