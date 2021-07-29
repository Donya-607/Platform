#include "SceneOver.h"

#include <vector>

#undef max
#undef min

#include "Donya/Color.h"			// Use ClearBackGround(), StartFade().
#include "Donya/CollisionShapes/ShapeAABB.h"
#include "Donya/CollisionShapes/ShapePoint.h"
#include "Donya/CollisionShapes/ShapeSphere.h"
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Common.h"
#include "Effect/EffectAdmin.h"
#include "Fader.h"
#include "FontHelper.h"
#include "Music.h"
#include "Parameter.h"
#include "Player.h"
#include "PlayerParam.h"
#include "RenderingStuff.h"

namespace
{
#if DEBUG_MODE
	constexpr bool IOFromBinary = false;
#else
	constexpr bool IOFromBinary = true;
#endif // DEBUG_MODE

#if USE_IMGUI
	static bool dontTransition = true;
#endif // USE_IMGUI
}

namespace
{
	struct SceneParam
	{
		float waitToFadeSecond = 3.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( waitToFadeSecond ) );

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode()
		{
			ImGui::DragFloat( u8"遷移までの待機秒数", &waitToFadeSecond, 0.01f );
			waitToFadeSecond = std::max( 0.0f, waitToFadeSecond );
		}
	#endif // USE_IMGUI
	};

	static ParamOperator<SceneParam> sceneParam{ "SceneOver" };
	const SceneParam &FetchParameter()
	{
		return sceneParam.Get();
	}
}
CEREAL_CLASS_VERSION( SceneParam, 0 )

void SceneOver::Init()
{
	Player::Remaining::Set( Player::Parameter().Get().initialRemainCount );

	timer = 0.0f;

	{
		posA = { -0.0f, +1.0f, 0.0f };
		posB = { +0.0f, -1.5f, 0.0f };
		using namespace Donya::Collision;
		constexpr float size = 0.5f;
		//auto pPoint			= ShapePoint::Generate( InteractionType::Dynamic );
		auto pAABB			= ShapeAABB::Generate( InteractionType::Dynamic, { size, size, size } );
		//auto pSphere		= ShapeSphere::Generate( InteractionType::Dynamic, size );
		auto pCapsuleTop	= ShapeSphere::Generate( InteractionType::Dynamic, size, { 0.0f, +size, 0.0f } );
		auto pCapsuleBtm	= ShapeSphere::Generate( InteractionType::Dynamic, size, { 0.0f, -size, 0.0f } );
		auto pFloor1		= ShapeAABB::Generate( InteractionType::Dynamic, { size, size, size }, { size*2.0f * 0.0f, 0.0f, 0.0f } );
		auto pFloor2		= ShapeAABB::Generate( InteractionType::Dynamic, { size, size, size }, { size*2.0f * 1.8f, 0.0f, 0.0f } );
		//auto pKinematic		= ShapeAABB::Generate( InteractionType::Kinematic, { size, size, size } );
		auto pSensor		= ShapeAABB::Generate( InteractionType::Sensor, { size, size, size } );
		colA = Collider::Generate();
		//colB = Collider::Generate();
		colB = colA; // duplicate
		pAABB->extraId = 1;
		colA.AddShape( pAABB );
		pSensor->extraId = 9;
		pFloor1->extraId = 9;
		colB.AddShape( pFloor1 );
		pCapsuleTop->extraId = 2;
		colA.AddShape( pCapsuleTop );
		pCapsuleBtm->extraId = 3;
		colA.AddShape( pCapsuleBtm );
		// pFloor2->extraId = 2;
		// colB.AddShape( pFloor2 );
		colA.SetPosition( posA );
		colB.SetPosition( posB );
		colA.SetMass( 1.0f );
		colB.SetMass( 65535.0f );

		colA.RegisterCallback_OnHitEnter	( OnHitEnterA );
		colA.RegisterCallback_OnHitContinue	( OnHitContinueA );
		colA.RegisterCallback_OnHitExit		( OnHitExitA );
		colB.RegisterCallback_OnHitEnter	( OnHitEnterB );
		colB.RegisterCallback_OnHitContinue	( OnHitContinueB );
		colB.RegisterCallback_OnHitExit		( OnHitExitB );

		body.SetPosition( { 42.0f, 42.0f, 42.0f } );
		body.RegisterCallback_OnHitEnter( OnHitEnterB );
		

		/*
		colB.RegisterCallback_OnHitContinue
		(
			[&]( DONYA_CALLBACK_ON_HIT_CONTINUE )
			{
				callbackStrs.emplace_back( "TIMER:" + std::to_string( timer ) );
			}
		);
		*/
		colB.RegisterCallback_OnHitEnter
		(
			[&]( DONYA_CALLBACK_ON_HIT_ENTER )
			{
				callbackStrs.emplace_back( "TIMER:" + std::to_string( timer ) );
			}
		);

		constexpr Donya::Vector2 screenSize{ Common::ScreenWidthF(), Common::ScreenHeightF() };
		constexpr Donya::Vector2 defaultZRange{ 0.1f, 500.0f };
		iCamera.Init			( Donya::ICamera::Mode::Look );
		iCamera.SetZRange		( defaultZRange.x, defaultZRange.y );
		iCamera.SetFOV			( ToRadian( 45.0f ) );
		iCamera.SetScreenSize	( screenSize );
		iCamera.SetPosition		( { 0.0f, 0.0f, -7.0f } );
		iCamera.SetFocusPoint	( { 0.0f, 0.0f, 0.0f } );
		iCamera.SetProjectionPerspective();
		// I can setting a configuration,
		// but current data is not changed immediately.
		// So update here.
		Donya::ICamera::Controller moveInitPoint{};
		moveInitPoint.SetNoOperation();
		moveInitPoint.slerpPercent = 1.0f;
		iCamera.Update( moveInitPoint );
	}


	Effect::Admin::Get().ClearInstances();

	Donya::Sound::Play( Music::BGM_Over );
}
void SceneOver::Uninit()
{
	Donya::Sound::Stop( Music::BGM_Over );

	Effect::Admin::Get().ClearInstances();
}

