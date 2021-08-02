#include "UseImGui.h"

#include <unordered_map>// Use at ShowStringNode()

#include "Donya.h"		// Use for GetWindowCaption()
#include "CollisionShapes/ShapeEmpty.h"	// Use for ShowBodyNode(), ShowShapeNode()
#include "CollisionShapes/ShapePoint.h"	// Use for ShowBodyNode(), ShowShapeNode()
#include "CollisionShapes/ShapeAABB.h"	// Use for ShowBodyNode(), ShowShapeNode()
#include "CollisionShapes/ShapeSphere.h"// Use for ShowBodyNode(), ShowShapeNode()

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
			ImGui::Text( u8"%d個", count );
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
			name =  u8"現在：";
			name += u8"[";
			name += Easing::KindName( intKind );
			name += u8":";
			name += Easing::TypeName( intType );
			name += u8"]";
			ImGui::Text( name.c_str() );

			ImGui::SliderInt( u8"種類",		&intKind, 0, Easing::GetKindCount() - 1 );
			ImGui::SliderInt( u8"タイプ",	&intType, 0, Easing::GetTypeCount() - 1 );

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
				ImGui::TextDisabled( u8"適用" );
			}
			else if ( ImGui::Button( u8"適用" ) )
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

			ImGui::ColorEdit4( u8"アンビエント",		&p->ambientColor.x	);
			ImGui::ColorEdit4( u8"ディフューズ",		&p->diffuseColor.x	);
			ImGui::ColorEdit3( u8"スペキュラ・色",	&p->specularColor.x	);
			ImGui::DragFloat ( u8"スペキュラ・強度",	&p->specularColor.w, 0.01f );
			p->specularColor.w = std::max( 0.0f, p->specularColor.w );

			if ( useTreeNode ) { ImGui::TreePop(); }
		}
		void ShowDirectionalLightNode	( const std::string &nodeCaption, Donya::Model::Constants::PerScene::DirectionalLight	*p, bool useTreeNode )
		{
			if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ShowLightNode( "", &p->light, /* useTreeNode = */ false );
			ImGui::SliderFloat4( u8"方向", &p->direction.x, -1.0f, 1.0f );
			if ( ImGui::Button( u8"方向を正規化" ) )
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

			ImGui::DragFloat3( u8"ワールド座標", &p->wsPos.x, 0.01f );
			p->_wsPosW = 1.0f;

			ImGui::DragFloat( u8"影響範囲",			&p->range,			0.1f );
			ImGui::DragFloat( u8"減衰係数・定数",		&p->attenuation.x,	0.01f );
			ImGui::DragFloat( u8"減衰係数・線形",		&p->attenuation.y,	0.01f );
			ImGui::DragFloat( u8"減衰係数・２次",		&p->attenuation.z,	0.01f );

			p->attenuation.x	= std::max( 0.0f, p->attenuation.x	);
			p->attenuation.y	= std::max( 0.0f, p->attenuation.y	);
			p->attenuation.z	= std::max( 0.0f, p->attenuation.z	);
			p->range			= std::max( 0.0f, p->range			);

			if ( useTreeNode ) { ImGui::TreePop(); }
		}
		void ShowShapeNode	( const std::string &nodeCaption, Donya::Collision::ShapeBase *pShape )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::Text( u8"hogehogefoobar" );

			ImGui::TreePop();
		}
		void ShowBodyNode	( const std::string &nodeCaption, const std::string &bufferId, Donya::Collision::Body *pBody )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			constexpr auto tabFlag = ImGuiTabBarFlags_None | ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_FittingPolicyScroll;
			if ( !ImGui::BeginTabBar( ( bufferId + "EditTab" ).c_str(), tabFlag ) ) { return; }
			// else

			
			// Show Body's GUI
			if ( ImGui::BeginTabItem( u8"ボディ-編集" ) )
			{
				ImGui::Text( u8"固有ID：%d", pBody->GetId() );

				
				float mass = pBody->GetMass();
				ImGui::DragFloat( u8"質量", &mass, 0.005f );
				pBody->SetMass( std::max( 0.001f, mass ) );


				Donya::Vector3 pos = pBody->GetPosition();
				ImGui::DragFloat3( u8"座標", &pos.x );
				pBody->SetPosition( pos );
				
				
				bool wantIgnore = pBody->NowIgnoringIntersection();
				ImGui::Checkbox( u8"当たり判定を無視する", &wantIgnore );
				pBody->SetIgnoringIntersection( wantIgnore );


				ImGui::EndTabItem();
			}


			// Show Shape's GUIs
			if ( ImGui::BeginTabItem( u8"形状-編集" ) )
			{
				auto pShapes = pBody->RequireRawShapePointers();

				std::string caption;
				std::shared_ptr<Donya::Collision::ShapeBase> pShape = nullptr;

				const size_t count = ( pShapes ) ? pShapes->size() : 0U;
				for ( size_t i = 0; i < count; ++i )
				{
					pShape = pShapes->at( i );
					if ( !pShape ) { continue; }
					// else

					caption = Donya::MakeArraySuffix( i ) + ":" + Donya::Collision::GetShapeName( pShape->GetShapeKind() );
					ShowShapeNode( caption, pShape.get() );
				}

				
				ImGui::EndTabItem();
			}
			
			if ( ImGui::BeginTabItem( u8"形状-追加/削除" ) )
			{
				// Append
				if ( ImGui::Button( u8"末尾に追加" ) )
				{
					// For visualize, append some AABB
					constexpr auto initType = Donya::Collision::InteractionType::Sensor;
					constexpr auto initSize = Donya::Vector3{ 0.5f, 0.5f, 0.5f };
					auto tmp = Donya::Collision::ShapeAABB::Generate( initType, initSize );
					pBody->AddShape( tmp );
				}
				ImGui::Text( u8"" );


				// Remove
				auto pTargetShapes = pBody->RequireRawShapePointers();
				
				static std::unordered_map<std::string, std::shared_ptr<Donya::Collision::ShapeBase>> choosingShapeMap;
				auto &choosingShape = choosingShapeMap[bufferId];

				// Find erasing one
				std::string caption;
				std::shared_ptr<Donya::Collision::ShapeBase> pShape = nullptr;

				const size_t shapeCount = ( pTargetShapes ) ? pTargetShapes->size() : 0U;
				for ( size_t i = 0; i < shapeCount; ++i )
				{
					pShape = pTargetShapes->at( i );
					if ( !pShape ) { continue; }
					// else

					caption = Donya::MakeArraySuffix( i ) + ":" + Donya::Collision::GetShapeName( pShape->GetShapeKind() );
					if ( ImGui::Selectable( caption.c_str(), choosingShape == pShape ) )
					{
						choosingShape = pShape;
					}
				}
				if ( ImGui::Selectable( u8"選択解除" ) )
				{
					choosingShape = nullptr;
				}


				// Erase if chosen
				if ( choosingShape )
				{
					if ( ImGui::Button( u8"選択した形状を削除" ) )
					{
						// Find the index
						size_t  eraseIndex = shapeCount;
						for ( size_t i = 0; i < shapeCount; ++i )
						{
							pShape = pTargetShapes->at( i );
							if ( !pShape ) { continue; }
							// else

							if ( choosingShape == pShape )
							{
								eraseIndex = i;
								break;
							}
						}

						// Erase it
						if ( eraseIndex < shapeCount )
						{
							pTargetShapes->erase( pTargetShapes->begin() + eraseIndex );
							if ( pTargetShapes->empty() )
							{
								choosingShape = nullptr;
							}
							else
							{
								eraseIndex = std::min( pTargetShapes->size() - 1, eraseIndex );
								choosingShape = pTargetShapes->at( eraseIndex );
							}
						}
					}
				}
				else
				{
					ImGui::TextDisabled( u8"選択した形状を削除" );
				}


				ImGui::EndTabItem();
			}

			if ( ImGui::BeginTabItem( u8"形状-並び替え" ) )
			{
				constexpr const char *columnId = "ReorderBodyShapesColumn";
				ImGui::Columns( 2, columnId );


				auto pShapes = pBody->RequireRawShapePointers();

				size_t reorderFrom = ( pShapes ) ? pShapes->size() : 0U;
				size_t reorderTo   = ( pShapes ) ? pShapes->size() : 0U;


				// Find reorder target
				std::string caption;
				std::shared_ptr<Donya::Collision::ShapeBase> pShape = nullptr;

				const size_t count = ( pShapes ) ? pShapes->size() : 0U;
				for ( size_t i = 0; i < count; ++i )
				{
					pShape = pShapes->at( i );
					if ( !pShape ) { continue; }
					// else


					// Name column
					caption = Donya::MakeArraySuffix( i ) + ":" + Donya::Collision::GetShapeName( pShape->GetShapeKind() );
					ImGui::Text( caption.c_str() );
					ImGui::NextColumn();


					// Arrow column
					if ( ImGui::ArrowButton( ( caption + "Up" ).c_str(), ImGuiDir_Up ) )
					{
						reorderFrom = i;
						reorderTo   = std::min( count, reorderFrom - 1 );
					}
					ImGui::SameLine();
					if ( ImGui::ArrowButton( ( caption + "Down" ).c_str(), ImGuiDir_Down ) )
					{
						reorderFrom = i;
						reorderTo   = std::max( 0U, reorderFrom + 1 );
					}
					ImGui::NextColumn();
				}


				// Reorder it if requested(reorderFrom != reorderTo)
				if ( pShapes && reorderFrom != reorderTo && reorderFrom < pShapes->size() )
				{
					auto &from = pShapes->at( reorderFrom );
					auto &to   = pShapes->at( reorderTo   );
					std::swap( from, to );
				}


				ImGui::Columns( 1 ); // Put back to original

				ImGui::EndTabItem();
			}


			//enum GUIMode
			//{
			//	Edit,	// Edit parameter of once
			//	AddSub,	// Append/Erase a shape
			//	Sort,	// Sort shapes
			//};
			//static std::unordered_map<std::string, GUIMode> modeMap;
			//GUIMode &mode = modeMap[bufferId];
			//// Show GUI by mode


			ImGui::EndTabBar();

			ImGui::TreePop();
		}
		void ShowAABBNode	( const std::string &nodeCaption, Donya::Collision::Box3F *p )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragFloat3( u8"中心座標",				&p->pos.x,		0.01f	);
			ImGui::DragFloat3( u8"オフセット",			&p->offset.x,	0.01f	);
			ImGui::DragFloat3( u8"サイズ（半分を指定）",	&p->size.x,		0.01f	);
			ImGui::Checkbox  ( u8"判定を有効にする",		&p->exist				);
			
			if ( ImGui::TreeNode( ( u8"ＩＤ：" + std::to_string( p->id ) ).c_str() ) )
			{
				ImGui::Text( u8"自身＿：%d", p->id );
				ImGui::Text( u8"所有者：%d", p->ownerID );
				
				ShowIgnoreList( u8"無視リスト", p->ignoreList );

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
		void ShowSphereNode	( const std::string &nodeCaption, Donya::Collision::Sphere3F *p )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragFloat3( u8"中心座標",				&p->pos.x,		0.01f	);
			ImGui::DragFloat3( u8"オフセット",			&p->offset.x,	0.01f	);
			ImGui::DragFloat3( u8"半径",					&p->radius,		0.01f	);
			ImGui::Checkbox  ( u8"判定を有効にする",		&p->exist				);

			if ( ImGui::TreeNode( ( u8"ＩＤ：" + std::to_string( p->id ) ).c_str() ) )
			{
				ImGui::Text( u8"自身＿：%d", p->id );
				ImGui::Text( u8"所有者：%d", p->ownerID );
				
				ShowIgnoreList( u8"無視リスト", p->ignoreList );

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
		void ShowFrontNode	( const std::string &nodeCaption, Donya::Quaternion *p, bool freezeUp, bool useTreeNode )
		{
			if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			Donya::Vector3 localFront = p->LocalFront();
			ImGui::SliderFloat3( u8"前方向", &localFront.x, -1.0f, 1.0f );
			*p = Donya::Quaternion::LookAt
			(
				Donya::Vector3::Front(),
				localFront.Unit(),
				( freezeUp )
				? Donya::Quaternion::Freeze::Up
				: Donya::Quaternion::Freeze::None
			);

			if ( ImGui::Button( u8"姿勢をリセット" ) )
			{
				*p = Donya::Quaternion::Identity();
			}

			if ( useTreeNode ) { ImGui::TreePop(); }
		}

		template<typename T, typename ChangeValueMethod>
		void ShowBezierNodeImpl( const std::string &nodeCaption, std::vector<T> *pCtrlPoints, ChangeValueMethod ChangeValueOf )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ResizeByButton( pCtrlPoints, T{} );
			if ( pCtrlPoints->size() < 2 )
			{
				pCtrlPoints->resize( 2 );
			}

			const int pointCount = pCtrlPoints->size();
			std::string caption;
			for ( int i = 0; i < pointCount; ++i )
			{
				caption = Donya::MakeArraySuffix( i );
				ChangeValueOf( caption.c_str(), &pCtrlPoints->at( i ) );
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
			ImGui::SliderFloat( u8"確認用タイマ", &timer, 0.0f, 1.0f );

			T result = Math::CalcBezierCurve( *pCtrlPoints, timer );
			// ImGui::SliderFloat2( u8"ベジェ曲線適用結果", &result.x, rangeMin, rangeMax );
			ChangeValueOf( u8"ベジェ曲線適用結果", &result );

			ImGui::TreePop();
		}
		void ShowBezier1DNode( const std::string &nodeCaption, std::vector<float> *pCtrlPoints, float rangeMin, float rangeMax )
		{
			auto Method = [&]( const char *caption, float *p )
			{
				ImGui::SliderFloat( caption, p, rangeMin, rangeMax );
			};
			ShowBezierNodeImpl( nodeCaption, pCtrlPoints, Method );
		}
		void ShowBezier2DNode( const std::string &nodeCaption, std::vector<Donya::Vector2> *pCtrlPoints, float rangeMin, float rangeMax )
		{
			auto Method = [&]( const char *caption, Donya::Vector2 *p )
			{
				ImGui::SliderFloat2( caption, &p->x, rangeMin, rangeMax );
			};
			ShowBezierNodeImpl( nodeCaption, pCtrlPoints, Method );
		}
		void ShowBezier3DNode( const std::string &nodeCaption, std::vector<Donya::Vector3> *pCtrlPoints, float rangeMin, float rangeMax )
		{
			auto Method = [&]( const char *caption, Donya::Vector3 *p )
			{
				ImGui::SliderFloat3( caption, &p->x, rangeMin, rangeMax );
			};
			ShowBezierNodeImpl( nodeCaption, pCtrlPoints, Method );
		}
	}
}

#endif // USE_IMGUI
