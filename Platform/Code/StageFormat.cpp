#include "StageFormat.h"
#include "Donya/Useful.h"

namespace StageFormat
{
	std::string MakeIDName( ID id )
	{
		switch ( id )
		{
		case EmptyValue:		return "Empty";
		case Space:				return "Space";
		case StartPointRight:	return "Start_Right";
		case StartPointLeft:	return "Start_Left";
		case ClearEvent:		return "ClearEv";
		case Normal:			return "Normal";
		case Ladder:			return "Ladder";
		case Needle:			return "Needle";
		default: break;
		}

		if ( RoomStart <= id && id <= RoomLast )
		{
			const int index = id - RoomStart;
			return "Room" + Donya::MakeArraySuffix( index );
		}
		if ( EnemyStart <= id && id <= EnemyLast )
		{
			const int index = id - EnemyStart;
			return "Enemy" + Donya::MakeArraySuffix( index );
		}
		if ( BossStart <= id && id <= BossLast )
		{
			const int index = id - BossStart;
			return "Boss" + Donya::MakeArraySuffix( index );
		}
		if ( ItemStart <= id && id <= ItemLast )
		{
			const int index = id - ItemStart;
			return "Item" + Donya::MakeArraySuffix( index );
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected ID!" );
		return "ERROR_ID";
	}

#if USE_IMGUI
	bool ShowImGuiNode( const std::string &nodeCaption, ID *p, bool useTreeNode )
	{
		if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else

		int intID = scast<int>( *p );
		ImGui::SliderInt( u8"ïœçX", &intID, 0, scast<int>( ID::IdentifierCount ) - 1 );
		*p = scast<ID>( intID );

		ImGui::Text( u8"åªç›ÅF%s", MakeIDName( *p ).c_str() );

		if ( useTreeNode ) { ImGui::TreePop(); }
		return true;
	}
#endif // USE_IMGUI
}