Scene::Result SceneOver::Update( float elapsedTime )
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F2 ) && !Fader::Get().IsExist() )
	{
		Donya::Sound::Play( Music::DEBUG_Strong );

		StartFade();
	}
#endif // DEBUG_MODE

#if USE_IMGUI
	UseImGui();
#endif // USE_IMGUI

	controller.Update();

	timer += elapsedTime;
	if ( FetchParameter().waitToFadeSecond <= timer )
	{
		if ( !Fader::Get().IsExist() )
		{
		#if USE_IMGUI
			if ( !dontTransition )
		#endif // USE_IMGUI
			StartFade();
		}
	}

#if 0 // ALLOW_SKIP
	if ( !Fader::Get().IsExist() )
	{
		bool shouldSkip = false;
		if ( controller.IsConnected() )
		{
			shouldSkip = controller.Trigger( Donya::Gamepad::Button::A );
		}
		else
		{
			shouldSkip = Donya::Keyboard::Trigger( 'Z' );
		}

		if ( shouldSkip )
		{
			StartFade();
		}
	}
#endif // ALLOW_SKIP


	{
		if ( Donya::Keyboard::Trigger( 'T' ) )
		{
			const bool now = colA.NowIgnoringIntersection();
			colA.SetIgnoringIntersection( !now );
		}
	}
	{
		constexpr float speed = 2.5f;
		Donya::Vector3 velA{};
		if ( Donya::Keyboard::Press( 'A' ) ) { velA.x -= speed; }
		if ( Donya::Keyboard::Press( 'D' ) ) { velA.x += speed; }
		if ( Donya::Keyboard::Press( 'W' ) ) { velA.y += speed; }
		if ( Donya::Keyboard::Press( 'S' ) ) { velA.y -= speed; }
		if ( Donya::Keyboard::Press( 'E' ) ) { velA.z += speed; }
		if ( Donya::Keyboard::Press( 'Q' ) ) { velA.z -= speed; }
		Donya::Vector3 velB{};
		if ( Donya::Keyboard::Press( VK_LEFT   ) ) { velB.x -= speed; }
		if ( Donya::Keyboard::Press( VK_RIGHT  ) ) { velB.x += speed; }
		if ( Donya::Keyboard::Press( VK_UP     ) ) { velB.y += speed; }
		if ( Donya::Keyboard::Press( VK_DOWN   ) ) { velB.y -= speed; }
		if ( Donya::Keyboard::Press( VK_RSHIFT ) ) { velB.z += speed; }
		if ( Donya::Keyboard::Press( VK_END    ) ) { velB.z -= speed; }

		posA += velA * elapsedTime;
		posB += velB * elapsedTime;

		colA.SetPosition( posA );
		colB.SetPosition( posB );
	}
	{
		Donya::Collision::Collider::Resolve();

		posA = colA.GetPosition();
		posB = colB.GetPosition();
	}



	return ReturnResult();
}

