#include "SceneTitle.h"

#include <vector>

#include "Donya/Blend.h"
#include "Donya/Color.h"			// Use ClearBackGround(), StartFade().
#include "Donya/Keyboard.h"			// Make an input of player.
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/Mouse.h"
#include "Donya/GeometricPrimitive.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Bloom.h"
#include "Bullet.h"
#include "Common.h"
#include "Enemy.h"
#include "Fader.h"
#include "FilePath.h"
#include "Item.h"
#include "ModelHelper.h"			// Use serialize methods
#include "Music.h"
#include "Parameter.h"
#include "PointLightStorage.h"
#include "StageNumber.h"

#undef max
#undef min

namespace
{
	constexpr const char *GetChoiceItemName( SceneTitle::Choice item )
	{
		switch ( item )
		{
		case SceneTitle::Choice::Start:		return "START";
		case SceneTitle::Choice::Option:	return "OPTION";
		default: break;
		}

		// Fail safe
		return GetChoiceItemName( SceneTitle::Choice::Start );
	}

	struct Member
	{
		struct
		{
			float slerpFactor	= 0.2f;
			float fovDegree		= 30.0f;
			Donya::Vector3 offsetPos{ 0.0f, 5.0f, -10.0f };	// The offset of position from the player position
			Donya::Vector3 offsetFocus;						// The offset of focus from the player position
		}
		camera;
		
		Donya::Model::Constants::PerScene::DirectionalLight directionalLight;

		float scrollTakeSecond	= 1.0f;	// The second required for scrolling
		
		struct ShadowMap
		{
			Donya::Vector3	color;			// RGB
			float			bias = 0.03f;	// Ease an acne

			Donya::Vector3	posOffset;		// From the camera position
			Donya::Vector3	projectDirection{  0.0f,  0.0f,  1.0f };
			Donya::Vector3	projectDistance { 10.0f, 10.0f, 50.0f };// [m]
			float			nearDistance = 1.0f;					// Z near is this. Z far is projectDistance.z.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( color			),
					CEREAL_NVP( bias			),
					CEREAL_NVP( posOffset		),
					CEREAL_NVP( projectDirection),
					CEREAL_NVP( projectDistance	),
					CEREAL_NVP( nearDistance	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		ShadowMap shadowMap;

		BloomApplier::Parameter bloomParam;

		std::vector<Donya::Vector2> itemPositions; // size() == SceneTitle::Choice::ItemCount
		float chooseItemMagni = 1.5f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( camera.slerpFactor	),
				CEREAL_NVP( camera.fovDegree	),
				CEREAL_NVP( camera.offsetPos	),
				CEREAL_NVP( camera.offsetFocus	),

				CEREAL_NVP( directionalLight	),
				CEREAL_NVP( scrollTakeSecond	),
				CEREAL_NVP( shadowMap			),
				CEREAL_NVP( bloomParam			)
			);

			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( itemPositions	),
					CEREAL_NVP( chooseItemMagni	)
				);
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode()
		{
			if ( ImGui::TreeNode( u8"カメラ" ) )
			{
				ImGui::DragFloat ( u8"補間倍率",						&camera.slerpFactor,	0.01f );
				ImGui::DragFloat ( u8"画角（Degree）",				&camera.fovDegree,		0.1f  );
				ImGui::DragFloat3( u8"自身の座標（自機からの相対）",	&camera.offsetPos.x,	0.01f );
				ImGui::DragFloat3( u8"注視点の座標（自機からの相対）",	&camera.offsetFocus.x,	0.01f );

				ImGui::TreePop();
			}
			
			ImGui::Helper::ShowDirectionalLightNode( u8"平行光", &directionalLight );

			if ( ImGui::TreeNode( u8"シャドウマップ関連" ) )
			{
				ImGui::ColorEdit3( u8"影の色",						&shadowMap.color.x );
				ImGui::DragFloat ( u8"アクネ用のバイアス",			&shadowMap.bias,				0.01f );
				ImGui::DragFloat3( u8"光源の座標（カメラからの相対）",	&shadowMap.posOffset.x,			1.0f  );
				ImGui::DragFloat3( u8"写す方向（単位ベクトル）",		&shadowMap.projectDirection.x,	0.01f );
				ImGui::DragFloat3( u8"写す範囲ＸＹＺ",				&shadowMap.projectDistance.x,	1.0f  );
				ImGui::DragFloat ( u8"Z-Near",						&shadowMap.nearDistance,		1.0f  );

				shadowMap.projectDistance.x = std::max( 0.1f, shadowMap.projectDistance.x );
				shadowMap.projectDistance.y = std::max( 0.1f, shadowMap.projectDistance.y );
				shadowMap.projectDistance.z = std::max( shadowMap.nearDistance + 1.0f, shadowMap.projectDistance.z );

				static bool alwaysNormalize = false;
				if ( ImGui::Button( u8"写す方向を正規化" ) || alwaysNormalize )
				{
					shadowMap.projectDirection.Normalize();
				}
				ImGui::Checkbox( u8"常に正規化する", &alwaysNormalize );

				ImGui::TreePop();
			}

			bloomParam.ShowImGuiNode( u8"ブルーム関連" );

			if ( ImGui::TreeNode( u8"秒数関連" ) )
			{
				ImGui::DragFloat( u8"スクロールに要する秒数", &scrollTakeSecond, 0.01f );
				scrollTakeSecond = std::max( 0.01f, scrollTakeSecond );

				ImGui::TreePop();
			}

			constexpr size_t itemCount = scast<size_t>( SceneTitle::Choice::ItemCount );
			if ( itemPositions.size() != itemCount )
			{
				constexpr Donya::Vector2 center{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };
				itemPositions.resize( itemCount, center );
			}
			if ( ImGui::TreeNode( u8"項目関連" ) )
			{
				ImGui::DragFloat( u8"選択物の拡大率", &chooseItemMagni, 0.01f );
				if ( ImGui::TreeNode( u8"中心座標" ) )
				{
					std::string caption{};
					for ( size_t i = 0; i < itemCount; ++i )
					{
						caption = GetChoiceItemName( scast<SceneTitle::Choice>( i ) );
						ImGui::DragFloat2( caption.c_str(), &itemPositions[i].x );
					}
					
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
		}
	#endif // USE_IMGUI
	};

	static ParamOperator<Member> sceneParam{ "SceneTitle" };
	Member FetchParameter()
	{
		return sceneParam.Get();
	}

#if DEBUG_MODE
	constexpr unsigned int maxPathBufferSize = MAX_PATH;
	std::string FetchStageFilePathByCommonDialog()
	{
		char chosenFullPaths[maxPathBufferSize] = { 0 };
		char chosenFileName [maxPathBufferSize] = { 0 };

		OPENFILENAMEA ofn{ 0 };
		ofn.lStructSize		= sizeof( decltype( ofn ) );
		ofn.hwndOwner		= Donya::GetHWnd();
		ofn.lpstrFilter		= "CSV-file(*.csv)\0*.csv\0"
							  "\0";
		ofn.lpstrFile		= chosenFullPaths;
		ofn.nMaxFile		= maxPathBufferSize;
		ofn.lpstrFileTitle	= chosenFileName;
		ofn.nMaxFileTitle	= maxPathBufferSize;
		ofn.Flags			= OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR; // Prevent the current directory of this application will be changed.

		auto  result = GetOpenFileNameA( &ofn );
		if ( !result ) { return std::string{}; }
		// else

		return std::string{ ofn.lpstrFile };
	}
#endif // DEBUG_MODE
}
CEREAL_CLASS_VERSION( Member, 0 )

void SceneTitle::Init()
{
	// Donya::Sound::Play( Music::BGM_Title );
#if DEBUG_MODE
	// Donya::Sound::AppendFadePoint( Music::BGM_Title, 2.0f, 0.0f, true ); // Too noisy.
#endif // DEBUG_MODE

	sceneParam.LoadParameter();

	constexpr int stageNo = Definition::StageNumber::Title();
	constexpr Donya::Int2 wholeScreenSize
	{
		Common::ScreenWidth(),
		Common::ScreenHeight(),
	};

	bool result{};

	{
		result = CreateRenderers( wholeScreenSize );
		assert( result );

		result = CreateSurfaces( wholeScreenSize );
		assert( result );

		result = CreateShaders();
		assert( result );

		pPlayerIniter = std::make_unique<PlayerInitializer>();
		pPlayerIniter->LoadParameter( stageNo );

		using Spr = SpriteAttribute;
		result = sprTitleLogo.LoadSprite( GetSpritePath( Spr::TitleLogo ), GetSpriteInstanceCount( Spr::TitleLogo ) );
		assert( result );
	}

	{
		/*
		The dependencies of initializations:
		CurrentRoomID	[House, Player]
		House - it is free
		Enemies:		[Map]				- depends on the map for decide the initial state
		Map:			[CurrentScreen]		- depends on an area of current screen the camera projects
		CurrentScreen:	[Camera]			- depends on the view and projection matrix of the camera
		Camera:			[Player, House]		- depends on the player position(camera) and room position(light camera)
		Player - it is free(currently)
		*/

		// Initialize a dependent objects

		pHouse = std::make_unique<House>();
		pHouse->Init( stageNo );

		PlayerInit();

		currentRoomID = CalcCurrentRoomID();

		// The calculation of camera position depends on the player's position
		CameraInit();
		// The calculation of screen space uses camera's view and projection matrix, so must calc it after CameraInit().
		currentScreen = CalcCurrentScreenPlane();

		pMap = std::make_unique<Map>();
		pMap->Init( stageNo );

		auto &enemyAdmin = Enemy::Admin::Get();
		enemyAdmin.ClearInstances();
		// enemyAdmin.LoadEnemies( stageNo, currentScreen, IOFromBinary );
	#if DEBUG_MODE
		// enemyAdmin.SaveEnemies( stageNo, true );
	#endif // DEBUG_MODE

		// Initialize a non-dependent objects

		Bullet::Admin::Get().ClearInstances();

		auto &itemAdmin = Item::Admin::Get();
		itemAdmin.ClearInstances();
		// itemAdmin.LoadItems( stageNo, IOFromBinary );
	#if DEBUG_MODE
		// itemAdmin.SaveItems( stageNo, true );
	#endif // DEBUG_MODE
	}
}
void SceneTitle::Uninit()
{
	//Donya::Sound::Stop( Music::BGM_Title );

	if ( pMap		) { pMap->Uninit();		}
	if ( pHouse		) { pHouse->Uninit();	}
	if ( pPlayer	) { pPlayer->Uninit();	}
	pMap.reset();
	pHouse.reset();
	pPlayer.reset();

	Bullet::Admin::Get().ClearInstances();
	Enemy::Admin::Get().ClearInstances();
	Item::Admin::Get().ClearInstances();
}

Scene::Result SceneTitle::Update( float elapsedTime )
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F5 ) )
	{
		nowDebugMode = !nowDebugMode;

		if ( nowDebugMode )
		{
			iCamera.ChangeMode( Donya::ICamera::Mode::Free );
		}
		else
		{
			iCamera.SetOrientation( Donya::Quaternion::Identity() );
			CameraInit();
		}
	}
