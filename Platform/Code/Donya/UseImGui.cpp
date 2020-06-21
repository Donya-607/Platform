#include "UseImGui.h"

#include "Donya.h"		// Use for GetWindowCaption().

namespace Donya
{
	static bool isAllowShowingImGui = true;
	void SetShowStateOfImGui( bool isAllow )
	{
		isAllowShowingImGui = isAllow;
	}
	void ToggleShowStateOfImGui()
	{
		isAllowShowingImGui = !isAllowShowingImGui;
	}

	bool IsAllowShowImGui()
	{
		return isAllowShowingImGui;
	}

	bool IsMouseHoveringImGuiWindow()
	{
	#if USE_IMGUI
		return ImGui::IsMouseHoveringAnyWindow();
	#else
		return false;
	#endif // !USE_IMGUI
	}
}

#if USE_IMGUI

namespace Donya
{
	Donya::Vector2 FromImVec( const ImVec2 &v ) { return Donya::Vector2{ v.x, v.y }; }
	Donya::Vector4 FromImVec( const ImVec4 &v ) { return Donya::Vector4{ v.x, v.y, v.z, v.w }; }
	ImVec2 ToImVec( const Donya::Vector2 &v ) { return ImVec2{ v.x, v.y }; }
	ImVec4 ToImVec( const Donya::Vector4 &v ) { return ImVec4{ v.x, v.y, v.z, v.w }; }
}

namespace ImGui
{
	bool BeginIfAllowed( const char* name, bool* p_open, ImGuiWindowFlags flags )
	{
	#if !USE_IMGUI

		return false;

	#endif // !USE_IMGUI

		if ( !Donya::IsAllowShowImGui() ) { return false; }
		// else

		const char *windowName	=	( name == nullptr )
									? Donya::GetWindowCaption()
									: name;

		if ( !ImGui::Begin( windowName, p_open, flags ) )
		{
			ImGui::End();
			return false;
		}
		// else
		return true;
	}
	
	namespace Helper
	{
		void ShowAABBNode  ( const std::string &nodeCaption, Donya::Collision::Box3F	*p )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragFloat3( u8"中心座標",				&p->pos.x,		0.01f	);
			ImGui::DragFloat3( u8"オフセット",			&p->offset.x,	0.01f	);
			ImGui::DragFloat3( u8"サイズ（半分を指定）",	&p->size.x,		0.01f	);
			ImGui::Checkbox  ( u8"判定を有効にする",		&p->exist				);

			ImGui::TreePop();
		}
		void ShowSphereNode( const std::string &nodeCaption, Donya::Collision::Sphere3F	*p )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragFloat3( u8"中心座標",				&p->pos.x,		0.01f	);
			ImGui::DragFloat3( u8"オフセット",			&p->offset.x,	0.01f	);
			ImGui::DragFloat3( u8"半径",					&p->radius,		0.01f	);
			ImGui::Checkbox  ( u8"判定を有効にする",		&p->exist				);

			ImGui::TreePop();
		}
	}
}

#endif // USE_IMGUI