void SceneOver::Draw( float elapsedTime )
{
	ClearBackGround();

	/*
	Call Effect::Admin::Draw() if you use it
	*/

#if DEBUG_MODE
	const Donya::Vector4x4 V = iCamera.CalcViewMatrix();
	const Donya::Vector4x4 VP = V * iCamera.GetProjectionMatrix();

	RenderingStuff *p = RenderingStuffInstance::Get().Ptr();
	if ( Common::IsShowCollision() && p )
	{
		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= VP;
		constant.lightDirection	= -Donya::Vector3::Up();

		auto DrawProcess = [&]( const Donya::Collision::Collider &col, const Donya::Vector4 &color )
		{
			constant.drawColor = color;

			constexpr float pointSize = 0.05f;

			const auto *shapePtrs = col.GetAddedShapePointers();
			for ( const auto &pShape : *shapePtrs )
			{
				if ( !pShape ) { continue; }
				// else

				const Donya::Vector3 pos = pShape->GetPosition();
				constant.matWorld._41 = pos.x;
				constant.matWorld._42 = pos.y;
				constant.matWorld._43 = pos.z;

				using namespace Donya::Collision;
				switch ( pShape->GetShapeKind() )
				{
				case Shape::Point:
					{
						constant.matWorld._11 = pointSize;
						constant.matWorld._22 = pointSize;
						constant.matWorld._33 = pointSize;
						p->renderer.ProcessDrawingCube( constant );
					}
					break;
				case Shape::AABB:
					{
						auto pAABB = dynamic_cast<const ShapeAABB *>( pShape.get() );
						if ( pAABB )
						{
							const Donya::Vector3 size = pAABB->GetSize();
							constant.matWorld._11 = size.x * 2.0f;
							constant.matWorld._22 = size.y * 2.0f;
							constant.matWorld._33 = size.z * 2.0f;
							p->renderer.ProcessDrawingCube( constant );
						}
					}
					break;
				case Shape::Sphere:
					{
						auto pSphere = dynamic_cast<const ShapeSphere *>( pShape.get() );
						if ( pSphere )
						{
							const float radius = pSphere->GetRadius();
							constant.matWorld._11 = radius * 2.0f;
							constant.matWorld._22 = radius * 2.0f;
							constant.matWorld._33 = radius * 2.0f;
							p->renderer.ProcessDrawingSphere( constant );
						}
					}
					break;
				default:
					break;
				}
			}
		};

		constexpr float alpha = 0.6f;
		constexpr Donya::Vector4 red	{ 1.0f, 0.0f, 0.0f, alpha };
		constexpr Donya::Vector4 green	{ 0.0f, 1.0f, 0.0f, alpha };
		constexpr Donya::Vector4 blue	{ 0.0f, 0.0f, 1.0f, alpha };
		DrawProcess( colA, red );
		DrawProcess( colB, green );
	}
#endif // DEBUG_MODE

	const auto pFontRenderer = FontHelper::GetRendererOrNullptr( FontAttribute::Main );
	if ( pFontRenderer )
	{
		constexpr Donya::Vector2 pivot { 0.5f, 0.5f };
		constexpr Donya::Vector2 center{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };
		constexpr Donya::Vector4 color { Donya::Color::MakeColor( Donya::Color::Code::LIGHT_GRAY ), 1.0f };
		constexpr Donya::Vector2 scale { 4.0f, 4.0f };

		pFontRenderer->DrawExt( L"GAME OVER", center, pivot, scale, color );
		Donya::Sprite::Flush();
	}
}

