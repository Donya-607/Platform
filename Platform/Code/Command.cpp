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
			
				// 1-based
				int intValue = scast<int>( sticks[i] ) + 1;
				ImGui::InputInt( caption.c_str(), &intValue, disallowButton );
				// to 0-based
				sticks[i] = scast<NumPad::Value>( intValue - 1 );

				ImGui::NextColumn();
			}

			// 1-based. I expect it to be [1 ~ 9]
			constexpr std::array<char, 2U> initialStatus{ '\0', '\0' };
			static std::array<char, 2U> buf = initialStatus;
			ImGui::InputTextWithHint( "##_ToEasilyAddNewElement", u8"追加", buf.data(), 2U, ImGuiInputTextFlags_AutoSelectAll );
			if ( ImGui::IsItemDeactivatedAfterEdit() )
			{
				const unsigned int num = scast<unsigned int>( buf[0] - '0' );
				const bool addable = ( 0 < num && num <= NumPad::keyCount );
				if ( addable )
				{
					// to 0-based
					const NumPad::Value addition = scast<NumPad::Value>( num - 1 );
					sticks.emplace_back( addition );
				}

				buf = initialStatus;
			}

			ImGui::NextColumn();
		}
		ImGui::Separator();

		
		// Erase buttons
		{
			size_t eraseIndex = count;
			for ( size_t i = 0; i < count; ++i )
			{
				caption = u8"消去" + ( "##" + Donya::MakeArraySuffix( i ) );
				if ( ImGui::Button( caption.c_str() ) )
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