#endif // DEBUG_MODE

#if USE_IMGUI
	UseImGui();

	// Apply for be able to see an adjustment immediately
	{
		if ( pBloomer ) { pBloomer->AssignParameter( FetchParameter().bloomParam ); }
	}
#endif // USE_IMGUI

	PointLightStorage::Get().Clear();

	elapsedSecond += elapsedTime;

	controller.Update();

	const float deltaTimeForMove = ( scroll.active ) ? 0.0f : elapsedTime;

	if ( pMap ) { pMap->Update( deltaTimeForMove ); }
	const Map emptyMap{}; // Used for empty argument. Fali safe.
	const Map &mapRef = ( pMap ) ? *pMap : emptyMap;

	const int oldRoomID = currentRoomID;
	if ( scroll.active )
	{
		// Update the roomID if the game is pausing for the scroll.
		// That limit prevents the camera moves to not allowed direction.
		currentRoomID = CalcCurrentRoomID();
	}

	const Room *pCurrentRoom = pHouse->FindRoomOrNullptr( currentRoomID );
	
	PlayerUpdate( deltaTimeForMove, mapRef );

	const Donya::Vector3 playerPos = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3::Zero();
	Bullet::Admin::Get().Update( deltaTimeForMove, currentScreen );
	Enemy::Admin::Get().Update( deltaTimeForMove, playerPos, currentScreen );
	Item::Admin::Get().Update( deltaTimeForMove, currentScreen );

	// PhysicUpdates
	{
		using Dir = Definition::Direction;

		const auto currentRoomArea	= ( pCurrentRoom )
									? pHouse->CalcRoomArea( currentRoomID )
									: currentScreen; // Fail safe
		const auto transitionable	= ( pCurrentRoom )
									? pCurrentRoom->GetTransitionableDirection()
									: Dir::Nil;

		if ( pPlayer )
		{
			const float leftBorder	= Contain( transitionable, Dir::Left )
									? -FLT_MAX
									: currentRoomArea.Min().x;
			const float rightBorder	= Contain( transitionable, Dir::Right )
									? FLT_MAX
									: currentRoomArea.Max().x;
			pPlayer->PhysicUpdate( deltaTimeForMove, mapRef, leftBorder, rightBorder );

			const int movedRoomID = CalcCurrentRoomID();
			if ( movedRoomID != currentRoomID )
			{
				// RoomID will update in next frame of the game.
				// But if the scroll is not allowed, stay in the current room.

				// If allow the scroll to a connecting room, the scroll waiting will occur as strange.
				if ( pCurrentRoom && !pCurrentRoom->IsConnectTo( movedRoomID ) )
				{
					bool scrollToUp		= false;
					bool scrollToDown	= false;
					{
						const auto movedRoomArea = pHouse->CalcRoomArea( movedRoomID );
						if ( currentRoomArea.pos.y < movedRoomArea.pos.y )
						{
							scrollToUp		= true;
						}
						if ( currentRoomArea.pos.y > movedRoomArea.pos.y )
						{
							scrollToDown	= true;
						}
					}

					if ( scrollToUp )
					{
						if ( pPlayer->NowGrabbingLadder() && Contain( transitionable, Dir::Up ) )
						{
							PrepareScrollIfNotActive( currentRoomID, movedRoomID );
						}
					}
					else
					if ( scrollToDown )
					{
						if ( Contain( transitionable, Dir::Down ) )
						{
							PrepareScrollIfNotActive( currentRoomID, movedRoomID );
						}
					}
					else
					{
						PrepareScrollIfNotActive( currentRoomID, movedRoomID );
					}
				}
			}
		}

		Bullet::Admin::Get().PhysicUpdate( deltaTimeForMove, mapRef );
		Enemy::Admin::Get().PhysicUpdate( deltaTimeForMove, mapRef );
		Item::Admin::Get().PhysicUpdate( deltaTimeForMove, mapRef );
	}

	// CameraUpdate() depends the currentScreen, so I should update that before CameraUpdate().
	currentScreen = CalcCurrentScreenPlane();
	CameraUpdate( elapsedTime );


	return ReturnResult();
}