void SceneOver::ClearBackGround() const
{
	constexpr Donya::Vector3 gray = Donya::Color::MakeColor( Donya::Color::Code::GRAY );
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );
}
void SceneOver::StartFade()
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeSecond	= Fader::GetDefaultCloseSecond();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneOver::ReturnResult()
{
	if ( Fader::Get().IsClosed() )
	{
		Scene::Type next = Scene::Type::Over;
		if ( Donya::Keyboard::Press( VK_CONTROL ) )
		{
			next = Scene::Type::Title;
		}

		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = next;
		return change;
	}

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
namespace
{
	constexpr const char *filePathBin  = "./Data/foobarColliders.bin";
	constexpr const char *filePathJson = "./Data/foobarColliders.json";
	constexpr const char *objectName   = "Colliders_AandB";
}
#include "Donya/Serializer.h"
void SceneOver::Save()
{
	Donya::Serializer tmp;
	tmp.SaveBinary	( *this, filePathBin,  objectName );
	tmp.SaveJSON	( *this, filePathJson, objectName );
}
void SceneOver::LoadBin()
{
	Donya::Serializer tmp;
	tmp.LoadBinary( *this, filePathBin, objectName );
}
void SceneOver::LoadJson()
{
	Donya::Serializer tmp;
	tmp.LoadJSON( *this, filePathJson, objectName );
}
void SceneOver::UseImGui()
{
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else
	
	sceneParam.ShowImGuiNode( u8"ゲームオーバーシーンのパラメータ" );

	ImGui::Checkbox( u8"遷移を止める", &dontTransition );

	using namespace Donya::Collision;
	constexpr float shapeSize = 0.5f;
	constexpr Donya::Vector3 shapeSize3{ shapeSize, shapeSize, shapeSize };
	constexpr InteractionType shapeTypes[]{ InteractionType::Dynamic, InteractionType::Kinematic, InteractionType::Sensor };
	constexpr int shapeTypeCount = ArraySize( shapeTypes );
	if ( ImGui::TreeNode( u8"変更とシリアライズ" ) )
	{
		using Type = InteractionType;

		static int typeA = scast<int>( Type::Dynamic );
		static int typeB = scast<int>( Type::Dynamic );
		static int kindA = 1; // 0:Point, 1:AABB, 2:Sphere
		static int kindB = 1; // 0:Point, 1:AABB, 2:Sphere
		constexpr int shapeKindCount = 3;
		static Donya::Vector3 sizeA = shapeSize3;
		static Donya::Vector3 sizeB = shapeSize3;
		static Donya::Vector3 offsetA{};
		static Donya::Vector3 offsetB{};
		auto Show = [&]( std::string colName, Collider *pCol, int *pType, int *pKind, Donya::Vector3 *pSize, Donya::Vector3 *pOffset )
		{
			if ( !ImGui::TreeNode( ( u8"変更：" + colName ).c_str() ) ) { return; }
			// else


			// Type
			static const char *shapeTypeNames[]{ "Dynamic", "Kinematic", "Sensor" };
			for ( int i = 0; i < shapeTypeCount; ++i )
			{
				if ( ImGui::RadioButton( shapeTypeNames[i], *pType == i ) )
				{
					*pType = i;
				}

				if ( i + 1 < shapeTypeCount )
				{
					ImGui::SameLine();
				}
			}
			
			
			// Kind
			static const char *shapeKindNames[]{ "Point", "AABB", "Sphere" };
			for ( int i = 0; i < shapeKindCount; ++i )
			{
				if ( ImGui::RadioButton( shapeKindNames[i], *pKind == i ) )
				{
					*pKind = i;
				}

				if ( i + 1 < shapeKindCount )
				{
					ImGui::SameLine();
				}
			}


			// Size
			ImGui::DragFloat3( u8"Size", &pSize->x, 0.1f );
			
			
			// Offset
			ImGui::DragFloat3( u8"Offset", &pOffset->x, 0.05f );


			// Assign
			if ( ImGui::Button( u8"再設定" ) )
			{
				pCol->RemoveAllShapes();

				switch ( *pKind )
				{
				case 0:
					pCol->AddShape
					(
						ShapePoint::Generate
						(
							scast<Type>( *pType ),
							*pOffset
						)
					);
					break;
				case 1:
					pCol->AddShape
					(
						ShapeAABB::Generate
						(
							scast<Type>( *pType ),
							*pSize,
							*pOffset
						)
					);
					break;
				case 2:
					pCol->AddShape
					(
						ShapeSphere::Generate
						(
							scast<Type>( *pType ),
							pSize->x,
							*pOffset
						)
					);
					break;
				default: assert( 0 ); break;
				}
			}


			ImGui::TreePop();
		};

		Show( u8"A-赤", &colA, &typeA, &kindA, &sizeA, &offsetA );
		Show( u8"B-緑", &colB, &typeB, &kindB, &sizeB, &offsetB );

		if ( ImGui::Button( u8"保存" ) )
		{
			Save();
		}
		if ( ImGui::Button( u8"読み込み-Bin" ) )
		{
			LoadBin();
			posA = colA.GetPosition();
			posB = colB.GetPosition();
		}
		if ( ImGui::Button( u8"読み込み-Json" ) )
		{
			LoadJson();
			posA = colA.GetPosition();
			posB = colB.GetPosition();
		}

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"コールバックログ" ) )
	{
		ImGui::Text( u8"[O]:Other, [M]:Myself\n[T]:interaction type, [S]:shape kind\n[Ex]:extra id\n" );

		if ( ImGui::Button( u8"ログを消去" ) )
		{
			callbackStrs.clear();
		}

		for ( const auto &str : callbackStrs )
		{
			ImGui::Text( str.c_str() );
		}

		ImGui::TreePop();
	}

	ImGui::End();
}

