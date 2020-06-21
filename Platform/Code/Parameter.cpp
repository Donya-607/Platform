#include "Parameter.h"

#if USE_IMGUI
namespace ParameterHelper
{
	IOOperation ShowIONode()
	{
		if ( !ImGui::TreeNode( u8"ƒtƒ@ƒCƒ‹ I/O" ) ) { return IOOperation::None;; }
		// else

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		ImGui::SameLine();
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = "Load by:";
		loadStr += ( isBinary ) ? "Binary" : "Json";

		constexpr	float  charWidth	= 16.0f;
		constexpr	float  charHeight	= 32.0f;
		constexpr	float  buttonWidth	= charWidth * 8;
		constexpr	float  buttonHeight	= charHeight;
		const		ImVec2 buttonSize{ buttonWidth, buttonHeight };

		IOOperation command = IOOperation::None;
		if ( ImGui::Button( "Save", buttonSize ) )
		{
			command = IOOperation::Save;
		}
		ImGui::SameLine();
		if ( ImGui::Button( loadStr.c_str(), buttonSize ) )
		{
			command = ( isBinary )
			? IOOperation::LoadBinary
			: IOOperation::LoadJson;
		}

		ImGui::TreePop();
		return command;
	}
}
#endif // USE_IMGUI