void SceneTitle::Draw( float elapsedTime )
{
	ClearBackGround();

	if ( !AreRenderersReady() ) { return; }
	// else

	auto UpdateSceneConstant	= [&]( const Donya::Model::Constants::PerScene::DirectionalLight &directionalLight, const Donya::Vector3 &eyePos, const Donya::Vector4x4 &viewProjectionMatrix )
	{
		Donya::Model::Constants::PerScene::Common constant{};
		constant.directionalLight	= directionalLight;
		constant.eyePosition		= Donya::Vector4{ eyePos, 1.0f };
		constant.viewProjMatrix		= viewProjectionMatrix;
		pRenderer->UpdateConstant( constant );
	};
	auto DrawObjects			= [&]( bool castShadow )
	{
		// The drawing priority is determined by the priority of the information.

		( castShadow )
		? pRenderer->ActivateShaderShadowStatic()
		: pRenderer->ActivateShaderNormalStatic();

		if ( pMap ) { pMap->Draw( pRenderer.get() ); }

		( castShadow )
		? pRenderer->DeactivateShaderShadowStatic()
		: pRenderer->DeactivateShaderNormalStatic();


		( castShadow )
		? pRenderer->ActivateShaderShadowSkinning()
		: pRenderer->ActivateShaderNormalSkinning();

		Bullet::Admin::Get().Draw( pRenderer.get() );
		if ( pPlayer ) { pPlayer->Draw( pRenderer.get() ); }
		Enemy::Admin::Get().Draw( pRenderer.get() );
		Item::Admin::Get().Draw( pRenderer.get() );

		( castShadow )
		? pRenderer->DeactivateShaderShadowSkinning()
		: pRenderer->DeactivateShaderNormalSkinning();
	};

	const Donya::Vector4x4 VP  = iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix();
	const Donya::Vector4x4 LVP = CalcLightViewProjectionMatrix();
	const auto &data = FetchParameter();

	Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLess );
	Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullBack_CCW );
	pRenderer->ActivateSamplerModel( Donya::Sampler::Defined::Aniso_Wrap );


	pShadowMap->SetRenderTarget();
	pShadowMap->SetViewport();
	// Update scene constant as light source
	{
		Donya::Model::Constants::PerScene::DirectionalLight tmpDirLight{};
		tmpDirLight.direction = Donya::Vector4{ data.shadowMap.projectDirection.Unit(), 0.0f };
		UpdateSceneConstant( tmpDirLight, lightCamera.GetPosition(), LVP );
	}
	// Make the shadow map
	{
		pRenderer->ActivateConstantScene();
		DrawObjects( /* castShadow = */ true );
		pRenderer->DeactivateConstantScene();
	}
	Donya::Surface::ResetRenderTarget();


	pScreenSurface->SetRenderTarget();
	pScreenSurface->SetViewport();

	// Update scene and shadow constants
	{
		UpdateSceneConstant( data.directionalLight, iCamera.GetPosition(), VP );

		RenderingHelper::ShadowConstant constant{};
		constant.lightProjMatrix	= LVP;
		constant.shadowColor		= data.shadowMap.color;
		constant.shadowBias			= data.shadowMap.bias;
		pRenderer->UpdateConstant( constant );
	}
	// Update point light constant
	{
		pRenderer->UpdateConstant( PointLightStorage::Get().GetStorage() );
	}
	// Draw normal scene with shadow map
	{
		pRenderer->ActivateConstantScene();
		pRenderer->ActivateConstantPointLight();
		pRenderer->ActivateConstantShadow();
		pRenderer->ActivateSamplerShadow( Donya::Sampler::Defined::Point_Border_White );
		pRenderer->ActivateShadowMap( *pShadowMap );

		DrawObjects( /* castShadow = */ false );

		pRenderer->DeactivateShadowMap( *pShadowMap );
		pRenderer->DeactivateSamplerShadow();
		pRenderer->DeactivateConstantShadow();
		pRenderer->DeactivateConstantPointLight();
		pRenderer->DeactivateConstantScene();
	}
	
	pRenderer->DeactivateSamplerModel();
	Donya::Rasterizer::Deactivate();
	Donya::DepthStencil::Deactivate();

	// Draw sprites
	{
		sprTitleLogo.pos.x = Common::HalfScreenWidthF();
		sprTitleLogo.pos.y = Common::HalfScreenHeightF();
		sprTitleLogo.alpha = 1.0f;
		sprTitleLogo.Draw( 0.0f );
		Donya::Sprite::Flush();
	}

	Donya::Surface::ResetRenderTarget();

	// Generate the buffers of bloom
	{
		constexpr Donya::Vector4 black{ 0.0f, 0.0f, 0.0f, 1.0f };
		pBloomer->ClearBuffers( black );

		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLessEq );
		Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullNone );

		Donya::Sampler::SetPS( Donya::Sampler::Defined::Linear_Border_Black, 0 );
		pBloomer->WriteLuminance( *pScreenSurface );
		Donya::Sampler::ResetPS( 0 );

		Donya::Sampler::SetPS( Donya::Sampler::Defined::Aniso_Wrap, 0 );
		pBloomer->WriteBlur();
		Donya::Sampler::ResetPS( 0 );

		Donya::Rasterizer::Deactivate();
		Donya::DepthStencil::Deactivate();
	}

	pScreenSurface->SetRenderTarget();
	pScreenSurface->SetViewport();
	// Draw a fonts
	{
		Donya::Sprite::SetDrawDepth( 0.0f );
		constexpr Donya::Vector2 pivot{ 0.5f, 0.5f };
		constexpr Donya::Vector4 selectColor	{ 1.0f, 1.0f, 1.0f, 1.0f };
		constexpr Donya::Vector4 unselectColor	{ 0.4f, 0.4f, 0.4f, 1.0f };
		const float &selectScale  = data.chooseItemMagni;
		const float unselectScale = 1.0f;

		auto Draw = [&]( const wchar_t *itemName, Choice item, bool selected )
		{
			// pFontRenderer->DrawExt( str, pos, pivot, scale, color );

			std::wstring string =
				( selected )
				? L"＞"
				: L"　";
			string += itemName;

			const int itemIndex = scast<int>( item );
			const Donya::Vector2 pos =
				( scast<int>( data.itemPositions.size() ) <= itemIndex )
				? Donya::Vector2::Zero()
				: data.itemPositions[itemIndex];

			const Donya::Vector2 &scale =
				( selected )
				? selectScale
				: unselectScale;

			const Donya::Vector4 &color =
				( selected )
				? selectColor
				: unselectColor;

			pFontRenderer->DrawExt( string, pos, pivot, scale, color );
		};

		Draw( L"ＳＴＡＲＴ",		Choice::Start,	( chooseItem == Choice::Start  ) );
		Draw( L"ＯＰＴＩＯＮ",	Choice::Option,	( chooseItem == Choice::Option ) );
	
		Donya::Sprite::Flush();
	}
	Donya::Surface::ResetRenderTarget();

	Donya::SetDefaultRenderTargets();

	const Donya::Vector2 screenSurfaceSize = pScreenSurface->GetSurfaceSizeF();

	Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLessEq );
	Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullNone );
	
	Donya::Sampler::SetPS( Donya::Sampler::Defined::Aniso_Wrap, 0 );
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA );
	// Draw the scene to screen
	{
		pQuadShader->VS.Activate();
		pQuadShader->PS.Activate();

		pScreenSurface->SetRenderTargetShaderResourcePS( 0U );

		pDisplayer->Draw
		(
			screenSurfaceSize,
			Donya::Vector2::Zero()
		);

		pScreenSurface->ResetShaderResourcePS( 0U );

		pQuadShader->PS.Deactivate();
		pQuadShader->VS.Deactivate();
	}
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );
	Donya::Sampler::ResetPS( 0 );

	// Add the bloom buffers
	Donya::Blend::Activate( Donya::Blend::Mode::ADD_NO_ATC );
	pBloomer->DrawBlurBuffers( screenSurfaceSize );
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );

	Donya::Rasterizer::Deactivate();
	Donya::DepthStencil::Deactivate();