std::vector<std::string> SceneOver::callbackStrs{};
bool SceneOver::hitContinuingA = false;
bool SceneOver::hitContinuingB = false;
void AppendCallbackDescriptions( std::string *pTarget, const Donya::Collision::Body &other, const std::shared_ptr<Donya::Collision::ShapeBase> &pOtherShape, const Donya::Collision::Body &myself, const std::shared_ptr<Donya::Collision::ShapeBase> &pMyselfShape )
{
	if ( !pTarget ) { return; }
	// else

	*pTarget += "O-id:"	+ std::to_string( other.GetId()									) + ",";
	*pTarget += "O-T:"	+ std::to_string( scast<int>( pOtherShape->GetType() )			) + ",";
	*pTarget += "O-S:"	+ std::to_string( scast<int>( pOtherShape->GetShapeKind() )		) + ";";
	*pTarget += "O-Ex:"	+ std::to_string( scast<int>( pOtherShape->GetId() )			) + ";";
	*pTarget += " ";
	*pTarget += "M-id:"	+ std::to_string( myself.GetId()								) + ",";
	*pTarget += "M-T:"	+ std::to_string( scast<int>( pMyselfShape->GetType() )			) + ",";
	*pTarget += "M-S:"	+ std::to_string( scast<int>( pMyselfShape->GetShapeKind() )	) + ";";
	*pTarget += "M-Ex:"	+ std::to_string( scast<int>( pMyselfShape->GetId() )			) + ";";
}
void SceneOver::OnHitEnterA( DONYA_CALLBACK_ON_HIT_ENTER )
{
	std::string tmp = "Enter[A]: ";
	AppendCallbackDescriptions( &tmp, hitOther, pHitOtherShape, hitMyself, pHitMyselfShape );
	callbackStrs.emplace_back( std::move( tmp ) );
}
void SceneOver::OnHitContinueA( DONYA_CALLBACK_ON_HIT_CONTINUE )
{
	if ( hitContinuingA ) { return; }
	// else
	hitContinuingA = true;

	std::string tmp = "Continue[A]: ";
	AppendCallbackDescriptions( &tmp, hitOther, pHitOtherShape, hitMyself, pHitMyselfShape );
	callbackStrs.emplace_back( std::move( tmp ) );
}
void SceneOver::OnHitExitA( DONYA_CALLBACK_ON_HIT_EXIT )
{
	hitContinuingA = false;

	std::string tmp = "Exit[A]: ";
	AppendCallbackDescriptions( &tmp, leaveOther, pLeaveOtherShape, leaveMyself, pLeaveMyselfShape );
	callbackStrs.emplace_back( std::move( tmp ) );
}
void SceneOver::OnHitEnterB( DONYA_CALLBACK_ON_HIT_ENTER )
{
	std::string tmp = "Enter[B]: ";
	AppendCallbackDescriptions( &tmp, hitOther, pHitOtherShape, hitMyself, pHitMyselfShape );
	callbackStrs.emplace_back( std::move( tmp ) );
}
void SceneOver::OnHitContinueB( DONYA_CALLBACK_ON_HIT_CONTINUE )
{
	if ( hitContinuingB ) { return; }
	// else
	hitContinuingB = true;

	std::string tmp = "Continue[B]: ";
	AppendCallbackDescriptions( &tmp, hitOther, pHitOtherShape, hitMyself, pHitMyselfShape );
	callbackStrs.emplace_back( std::move( tmp ) );
}
void SceneOver::OnHitExitB( DONYA_CALLBACK_ON_HIT_EXIT )
{
	hitContinuingB = false;

	std::string tmp = "Exit[B]: ";
	AppendCallbackDescriptions( &tmp, leaveOther, pLeaveOtherShape, leaveMyself, pLeaveMyselfShape );
	callbackStrs.emplace_back( std::move( tmp ) );
}
#endif // USE_IMGUI
