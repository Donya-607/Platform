#include "Damage.h"

#include "Constant.h"	// Use scast macro

namespace Definition
{
	std::string Damage::GetContainName( Type value )
	{
		if ( value == Type::None ) { return "[None]"; }
		// else

		std::string str = "";
		if ( scast<int>( value & Type::Buster ) != 0 ) { str += "[Buster]"; }
		if ( scast<int>( value & Type::Pierce ) != 0 ) { str += "[Pierce]"; }
		return str;
	}
	Damage::Type Damage::Add( Type lhs, Type rhs )
	{
		return lhs | rhs;
	}
	Damage::Type Damage::Subtract( Type lhs, Type rhs )
	{
		return lhs & ~( rhs );
	}
	bool Damage::Contain( Type value, Type verify )
	{
		return scast<int>( value & verify ) != 0;
	}
#if USE_IMGUI
	void Damage::ShowImGuiNode( const std::string &nodeCaption, Type *p, bool useTreeNode )
	{
		constexpr int typeCount = 1;
		constexpr std::array<Type, typeCount> directions
		{
			Type::Buster,
		};

		std::array<bool, typeCount> states;
		for ( int i = 0; i < typeCount; ++i )
		{
			states[i] = Contain( *p, directions[i] );
		}

		constexpr std::array<const char *, typeCount> captions
		{
			"Buster",
		};
		for ( int i = 0; i < typeCount; ++i )
		{
			ImGui::Checkbox( captions[i], &states[i] );
			if ( i + 1 < typeCount )
			{
				ImGui::SameLine();
			}
		}

		for ( int i = 0; i < typeCount; ++i )
		{
			if ( states[i] )	{ *p = Add		( *p, directions[i] ); }
			else				{ *p = Subtract	( *p, directions[i] ); }
		}

		ImGui::Text( u8"Œ»ÝF%s", GetContainName( *p ).c_str() );
	}
#endif // USE_IMGUI

	void Damage::Combine( const Damage &addition )
	{
		amount += addition.amount;
		type = Add( type, addition.type );
	}
#if USE_IMGUI
	void Damage::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::DragInt( u8"—^‚¦‚é—Ê", &amount );
		ShowImGuiNode( u8"‘®«Ý’è", &type );

		ImGui::TreePop();
	}
#endif // USE_IMGUI
}