#if DEBUG_MODE
	// Object's hit/hurt boxes
	if ( Common::IsShowCollision() )
	{
		if ( pPlayer	) { pPlayer->DrawHitBox( pRenderer.get(), VP );					}
		if ( pMap		) { pMap->DrawHitBoxes( currentScreen, pRenderer.get(), VP );	}
		Bullet::Admin::Get().DrawHitBoxes( pRenderer.get(), VP );
		Enemy ::Admin::Get().DrawHitBoxes( pRenderer.get(), VP );
		Item  ::Admin::Get().DrawHitBoxes( pRenderer.get(), VP );
		if ( pHouse		) { pHouse->DrawHitBoxes( pRenderer.get(), VP );				}
	}
#endif // DEBUG_MODE

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		static Donya::Geometric::Line line{ 512U };
		static bool shouldInitializeLine = true;
		if ( shouldInitializeLine )
		{
			shouldInitializeLine = false;
			line.Init();
		}

		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= VP;
		constant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 0.6f };
		constant.lightDirection	= data.directionalLight.direction.XYZ();
		
		auto DrawCube = [&]( const Donya::Vector3 &pos, const Donya::Vector3 &scale = { 1.0f, 1.0f, 1.0f }, const Donya::Quaternion &rotation = Donya::Quaternion::Identity() )
		{
			constant.matWorld = Donya::Vector4x4::Identity();
			constant.matWorld._11 = scale.x * 2.0f;
			constant.matWorld._22 = scale.y * 2.0f;
			constant.matWorld._33 = scale.z * 2.0f;
			constant.matWorld *= rotation.MakeRotationMatrix();
			constant.matWorld._41 = pos.x;
			constant.matWorld._42 = pos.y;
			constant.matWorld._43 = pos.z;
			pRenderer->ProcessDrawingCube( constant );
		};

		// Screen box
		if ( 0 )
		{
			const auto hWidth  = currentScreen.size.x;
			const auto hHeight = currentScreen.size.y;
			const Donya::Vector2 scale{ hWidth * 2.0f / 0.5f, hHeight * 2.0f / 0.5f };

			constexpr auto  color = Donya::Color::Code::TEAL;
			constexpr float alpha = 0.6f;
			constant.drawColor = Donya::Vector4{ Donya::Color::MakeColor( color ), alpha };
			DrawCube( currentScreen.WorldPosition(), Donya::Vector3{ scale, 0.2f } );
		}

		// DirectionalLight source
		if ( 1 )
		{
			constexpr float lineLength = 24.0f;

			const auto lightPos = lightCamera.GetPosition();
			const auto lightDir = data.shadowMap.projectDirection.Unit();

			auto lineColor  = Donya::Color::Code::FUCHSIA;
			auto lineColor4 = Donya::Vector4{ Donya::Color::MakeColor( lineColor ), 1.0f };
			line.Reserve( lightPos, lightPos + ( lightDir * lineLength ), lineColor4 );

			/*
			Donya::Quaternion lookAt = Donya::Quaternion::LookAt
			(
				Donya::Quaternion::Identity(), lightDir
			);
			*/
			Donya::Quaternion lookAt = Donya::Quaternion::LookAt
			(
				Donya::Quaternion::Identity(), lightDir,
				Donya::Quaternion::Freeze::Up
			);
			lookAt = Donya::Quaternion::LookAt
			(
				lookAt, lightDir,
				Donya::Quaternion::Freeze::Right
			);
			lineColor = Donya::Color::Code::DARK_GRAY;
			lineColor4 = Donya::Vector4{ Donya::Color::MakeColor( lineColor ), 1.0f };
			//line.Reserve( lightPos, lightPos + ( lookAt.LocalFront()	* lineLength * 0.5f ), lineColor4 );
			line.Reserve( lightPos, lightPos + ( lookAt.LocalUp()		* lineLength * 0.5f ), lineColor4 );
			line.Reserve( lightPos, lightPos + ( lookAt.LocalRight()	* lineLength * 0.5f ), lineColor4 );

			constexpr auto  color = Donya::Color::Code::OLIVE;
			constexpr float alpha = 1.0f;
			constant.drawColor = Donya::Vector4{ Donya::Color::MakeColor( color ), alpha };
			DrawCube( lightPos, Donya::Vector3{ 1.0f, 1.0f, 1.0f }, lookAt );
		}

		// PointLight source
		if ( 1 )
		{
			const auto &plr = PointLightStorage::Get().GetStorage();

			constexpr Donya::Vector3 scale{ 0.05f, 0.05f, 0.05f };
			for ( unsigned int i = 0; i < plr.enableLightCount; ++i )
			{
				constant.drawColor.x = plr.lights[i].light.diffuseColor.x;
				constant.drawColor.y = plr.lights[i].light.diffuseColor.y;
				constant.drawColor.z = plr.lights[i].light.diffuseColor.z;
				constant.drawColor.w = 1.0f;
				DrawCube( plr.lights[i].wsPos, scale );
			}
		}
		
		line.Flush( VP );
	}
#endif // DEBUG_MODE
}

