#include "Command.h"

namespace Command
{
#if USE_IMGUI
	void Part::ShowImGuiNode( const char *nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption ) ) { return; }
		// else

		ImGui::DragFloat( u8"入力間の受付秒数", &marginSecond, 0.001f );

		
		ImGui::Text( u8"コマンドリスト（左始まり）" );

		constexpr int disallowButton = 0;
		const size_t count = sticks.size();

		ImGui::Columns( scast<int>( count + 1 ) );
		ImGui::Separator();

		std::string caption;


		// Input boxes and add box
		{
			for ( size_t i = 0; i < count; ++i )
			{
				caption = "##" + Donya::MakeArraySuffix( i );
			
				int intValue = scast<int>( sticks[i] );
				ImGui::InputInt( caption.c_str(), &intValue, disallowButton );
				sticks[i] = scast<NumPad::Value>( intValue );

				ImGui::NextColumn();
			}

			// 1-based. I expect it to be [1 ~ 9]
			char buf = -1;
			const bool changed = ImGui::InputTextWithHint( "##_ToEasilyAddNewElement", u8"追加", &buf, 1U );
			if ( changed )
			{
				const bool addable = ( 0 < buf && buf <= scast<char>( NumPad::keyCount ) );
				if ( addable )
				{
					// 0-based
					const NumPad::Value addition = scast<NumPad::Value>( buf - 1 );
					sticks.emplace_back( addition );
				}
			}

			ImGui::NextColumn();
		}
		ImGui::Separator();

		
		// Erase buttons
		{
			size_t eraseIndex = count;
			const ImVec2 buttonSize{ 32.0f, 32.0f };
			for ( size_t i = 0; i < count; ++i )
			{
				caption = u8"消去" + ( "##" + Donya::MakeArraySuffix( i ) );
				if ( ImGui::Button( caption.c_str(), buttonSize ) )
				{
					eraseIndex = i;
				}

				ImGui::NextColumn();
			}
			if ( eraseIndex != count )
			{
				sticks.erase( sticks.begin() + eraseIndex );
			}
		}
		ImGui::Separator();


		ImGui::Columns( 1 );
		ImGui::Separator();


		ImGui::TreePop();
	}
#endif // USE_IMGUI
}
