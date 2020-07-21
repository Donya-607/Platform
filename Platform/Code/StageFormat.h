#pragma once

#include <string>

#include "Donya/UseImGui.h"

namespace StageFormat
{
	enum ID
	{
		EmptyValue		= -1,
		Space			= 0,
		StartPointRight	= 1,	// Looking right
		StartPointLeft	= 2,	// Looking left
		ClearEvent		= 3,
		Normal			= 4,
		Ladder			= 5,
		Needle			= 6,

		RoomStart		= 8,				// [RoomStart <= N <= RoomLast] is room identifier
		RoomLast		= RoomStart + 15,	// [RoomStart <= N <= RoomLast] is room identifier

		EnemyStart		= RoomLast + 1,		// [EnemyStart <= N <= EnemyLast] is enemy identifier
		EnemyLast		= EnemyStart + 7,	// [EnemyStart <= N <= EnemyLast] is enemy identifier
		
		BossStart		= EnemyLast + 1,	// [BossStart <= N <= BossLast] is boss identifier
		BossLast		= BossStart + 7,	// [BossStart <= N <= BossLast] is boss identifier

		IdentifierCount
	};

	std::string MakeIDName( ID id );
#if USE_IMGUI
	/// <summary>
	/// Returns the return value of ImGui::TreeNode().
	/// </summary>
	bool ShowImGuiNode( const std::string &nodeCaption, ID *pID, bool useTreeNode = true );
#endif // USE_IMGUI
}