bool SceneTitle::CreateRenderers( const Donya::Int2 &wholeScreenSize )
{
	bool succeeded = true;

	pRenderer = std::make_unique<RenderingHelper>();
	if ( !pRenderer->Init() ) { succeeded = false; }
	
	auto pFontLoader = std::make_unique<Donya::Font::Holder>();
#if DEBUG_MODE
	const auto binPath = MakeFontPathBinary( FontAttribute::Main );
	if ( Donya::IsExistFile( binPath ) )
	{
		if ( !pFontLoader->LoadByCereal( binPath ) ) { succeeded = false; }
	}
	else
	{
		if ( !pFontLoader->LoadFntFile( MakeFontPathFnt( FontAttribute::Main ) ) ) { succeeded = false; }
		pFontLoader->SaveByCereal( binPath );
	}
#else
	if ( !pFontLoader->LoadByCereal( MakeFontPathBinary( FontAttribute::Main ) ) ) { succeeded = false; }
#endif // DEBUG_MODE
	pFontRenderer = std::make_unique<Donya::Font::Renderer>();
	if ( !pFontRenderer->Init( *pFontLoader ) ) { succeeded = false; }

	pDisplayer = std::make_unique<Donya::Displayer>();
	if ( !pDisplayer->Init() ) { succeeded = false; }

	pBloomer = std::make_unique<BloomApplier>();
	if ( !pBloomer->Init( wholeScreenSize ) ) { succeeded = false; }
	pBloomer->AssignParameter( FetchParameter().bloomParam );

	return succeeded;
}
bool SceneTitle::CreateSurfaces( const Donya::Int2 &wholeScreenSize )
{
	bool succeeded	= true;
	bool result		= true;

	pScreenSurface = std::make_unique<Donya::Surface>();
	result = pScreenSurface->Init
	(
		wholeScreenSize.x,
		wholeScreenSize.y,
		DXGI_FORMAT_R16G16B16A16_FLOAT
	);
	if ( !result ) { succeeded = false; }
	else { pScreenSurface->Clear( Donya::Color::Code::BLACK ); }

	pShadowMap = std::make_unique<Donya::Surface>();
	result = pShadowMap->Init
	(
		wholeScreenSize.x,
		wholeScreenSize.y,
		DXGI_FORMAT_R32_FLOAT, true,
		DXGI_FORMAT_R32_TYPELESS, true
	);
	if ( !result ) { succeeded = false; }
	else { pShadowMap->Clear( Donya::Color::Code::BLACK ); }

	return succeeded;
}
bool SceneTitle::CreateShaders()
{
	constexpr const char *VSPath = "./Data/Shaders/DisplayQuadVS.cso";
	constexpr const char *PSPath = "./Data/Shaders/DisplayQuadPS.cso";
	constexpr auto IEDescs = Donya::Displayer::Vertex::GenerateInputElements();

	// The vertex shader requires IE-descs as std::vector<>
	const std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsV{ IEDescs.begin(), IEDescs.end() };

	bool succeeded = true;

	pQuadShader = std::make_unique<Shader>();
	if ( !pQuadShader->VS.CreateByCSO( VSPath, IEDescsV	) ) { succeeded = false; }
	if ( !pQuadShader->PS.CreateByCSO( PSPath			) ) { succeeded = false; }

	return succeeded;
}
bool SceneTitle::AreRenderersReady() const
{
	if ( !pRenderer			) { return false; }
	if ( !pDisplayer		) { return false; }
	if ( !pBloomer			) { return false; }
	if ( !pScreenSurface	) { return false; }
	if ( !pShadowMap		) { return false; }
	if ( !pQuadShader		) { return false; }
	// else
	return true;
}

