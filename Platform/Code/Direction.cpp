#include "Direction.h"

namespace Definition
{
	std::string GetContainName( const Direction &dir )
	{
		if ( dir == Direction::Nil ) { return "[Nil]"; }
		// else

		std::string str = "";
		if ( Contain( dir, Direction::Up	) ) { str += "[Up]";	}
		if ( Contain( dir, Direction::Down	) ) { str += "[Down]";	}
		if ( Contain( dir, Direction::Left	) ) { str += "[Left]";	}
		if ( Contain( dir, Direction::Right	) ) { str += "[Right]";	}
		return str;
	}

#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption, Direction *p, bool useTreeNode )
	{
		if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		constexpr int directionCount = 4;
		constexpr std::array<Direction, directionCount> directions
		{
			Direction::Up,
			Direction::Down,
			Direction::Left,
			Direction::Right,
		};
		
		/*
		bool enableUp		= Contain( *p, Direction::Up	);
		bool enableDown		= Contain( *p, Direction::Down	);
		bool enableLeft		= Contain( *p, Direction::Left	);
		bool enableRight	= Contain( *p, Direction::Right	);
		*/
		std::array<bool, directionCount> states;
		for ( int i = 0; i < directionCount; ++i )
		{
			states[i] = Contain( *p, directions[i] );
		}

		/*
		ImGui::Checkbox( u8"上", &enableUp		);
		ImGui::Checkbox( u8"下", &enableDown		);
		ImGui::Checkbox( u8"左", &enableLeft		);
		ImGui::Checkbox( u8"右", &enableRight	);
		*/
		constexpr std::array<const char *, directionCount> captions
		{
			u8"上",
			u8"下",
			u8"左",
			u8"右",
		};
		for ( int i = 0; i < directionCount; ++i )
		{
			ImGui::Checkbox( captions[i], &states[i] );
			if ( i + 1 < directionCount )
			{
				ImGui::SameLine();
			}
		}

		for ( int i = 0; i < directionCount; ++i )
		{
			if ( states[i] )	{ *p = Add		( *p, directions[i] ); }
			else				{ *p = Subtract	( *p, directions[i] ); }
		}

		ImGui::Text( u8"現在：%s", GetContainName( *p ).c_str() );

		if ( useTreeNode ) { ImGui::TreePop(); }
	}
#endif // USE_IMGUI
}
