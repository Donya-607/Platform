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
#include "Easing.h"
#include "Quaternion.h"
namespace ImGui
{
	namespace Helper
	{
		void ShowEaseParam	( const std::string &nodeCaption, Donya::Easing::Kind *pKind, Donya::Easing::Type *pType );
		
		void ShowStringNode	( const std::string &nodeCaption, const std::string &bufferIdentifier, std::string *pString );

		void ShowAABBNode	( const std::string &nodeCaption, Donya::Collision::Box3F *pBox );
		void ShowSphereNode	( const std::string &nodeCaption, Donya::Collision::Sphere3F *pSphere );
		
		void ShowFrontNode	( const std::string &nodeCaption, Donya::Quaternion *pOrientation, bool freezeUpAxis = true, bool useTreeNode = false );

		template<class T>
		void ResizeByButton( std::vector<T> *p, const T &initializeValue = T{}, const char *captionAppend = u8"’Ç‰Á", const char *captionPop = u8"––”ö‚ðíœ" )
		{
			if ( ImGui::Button( captionAppend ) )
			{
				p->emplace_back( initializeValue );
			}
			if ( p->size() && ImGui::Button( captionPop ) )
			{
				p->pop_back();
			}
		}
	}
}

#endif // USE_IMGUI