Donya::Vector4x4 SceneTitle::MakeScreenTransform() const
{
	const Donya::Vector4x4 matViewProj = iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix();
	const Donya::Vector4x4 matViewport = Donya::Vector4x4::MakeViewport( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
	return matViewProj * matViewport;
}
Donya::Collision::Box3F SceneTitle::CalcCurrentScreenPlane() const
{
	const Donya::Vector4x4 toWorld = MakeScreenTransform().Inverse();

	Donya::Plane xyPlane;
	xyPlane.distance	= 0.0f;
	xyPlane.normal		= -Donya::Vector3::Front();

	auto Transform		= [&]( const Donya::Vector3 &v, float fourthParam, const Donya::Vector4x4 &m )
	{
		Donya::Vector4 tmp = m.Mul( v, fourthParam );
		tmp /= tmp.w;
		return tmp.XYZ();
	};
	auto CalcWorldPos	= [&]( const Donya::Vector2 &ssPos, const Donya::Vector3 &oldPos )
	{
		const Donya::Vector3 ssRayStart{ ssPos, 0.0f };
		const Donya::Vector3 ssRayEnd  { ssPos, 1.0f };

		const Donya::Vector3 wsRayStart	= Transform( ssRayStart,	1.0f, toWorld );
		const Donya::Vector3 wsRayEnd	= Transform( ssRayEnd,		1.0f, toWorld );

		const auto result = Donya::CalcIntersectionPoint( wsRayStart, wsRayEnd, xyPlane );
		return ( result.isIntersect ) ? result.intersection : oldPos;
	};

	constexpr Donya::Vector2 halfScreenSize
	{
		Common::HalfScreenWidthF(),
		Common::HalfScreenHeightF(),
	};
	constexpr Donya::Vector2 left	{ 0.0f,						halfScreenSize.y		};
	constexpr Donya::Vector2 top	{ halfScreenSize.x,			0.0f					};
	constexpr Donya::Vector2 right	{ halfScreenSize.x * 2.0f,	halfScreenSize.y		};
	constexpr Donya::Vector2 bottom	{ halfScreenSize.x,			halfScreenSize.y * 2.0f	};

	Donya::Vector3 nowLeft		{ currentScreen.Min().x,	currentScreen.pos.y,	0.0f };
	Donya::Vector3 nowTop		{ currentScreen.pos.x,		currentScreen.Min().y,	0.0f };
	Donya::Vector3 nowRight		{ currentScreen.Max().x,	currentScreen.pos.y,	0.0f };
	Donya::Vector3 nowBottom	{ currentScreen.pos.x,		currentScreen.Max().y,	0.0f };

	nowLeft		= CalcWorldPos( left,	nowLeft		);
	nowTop		= CalcWorldPos( top,	nowTop		);
	nowRight	= CalcWorldPos( right,	nowRight	);
	nowBottom	= CalcWorldPos( bottom, nowBottom	);

	const float halfWidth	= fabsf( nowRight.x - nowLeft.x ) * 0.5f;
	const float halfHeight	= fabsf( nowBottom.y - nowTop.y ) * 0.5f;

	Donya::Collision::Box3F nowScreen;
	nowScreen.pos.x  = nowLeft.x + halfWidth;	// Specify center
	nowScreen.pos.y  = nowTop.y  - halfHeight;	// Specify center
	nowScreen.pos.z  = 0.0f;
	nowScreen.size.x = halfWidth;
	nowScreen.size.y = halfHeight;
	nowScreen.size.z = FLT_MAX;
	return nowScreen;
}

void SceneTitle::CameraInit()
{
	const auto &data = FetchParameter();

	iCamera.Init( Donya::ICamera::Mode::Look );
	iCamera.SetZRange( 0.1f, 1000.0f );
	iCamera.SetFOV( ToRadian( data.camera.fovDegree ) );
	iCamera.SetScreenSize( { Common::ScreenWidthF(), Common::ScreenHeightF() } );

	lightCamera.Init( Donya::ICamera::Mode::Free );
	lightCamera.SetZRange( data.shadowMap.nearDistance, data.shadowMap.projectDistance.z );
	lightCamera.SetScreenSize( data.shadowMap.projectDistance.XY() );

	AssignCameraPos();

	iCamera.SetProjectionPerspective();
	lightCamera.SetProjectionOrthographic();

	// I can setting a configuration,
	// but current data is not changed immediately.
	// So update here.
	Donya::ICamera::Controller moveInitPoint{};
	moveInitPoint.SetNoOperation();
	moveInitPoint.slerpPercent = 1.0f;
	iCamera.Update( moveInitPoint );
	lightCamera.Update( moveInitPoint );

	scroll.active			= false;
	scroll.elapsedSecond	= 0.0f;
}
Donya::Vector3 SceneTitle::ClampFocusPoint( const Donya::Vector3 &focusPoint, int roomID )
{
	if ( !pHouse ) { return focusPoint; }
	// else
		
	const auto area = pHouse->CalcRoomArea( roomID );
	if ( area == Donya::Collision::Box3F::Nil() ) { return focusPoint; }
	// else

	const auto &data = FetchParameter();

	const auto min = area.Min();
	const auto max = area.Max();
			
	const float halfAreaWidth		= ( max.x - min.x ) * 0.5f;
	const float halfAreaHeight		= ( max.y - min.y ) * 0.5f;
	const float halfScreenWidth		= ( currentScreen.Max().x - currentScreen.Min().x ) * 0.5f;
	const float halfScreenHeight	= ( currentScreen.Max().y - currentScreen.Min().y ) * 0.5f;
	const float halfWidth			= std::min( halfAreaWidth,  halfScreenWidth  ); // If area size smaller than screen, set to center the focus point
	const float halfHeight			= std::min( halfAreaHeight, halfScreenHeight ); // If area size smaller than screen, set to center the focus point

	Donya::Vector3 clamped = focusPoint;
	clamped += data.camera.offsetFocus; // Clamp the center pos in offseted value
	clamped.x = Donya::Clamp( clamped.x, min.x + halfWidth,  max.x - halfWidth  );
	clamped.y = Donya::Clamp( clamped.y, min.y + halfHeight, max.y - halfHeight );
	clamped.z = Donya::Clamp( clamped.z, min.z, max.z );
	clamped -= data.camera.offsetFocus; // Back to before offset pos because below setting process expects that value
	return clamped;
}
void SceneTitle::PrepareScrollIfNotActive( int oldRoomID, int newRoomID )
{
	if ( scroll.active ) { return; }
	// else

	scroll.active			= true;
	scroll.elapsedSecond	= 0.0f;

	const Donya::Vector3 baseFocus = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3::Zero();
	scroll.cameraFocusStart = ClampFocusPoint( baseFocus, oldRoomID );
	scroll.cameraFocusDest  = ClampFocusPoint( baseFocus, newRoomID );
}
void SceneTitle::AssignCameraPos()
{
	const auto &data = FetchParameter();
	
	Donya::Vector3 focusPos;
	if ( scroll.active )
	{
		const float percent = scroll.elapsedSecond / ( data.scrollTakeSecond + EPSILON );
		const Donya::Vector3 diff = scroll.cameraFocusDest - scroll.cameraFocusStart;

		focusPos = scroll.cameraFocusStart + ( diff * percent );
	}
	else
	{
		focusPos = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3::Zero();
		focusPos = ClampFocusPoint( focusPos, currentRoomID );
	}

	const Donya::Vector3 cameraPos = focusPos + data.camera.offsetPos;
	iCamera.SetPosition  ( cameraPos );
	iCamera.SetFocusPoint( focusPos + data.camera.offsetFocus );
	
	const Room *pCurrentRoom = ( pHouse ) ? pHouse->FindRoomOrNullptr( currentRoomID ) : nullptr;
	const Donya::Vector3 basePos
	{
		cameraPos.x,
		( pCurrentRoom ) ? pCurrentRoom->GetArea().Max().y : cameraPos.y,
		cameraPos.z,
	};
	lightCamera.SetPosition( basePos + data.shadowMap.posOffset );
}
void SceneTitle::CameraUpdate( float elapsedTime )
{
	const auto &data = FetchParameter();

#if USE_IMGUI
	// Apply for be able to see an adjustment immediately
	{
		iCamera.SetFOV( ToRadian( data.camera.fovDegree ) );
		iCamera.SetProjectionPerspective();

		const auto cameraPos = previousCameraPos;
		const Room *pCurrentRoom = ( pHouse ) ? pHouse->FindRoomOrNullptr( currentRoomID ) : nullptr;
		const Donya::Vector3 basePos
		{
			cameraPos.x,
			( pCurrentRoom ) ? pCurrentRoom->GetArea().Max().y : cameraPos.y,
			cameraPos.z,
		};
		lightCamera.SetPosition( basePos + data.shadowMap.posOffset );
		lightCamera.SetZRange( data.shadowMap.nearDistance, data.shadowMap.projectDistance.z );
		lightCamera.SetScreenSize( data.shadowMap.projectDistance.XY() );
		lightCamera.SetProjectionOrthographic();
	}
#endif // USE_IMGUI

	if ( scroll.active )
	{
		scroll.elapsedSecond += elapsedTime;
		if ( data.scrollTakeSecond <= scroll.elapsedSecond )
		{
			scroll.active = false;
		}
	}

	Donya::ICamera::Controller input{};
	input.SetNoOperation();
	input.slerpPercent = data.camera.slerpFactor;

	lightCamera.Update( input );

#if !DEBUG_MODE
	AssignCameraPos();
	iCamera.Update( input );
#else
	if ( !nowDebugMode )
	{
		AssignCameraPos();
		iCamera.Update( input );
		previousCameraPos = iCamera.GetPosition();
		return;
	}
	// else

	static Donya::Int2 prevMouse{};
	static Donya::Int2 currMouse{};

	prevMouse = currMouse;

	auto nowMouse = Donya::Mouse::Coordinate();
	currMouse.x = scast<int>( nowMouse.x );
	currMouse.y = scast<int>( nowMouse.y );

	bool  isInputMouseButton = Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) || Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) || Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT );
	bool  isDriveMouse = ( prevMouse != currMouse ) || Donya::Mouse::WheelRot() || isInputMouseButton;
	if ( !isDriveMouse )
	{
		input.SetNoOperation();
		iCamera.Update( input );
		return;
	}
	// else

	const Donya::Vector2 diff = ( currMouse - prevMouse ).Float();
	
	Donya::Vector3 movement{};
	Donya::Vector3 rotation{};

	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		if ( Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) )
		{
			constexpr float ROT_AMOUNT = ToRadian( 0.5f );
			rotation.x = diff.x * ROT_AMOUNT;
			rotation.y = diff.y * ROT_AMOUNT;

			if ( isReverseCameraRotX ) { rotation.x *= -1.0f; }
			if ( isReverseCameraRotY ) { rotation.y *= -1.0f; }
		}
	}

	// Operation that ALT key is not needed 
	{
		if ( Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) )
		{
			constexpr float MOVE_SPEED = 0.05f;
			movement.x = diff.x * MOVE_SPEED;
			movement.y = diff.y * MOVE_SPEED;

			if ( isReverseCameraMoveX ) { movement.x *= -1.0f; }
			if ( isReverseCameraMoveY ) { movement.y *= -1.0f; }
		}

		constexpr float FRONT_SPEED = 3.0f;
		movement.z = FRONT_SPEED * scast<float>( Donya::Mouse::WheelRot() );
	}

	input.moveVelocity		= movement;
	input.yaw				= rotation.x;
	input.pitch				= rotation.y;
	input.roll				= 0.0f;
	input.moveInLocalSpace	= true;

	iCamera.Update( input );

	input.SetNoOperation();

#endif // !DEBUG_MODE
}

Donya::Vector4x4 SceneTitle::CalcLightViewProjectionMatrix() const
{
	const Donya::Vector3 lightPos = lightCamera.GetPosition();

	const Donya::Vector3 lookDirection = FetchParameter().shadowMap.projectDirection.Unit();
#define DISALLOW_ROLL ( true )
#if DISALLOW_ROLL
	Donya::Quaternion lookAt = Donya::Quaternion::LookAt
	(
		Donya::Quaternion::Identity(), lookDirection,
		Donya::Quaternion::Freeze::Up
	);
	lookAt = Donya::Quaternion::LookAt
	(
		lookAt, lookDirection,
		Donya::Quaternion::Freeze::Right
	);
	/*
	lookAt.RotateBy
	(
		Donya::Quaternion::Make
		(
			lookAt.LocalRight(),
			atan2f( -lookDirection.y, lookDirection.z )
		)
	);
	*/
#else
	Donya::Quaternion lookAt = Donya::Quaternion::LookAt( Donya::Vector3::Front(), lookDirection );
#endif // DISALLOW_ROLL
#undef DISALLOW_ROLL

	Donya::Vector4x4 view = lookAt.Conjugate().MakeRotationMatrix();
	view._41 = -lightPos.x;
	view._42 = -lightPos.y;
	view._43 = -lightPos.z;
	return view * lightCamera.GetProjectionMatrix();
}

