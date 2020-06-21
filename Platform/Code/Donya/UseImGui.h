#pragma once

#include "Constant.h"	// Use for DEBUG_MODE macro.

#ifndef FORCE_USE_IMGUI
#define FORCE_USE_IMGUI	( false )
#endif // FORCE_USE_IMGUI

#define USE_IMGUI		( DEBUG_MODE || FORCE_USE_IMGUI )

namespace Donya
{
	void SetShowStateOfImGui( bool isAllow );
	void ToggleShowStateOfImGui();

	/// <summary>
	/// In release build, returns false.
	/// </summary>
	bool IsAllowShowImGui();

	bool IsMouseHoveringImGuiWindow();
}

#if USE_IMGUI

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>

#include "Vector.h"

namespace Donya
{
	Donya::Vector2 FromImVec( const ImVec2 &v );
	Donya::Vector4 FromImVec( const ImVec4 &v );
	ImVec2 ToImVec( const Donya::Vector2 &v );
	ImVec4 ToImVec( const Donya::Vector4 &v );
}

namespace ImGui
{
	/// <summary>
	/// ! This is My Wrapper Function !<para></para>
	/// This function doing Donya::IsAllowShowImGui() before ImGui::Begin().<para></para>
	/// This function's return value is same as ImGui::Begin().<para></para>
	/// You must be evaluate this in if-statement, then If returns false, you must not do something of ImGui related.<para></para>
	/// If returns true, you must be call ImGui::End().
	/// </summary>
	bool BeginIfAllowed( const char* name = nullptr, bool* p_open = NULL, ImGuiWindowFlags flags = 0 );

}
#include "Collision.h"
namespace ImGui
{
	namespace Helper
	{
		void ShowAABBNode  ( const std::string &nodeCaption, Donya::Collision::Box3F	*pBox );
		void ShowSphereNode( const std::string &nodeCaption, Donya::Collision::Sphere3F	*pSphere );
	}
}

#endif // USE_IMGUI
