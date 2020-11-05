#include "UseImGui.h"

#include <unordered_map>// Use at ShowStringNode()

#include "Donya.h"		// Use for GetWindowCaption()

#include "../Math.h"	// Use CalcBezierCurve()

#undef max
#undef min

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
		void ShowIgnoreList( const std::string &nodeCaption, const std::vector<Donya::Collision::IgnoreElement> &ignoreList )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			const size_t count = ignoreList.size();
			ImGui::Text( u8"%d��", count );
			std::string caption;
			for ( size_t i = 0; i < count; ++i )
			{
				const auto &element = ignoreList[i];
				caption = Donya::MakeArraySuffix( element.ignoreID );
				caption += "[Remain:" + std::to_string( element.ignoreSecond ) + "]";
				ImGui::Text( caption.c_str() );
			}

			ImGui::TreePop();
		}

		void ShowEaseParam	( const std::string &nodeCaption, Donya::Easing::Kind *pKind, Donya::Easing::Type *pType )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			using namespace Donya;

			int intKind = scast<int>( *pKind );
			int intType = scast<int>( *pType );

			std::string name{};
			name =  u8"���݁F";
			name += u8"[";
			name += Easing::KindName( intKind );
			name += u8":";
			name += Easing::TypeName( intType );
			name += u8"]";
			ImGui::Text( name.c_str() );

			ImGui::SliderInt( u8"���",		&intKind, 0, Easing::GetKindCount() - 1 );
			ImGui::SliderInt( u8"�^�C�v",	&intType, 0, Easing::GetTypeCount() - 1 );

			*pKind = scast<Easing::Kind>( intKind );
			*pType = scast<Easing::Type>( intType );

			ImGui::TreePop();
		}
		void ShowStringNode	( const std::string &nodeCaption, const std::string &bufferId, std::string *p )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			constexpr size_t bufferSize			= 256U;
			constexpr size_t bufferSizeWithNull	= bufferSize + 1;
			using  BufferType = std::array<char, bufferSizeWithNull>;
			static std::unordered_map<std::string, BufferType> identifierMap;

			auto &buffer = identifierMap[bufferId];
			if ( std::strlen( buffer.data() ) == 0 )
			{
				strncpy_s( buffer.data(), bufferSize, p->c_str(), std::min( bufferSize, p->size() ) );
			}

			if ( *p == buffer.data() )
			{
				ImGui::TextDisabled( u8"�K�p" );
			}
			else if ( ImGui::Button( u8"�K�p" ) )
			{
				*p = buffer.data();
			}
			ImGui::SameLine();

			ImGui::InputText( ( "##" + bufferId ).c_str(), buffer.data(), bufferSize );

			ImGui::TreePop();
		}
		void ShowLightNode				( const std::string &nodeCaption, Donya::Model::Constants::PerScene::Light				*p, bool useTreeNode )
		{
			if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::ColorEdit4( u8"�A���r�G���g",		&p->ambientColor.x	);
			ImGui::ColorEdit4( u8"�f�B�t���[�Y",		&p->diffuseColor.x	);
			ImGui::ColorEdit3( u8"�X�y�L�����E�F",	&p->specularColor.x	);
			ImGui::DragFloat ( u8"�X�y�L�����E���x",	&p->specularColor.w, 0.01f );
			p->specularColor.w = std::max( 0.0f, p->specularColor.w );

			if ( useTreeNode ) { ImGui::TreePop(); }
		}
		void ShowDirectionalLightNode	( const std::string &nodeCaption, Donya::Model::Constants::PerScene::DirectionalLight	*p, bool useTreeNode )
		{
			if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ShowLightNode( "", &p->light, /* useTreeNode = */ false );
			ImGui::SliderFloat4( u8"����", &p->direction.x, -1.0f, 1.0f );
			if ( ImGui::Button( u8"�����𐳋K��" ) )
			{
				const Donya::Vector3 unit3D = p->direction.XYZ().Unit();
				p->direction.x = unit3D.x;
				p->direction.y = unit3D.y;
				p->direction.z = unit3D.z;
			}

			if ( useTreeNode ) { ImGui::TreePop(); }
		}
		void ShowPointLightNode			( const std::string &nodeCaption, Donya::Model::Constants::PerScene::PointLight			*p, bool useTreeNode )
		{
			if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ShowLightNode( "", &p->light, /* useTreeNode = */ false );

			ImGui::DragFloat3( u8"���[���h���W", &p->wsPos.x, 0.01f );
			p->_wsPosW = 1.0f;

			ImGui::DragFloat( u8"�e���͈�",			&p->range,			0.1f );
			ImGui::DragFloat( u8"�����W���E�萔",		&p->attenuation.x,	0.01f );
			ImGui::DragFloat( u8"�����W���E���`",		&p->attenuation.y,	0.01f );
			ImGui::DragFloat( u8"�����W���E�Q��",		&p->attenuation.z,	0.01f );

			p->attenuation.x	= std::max( 0.0f, p->attenuation.x	);
			p->attenuation.y	= std::max( 0.0f, p->attenuation.y	);
			p->attenuation.z	= std::max( 0.0f, p->attenuation.z	);
			p->range			= std::max( 0.0f, p->range			);

			if ( useTreeNode ) { ImGui::TreePop(); }
		}
		void ShowAABBNode	( const std::string &nodeCaption, Donya::Collision::Box3F *p )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragFloat3( u8"���S���W",				&p->pos.x,		0.01f	);
			ImGui::DragFloat3( u8"�I�t�Z�b�g",			&p->offset.x,	0.01f	);
			ImGui::DragFloat3( u8"�T�C�Y�i�������w��j",	&p->size.x,		0.01f	);
			ImGui::Checkbox  ( u8"�����L���ɂ���",		&p->exist				);
			
			if ( ImGui::TreeNode( ( u8"�h�c�F" + std::to_string( p->id ) ).c_str() ) )
			{
				ImGui::Text( u8"���g�Q�F%d", p->id );
				ImGui::Text( u8"���L�ҁF%d", p->ownerID );
				
				ShowIgnoreList( u8"�������X�g", p->ignoreList );

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
		void ShowSphereNode	( const std::string &nodeCaption, Donya::Collision::Sphere3F *p )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragFloat3( u8"���S���W",				&p->pos.x,		0.01f	);
			ImGui::DragFloat3( u8"�I�t�Z�b�g",			&p->offset.x,	0.01f	);
			ImGui::DragFloat3( u8"���a",					&p->radius,		0.01f	);
			ImGui::Checkbox  ( u8"�����L���ɂ���",		&p->exist				);

			if ( ImGui::TreeNode( ( u8"�h�c�F" + std::to_string( p->id ) ).c_str() ) )
			{
				ImGui::Text( u8"���g�Q�F%d", p->id );
				ImGui::Text( u8"���L�ҁF%d", p->ownerID );
				
				ShowIgnoreList( u8"�������X�g", p->ignoreList );

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
		void ShowFrontNode	( const std::string &nodeCaption, Donya::Quaternion *p, bool freezeUp, bool useTreeNode )
		{
			if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			Donya::Vector3 localFront = p->LocalFront();
			ImGui::SliderFloat3( u8"�O����", &localFront.x, -1.0f, 1.0f );
			*p = Donya::Quaternion::LookAt
			(
				Donya::Vector3::Front(),
				localFront.Unit(),
				( freezeUp )
				? Donya::Quaternion::Freeze::Up
				: Donya::Quaternion::Freeze::None
			);

			if ( ImGui::Button( u8"�p�������Z�b�g" ) )
			{
				*p = Donya::Quaternion::Identity();
			}

			if ( useTreeNode ) { ImGui::TreePop(); }
		}

		void ShowBezier2DNode( const std::string &nodeCaption, std::vector<Donya::Vector2> *pCtrlPoints, float rangeMin, float rangeMax )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			constexpr Donya::Vector2 append{ 1.0f, 1.0f };
			ResizeByButton( pCtrlPoints, append );
			if ( pCtrlPoints->size() < 2 )
			{
				pCtrlPoints->resize( 2 );
			}

			const int pointCount = pCtrlPoints->size();
			std::string caption;
			for ( int i = 0; i < pointCount; ++i )
			{
				caption = Donya::MakeArraySuffix( i );
				ImGui::SliderFloat2( caption.c_str(), &pCtrlPoints->at( i ).x, rangeMin, rangeMax );
			}

			ImGui::Text( "" ); // Line feed

			static std::unordered_map<std::string, float> checkers;
			auto found =  checkers.find( nodeCaption );
			if ( found == checkers.end() )
			{
				checkers.insert( std::make_pair( nodeCaption, 0.0f ) );
				found = checkers.find( nodeCaption );
			}

			auto &timer = found->second;
			ImGui::SliderFloat( u8"�m�F�p�^�C�}", &timer, 0.0f, 1.0f );

			Donya::Vector2 result = Math::CalcBezierCurve( *pCtrlPoints, timer );
			ImGui::SliderFloat2( u8"�x�W�F�Ȑ��K�p����", &result.x, rangeMin, rangeMax );

			ImGui::TreePop();
		}
	}
}

#endif // USE_IMGUI