void SceneTitle::PlayerInit()
{
	if ( !pPlayerIniter )
	{
		_ASSERT_EXPR( 0, L"Error: Initializer is not available!" );
		return;
	}
	// else

	if ( pPlayer )
	{
		pPlayer->Uninit();
		pPlayer.reset();
	}

	pPlayer = std::make_unique<Player>();
	pPlayer->Init( *pPlayerIniter );
}
void SceneTitle::PlayerUpdate( float elapsedTime, const Map &terrain )
{
	if ( !pPlayer ) { return; }
	// else

	Player::Input input{};
	input.moveVelocity	= { 0.0f, 0.0f };
	input.useJump		= false;
	input.useShot		= false;

	pPlayer->Update( elapsedTime, input, terrain );
}

int  SceneTitle::CalcCurrentRoomID() const
{
	if ( !pPlayer || !pHouse ) { return currentRoomID; }
	// else

	const int newID =  pHouse->CalcBelongRoomID( pPlayer->GetPosition() );
	return (  newID == Room::invalidID ) ? currentRoomID : newID;
}

void SceneTitle::ClearBackGround() const
{
	if ( pShadowMap ) { pShadowMap->Clear( Donya::Color::Code::BLACK ); }

	constexpr Donya::Vector3 gray = Donya::Color::MakeColor( Donya::Color::Code::GRAY );
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );

	if ( pScreenSurface ) { pScreenSurface->Clear( Donya::Vector4{ gray, 1.0f } ); }
#if DEBUG_MODE
	if ( nowDebugMode )
	{
		constexpr Donya::Vector3 teal = Donya::Color::MakeColor( Donya::Color::Code::CYAN );
		constexpr FLOAT DEBUG_COLOR[4]{ teal.x, teal.y, teal.z, 1.0f };
		Donya::ClearViews( DEBUG_COLOR );

		if ( pScreenSurface ) { pScreenSurface->Clear( Donya::Vector4{ teal, 1.0f } ); }
	}
#endif // DEBUG_MODE
}
void SceneTitle::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeSecond	= Fader::GetDefaultCloseSecond();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneTitle::ReturnResult()
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F2 ) && !Fader::Get().IsExist() )
	{
		Donya::Sound::Play( Music::DEBUG_Strong );
		StartFade();
	}
#endif // DEBUG_MODE

	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Game;
		return change;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
void SceneTitle::UseImGui()
{
	ImGui::SetNextWindowBgAlpha( 0.6f );
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else

	constexpr int stageNo = Definition::StageNumber::Title();

	sceneParam.ShowImGuiNode( u8"タイトルシーンのパラメータ" );

	if ( ImGui::TreeNode( u8"ステージファイルの読み込み" ) )
	{
		static bool thenSave = true;
		ImGui::Checkbox( u8"読み込み・適用後にセーブする", &thenSave );

		auto PrepareCSVData	= []( const std::string &filePath )
		{
			CSVLoader loader;
			loader.Clear();

			if ( filePath.empty() || !Donya::IsExistFile( filePath ) )
			{
				std::string msg = u8"ファイルオープンに失敗しました。\n";
				msg += u8"ファイル名：[" + filePath + u8"]";
				Donya::ShowMessageBox
				(
					msg,
					"Loading stage is failed",
					MB_ICONEXCLAMATION | MB_OK
				);

				return loader;
			}
			// else

			if ( !loader.Load( filePath ) )
			{
				Donya::ShowMessageBox
				(
					u8"パースに失敗しました。",
					"Loading stage is failed",
					MB_ICONEXCLAMATION | MB_OK
				);
				loader.Clear();
				return loader;
			}
			// else

			return loader;
		};
		auto IsValidData	= []( const CSVLoader &loadedData )
		{
			return ( !loadedData.Get().empty() );
		};

		auto ApplyToEnemy	= [&]( const CSVLoader &loadedData )
		{
			Enemy::Admin::Get().ClearInstances();
			Enemy::Admin::Get().RemakeByCSV( loadedData, currentScreen );

			if ( thenSave )
			{
				Enemy::Admin::Get().SaveEnemies( stageNo, /* fromBinary = */ true  );
				Enemy::Admin::Get().SaveEnemies( stageNo, /* fromBinary = */ false );
			}
		};
		auto ApplyToItem	= [&]( const CSVLoader &loadedData )
		{
			Item::Admin::Get().ClearInstances();
			Item::Admin::Get().RemakeByCSV( loadedData );

			if ( thenSave )
			{
				Item::Admin::Get().SaveItems( stageNo, /* fromBinary = */ true  );
				Item::Admin::Get().SaveItems( stageNo, /* fromBinary = */ false );
			}
		};
		auto ApplyToMap		= [&]( const CSVLoader &loadedData )
		{
			if ( !pMap ) { return; }
			// else

			pMap->RemakeByCSV( loadedData );
			if ( thenSave )
			{
				pMap->SaveMap( stageNo, /* fromBinary = */ true  );
				pMap->SaveMap( stageNo, /* fromBinary = */ false );
			}
		};
		auto ApplyToPlayer	= [&]( const CSVLoader &loadedData )
		{
			if ( !pPlayerIniter ) { pPlayerIniter = std::make_unique<PlayerInitializer>(); }
			pPlayerIniter->RemakeByCSV( loadedData );

			if ( thenSave )
			{
				pPlayerIniter->SaveBin ( stageNo );
				pPlayerIniter->SaveJson( stageNo );
			}

			PlayerInit();
		};
		auto ApplyToRoom	= [&]( const CSVLoader &loadedData )
		{
			if ( !pHouse ) { return; }
			// else

			pHouse->RemakeByCSV( loadedData );
			if ( thenSave )
			{
				pHouse->SaveRooms( stageNo, /* fromBinary = */ true  );
				pHouse->SaveRooms( stageNo, /* fromBinary = */ false );
			}
		};

		if ( ImGui::TreeNode( u8"バッチロード" ) )
		{
			constexpr size_t bufferSize			= 128U;
			constexpr size_t bufferSizeWithNull	= bufferSize + 1;
			using  BufferType = std::array<char, bufferSizeWithNull>;

			// These default value are my prefer
			static int readStageNumber = Definition::StageNumber::Game();
			static BufferType bufferDirectory	{ "./../../EdittedData/"	};
			static BufferType bufferPrefix		{ "Stage"					};
			static BufferType bufferEnemy		{ "Enemy"					};
			static BufferType bufferItem		{ "Item"					};
			static BufferType bufferMap			{ "Map"						};
			static BufferType bufferRoom		{ "Room"					};
			static BufferType bufferExtension	{ ".csv"					};

			if ( ImGui::Button( u8"読み込み開始" ) )
			{
				Bullet::Admin::Get().ClearInstances();

				const std::string fileDirectory	= bufferDirectory.data();
				const std::string filePrefix	= bufferPrefix.data();
				const std::string fileExtension	= bufferExtension.data();
				const std::string strStageNo	= ( 0 <= readStageNumber && readStageNumber < 10 ) ? "0" + std::to_string( readStageNumber ) : std::to_string( readStageNumber );
				constexpr const char noSuffix = '_';
				std::string	filePath{};
				CSVLoader	loader{};
				auto ProcessOf = [&]( const BufferType &bufferIdentify, const std::function<void( const CSVLoader & )> &ApplyToXXX )
				{
					filePath =	bufferDirectory.data() +
								filePrefix + strStageNo + noSuffix +
								bufferIdentify.data() +
								bufferExtension.data();

					loader.Clear();
					loader = PrepareCSVData( filePath );

					if ( IsValidData( loader ) )
					{
						ApplyToXXX( loader );
					}
				};

				ProcessOf( bufferMap,	ApplyToMap		);
				ProcessOf( bufferRoom,	ApplyToRoom		);
				ProcessOf( bufferMap,	ApplyToPlayer	);
				ProcessOf( bufferEnemy,	ApplyToEnemy	);
				ProcessOf( bufferItem,	ApplyToItem		);

				if ( pMap ) { pMap->ReloadModel( readStageNumber ); }

				currentRoomID = CalcCurrentRoomID();
			}

			ImGui::InputInt ( u8"読み込むステージ番号",	&readStageNumber );
			ImGui::InputText( u8"接頭辞",				bufferPrefix.data(),	bufferSize );
			ImGui::InputText( u8"ディレクトリ",			bufferDirectory.data(),	bufferSize );
			ImGui::InputText( u8"識別子・敵",			bufferEnemy.data(),		bufferSize );
			ImGui::InputText( u8"識別子・アイテム",		bufferItem.data(),		bufferSize );
			ImGui::InputText( u8"識別子・マップ＆自機",	bufferMap.data(),		bufferSize );
			ImGui::InputText( u8"識別子・ルーム",			bufferRoom.data(),		bufferSize );
			ImGui::InputText( u8"拡張子",				bufferExtension.data(),	bufferSize );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"個別ロード" ) )
		{
			static bool applyEnemy	= false;
			static bool applyItem	= false;
			static bool applyMap	= true;
			static bool applyPlayer	= true;
			static bool applyRoom	= false;

			ImGui::Checkbox( u8"敵に適用",				&applyEnemy		);
			ImGui::Checkbox( u8"アイテムに適用",			&applyItem		);
			ImGui::Checkbox( u8"マップに適用",			&applyMap		); ImGui::SameLine();
			ImGui::Checkbox( u8"自機に適用",				&applyPlayer	);
			ImGui::Checkbox( u8"ルームに適用",			&applyRoom		);
		
			if ( ImGui::Button( u8"CSVファイルを読み込む" ) )
			{
				const auto filePath	= FetchStageFilePathByCommonDialog();
				const auto loader	= PrepareCSVData( filePath );
				if ( IsValidData( loader ) )
				{
					Bullet::Admin::Get().ClearInstances();

					if ( applyEnemy		) { ApplyToEnemy	( loader ); }
					if ( applyItem		) { ApplyToItem		( loader ); }
					if ( applyMap		) { ApplyToMap		( loader );	}
					if ( applyPlayer	) { ApplyToPlayer	( loader );	currentRoomID = CalcCurrentRoomID(); }
					if ( applyRoom		) { ApplyToRoom		( loader );	currentRoomID = CalcCurrentRoomID(); }
				}
			}

			static int loadMapNumber = 0;
			ImGui::InputInt( u8"マップモデルに適用するステージ番号", &loadMapNumber );
			if ( ImGui::Button( u8"マップモデルを読み込む" ) && pMap )
			{
				pMap->ReloadModel( loadMapNumber );
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"各オブジェクトの調整" ) )
	{
		ImGui::InputInt( u8"現在のルーム番号", &currentRoomID );

		if ( ImGui::TreeNode( u8"カメラの現在" ) )
		{
			ImGui::Checkbox( u8"移動方向を反転する・Ｘ", &isReverseCameraMoveX );
			ImGui::Checkbox( u8"移動方向を反転する・Ｙ", &isReverseCameraMoveY );
			ImGui::Checkbox( u8"回転方向を反転する・Ｘ", &isReverseCameraRotX );
			ImGui::Checkbox( u8"回転方向を反転する・Ｙ", &isReverseCameraRotY );

			auto ShowVec3 = []( const std::string &prefix, const Donya::Vector3 &v )
			{
				ImGui::Text( ( prefix + u8"[X:%5.2f][Y:%5.2f][Z:%5.2f]" ).c_str(), v.x, v.y, v.z );
			};

			const Donya::Vector3 cameraPos = iCamera.GetPosition();
			ShowVec3( u8"現在位置", cameraPos );
			ImGui::Text( "" );

			const Donya::Vector3 focusPoint = iCamera.GetFocusPoint();
			ShowVec3( u8"注視点位置", focusPoint );
			ImGui::Text( "" );
			ImGui::TreePop();
		}
		ImGui::Text( "" );

		if ( pPlayer ) { pPlayer->ShowImGuiNode( u8"自機の現在" ); }
		Player::UpdateParameter( u8"自機のパラメータ" );
		ImGui::Text( "" );

		if ( pMap    ) { pMap->ShowImGuiNode( u8"マップの現在", stageNo ); }
		if ( pHouse  ) { pHouse->ShowImGuiNode( u8"部屋の現在", stageNo ); }
		ImGui::Text( "" );

		Bullet::Admin::Get().ShowImGuiNode( u8"弾の現在" );
		Bullet::Parameter::Update( u8"弾のパラメータ" );
		ImGui::Text( "" );

		Enemy::Admin::Get().ShowImGuiNode( u8"敵の現在", stageNo, currentScreen );
		Enemy::Parameter::Update( u8"敵のパラメータ" );
		ImGui::Text( "" );

		Item::Admin::Get().ShowImGuiNode( u8"アイテムの現在", stageNo );
		Item::Parameter::Update( u8"アイテムのパラメータ" );
		ImGui::Text( "" );

		Meter::Parameter::Update( u8"メータのパラメータ" );
		ImGui::Text( "" );

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"サーフェス描画" ) )
	{
		static Donya::Vector2 drawSize{ 320.0f, 180.0f };
		ImGui::DragFloat2( u8"描画サイズ", &drawSize.x, 10.0f );
		drawSize.x = std::max( 10.0f, drawSize.x );
		drawSize.y = std::max( 10.0f, drawSize.y );

		if ( pShadowMap && ImGui::TreeNode( u8"シャドウマップ" ) )
		{
			pShadowMap->DrawDepthStencilToImGui( drawSize );
			ImGui::TreePop();
		}
		if ( pScreenSurface && ImGui::TreeNode( u8"スクリーン" ) )
		{
			pScreenSurface->DrawRenderTargetToImGui( drawSize );
			ImGui::TreePop();
		}
		if ( pBloomer && ImGui::TreeNode( u8"ブルーム" ) )
		{
			ImGui::Text( u8"輝度抽出：" );
			pBloomer->DrawHighLuminanceToImGui( drawSize );
			ImGui::Text( u8"縮小バッファたち：" );
			pBloomer->DrawBlurBuffersToImGui( drawSize );
			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
	
	ImGui::End();
}
#endif // USE_IMGUI
