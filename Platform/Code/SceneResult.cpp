#include "SceneResult.h"

#include <algorithm>				// Use std::find
#include <vector>

#undef max
#undef min

#include "Donya/Blend.h"
#include "Donya/Color.h"			// Use ClearBackGround(), StartFade().
#include "Donya/Constant.h"
#include "Donya/Keyboard.h"			// Make an input of player.
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Template.h"			// Use Clamp
#include "Donya/Useful.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Common.h"
#include "Effect/EffectAdmin.h"
#include "Enemies/SuperBallMachine.h"
#include "Fader.h"
#include "FilePath.h"
#include "FontHelper.h"
#include "Input.h"
#include "Music.h"
#include "Parameter.h"
#include "PointLightStorage.h"
#include "StageNumber.h"

#if DEBUG_MODE
#include "CSVLoader.h"
#pragma comment( lib, "comdlg32.lib" ) // Used for common-dialog
#endif // DEBUG_MODE

namespace
{
#if DEBUG_MODE
	constexpr bool IOFromBinary = false;
#else
	constexpr bool IOFromBinary = true;
#endif // DEBUG_MODE
}

namespace
{
	struct SceneParam
	{
		struct
		{
			float fovDegree = 30.0f;
			Donya::Vector3 offsetPos{ 0.0f, 5.0f, -10.0f };
			Donya::Vector3 offsetFocus;
		}
		camera;

		Donya::Model::Constants::PerScene::DirectionalLight directionalLight;

		struct ShadowMap
		{
			Donya::Vector3	color;			// RGB
			float			bias = 0.03f;	// Ease an acne

			float			offsetDistance	= 10.0f;				// From the player position
			Donya::Vector3	projectDirection{  0.0f,  0.0f,  1.0f };
			Donya::Vector3	projectDistance { 10.0f, 10.0f, 50.0f };// [m]
			float			nearDistance	= 1.0f;					// Z near is this. Z far is projectDistance.z.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( color			),
					CEREAL_NVP( bias			),
					CEREAL_NVP( offsetDistance	),
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

		std::vector<Enemy::InitializeParam> enemies;
		float firstGenerateEnemySecond		= 1.0f;
		float generateEnemyIntervalSecond	= 1.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( camera.fovDegree	),
				CEREAL_NVP( camera.offsetPos	),
				CEREAL_NVP( camera.offsetFocus	),
				CEREAL_NVP( directionalLight	),
				CEREAL_NVP( shadowMap			),
				CEREAL_NVP( bloomParam			),
				CEREAL_NVP( enemies				)
			);

			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( firstGenerateEnemySecond	),
					CEREAL_NVP( generateEnemyIntervalSecond	)
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
				ImGui::DragFloat ( u8"画角（Degree）",	&camera.fovDegree,		0.1f  );
				ImGui::DragFloat3( u8"自身の座標",		&camera.offsetPos.x,	0.01f );
				ImGui::DragFloat3( u8"注視点の座標",		&camera.offsetFocus.x,	0.01f );

				ImGui::TreePop();
			}

			ImGui::Helper::ShowDirectionalLightNode( u8"平行光", &directionalLight );

			if ( ImGui::TreeNode( u8"シャドウマップ関連" ) )
			{
				ImGui::ColorEdit3( u8"影の色",					&shadowMap.color.x );
				ImGui::DragFloat ( u8"アクネ用のバイアス",		&shadowMap.bias,				0.01f );
				ImGui::DragFloat ( u8"自機からの距離",			&shadowMap.offsetDistance,		0.1f  );
				ImGui::DragFloat3( u8"写す方向（単位ベクトル）",	&shadowMap.projectDirection.x,	0.01f );
				ImGui::DragFloat3( u8"写す範囲ＸＹＺ",			&shadowMap.projectDistance.x,	1.0f  );
				ImGui::DragFloat ( u8"Z-Near",					&shadowMap.nearDistance,		1.0f  );

				shadowMap.offsetDistance	= std::max( 0.01f, shadowMap.offsetDistance );
				shadowMap.projectDistance.x	= std::max( 0.01f, shadowMap.projectDistance.x );
				shadowMap.projectDistance.y	= std::max( 0.01f, shadowMap.projectDistance.y );
				shadowMap.projectDistance.z	= std::max( shadowMap.nearDistance + 1.0f, shadowMap.projectDistance.z );

				static bool alwaysNormalize = false;
				if ( ImGui::Button( u8"写す方向を正規化" ) || alwaysNormalize )
				{
					shadowMap.projectDirection.Normalize();
				}
				ImGui::Checkbox( u8"常に正規化する", &alwaysNormalize );

				ImGui::TreePop();
			}

			bloomParam.ShowImGuiNode( u8"ブルーム関連" );

			if ( enemies.empty() )
			{
				Enemy::InitializeParam arg{};
				enemies.emplace_back( arg );
			}
			if ( ImGui::TreeNode( u8"敵関連" ) )
			{
				ImGui::DragFloat( u8"初回生成までの待機秒数",			&firstGenerateEnemySecond,		0.01f );
				ImGui::DragFloat( u8"全滅後に再生成するまでの秒数",	&generateEnemyIntervalSecond,	0.01f );
				firstGenerateEnemySecond	= std::max( 0.0f, firstGenerateEnemySecond		);
				generateEnemyIntervalSecond	= std::max( 0.0f, generateEnemyIntervalSecond	);

				ImGui::Helper::ResizeByButton( &enemies );

				const int enemyCount = enemies.size();
				for ( int i = 0; i < enemyCount; ++i )
				{
					enemies[i].ShowImGuiNode( Donya::MakeArraySuffix( i ) );
				}

				ImGui::TreePop();
			}
		}
	#endif // USE_IMGUI
	};

	static ParamOperator<SceneParam> sceneParam{ "SceneResult" };
	const SceneParam &FetchParameter()
	{
		return sceneParam.Get();
	}

	static Donya::Collision::Box3F unlimitedScreen{ Donya::Vector3::Zero(), { FLT_MAX * 0.4f, FLT_MAX * 0.4f, FLT_MAX * 0.4f } };

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
CEREAL_CLASS_VERSION( SceneParam, 0 )

void SceneResult::Init()
{
	Donya::Sound::Play( Music::BGM_Result );
#if DEBUG_MODE
	Donya::Sound::AppendFadePoint( Music::BGM_Result, 0.0f, 0.0f, true );
#endif // DEBUG_MODE

	sceneParam.LoadParameter();
	const auto &data = FetchParameter();

	constexpr int stageNo = Definition::StageNumber::Result();
	constexpr Donya::Int2 wholeScreenSize
	{
		Common::ScreenWidth(),
		Common::ScreenHeight(),
	};
	
	bool result{};

	result = CreateRenderers( wholeScreenSize );
	assert( result );

	result = CreateSurfaces( wholeScreenSize );
	assert( result );

	result = CreateShaders();
	assert( result );

	currentTimer	= 0.0f;
	previousTimer	= 0.0f;
	extinctTime		= 0.0f;

	pMap = std::make_unique<Map>();
	pMap->Init( stageNo, /* reloadModel = */ false );

	playerIniter.LoadParameter( stageNo );
	PlayerInit( playerIniter, *pMap );

	CameraInit();

	Bullet::Admin::Get().ClearInstances();

	Enemy::Admin::Get().ClearInstances();
	enemies.clear();

	auto &effectAdmin = Effect::Admin::Get();
	effectAdmin.SetLightColorAmbient( { 1.0f, 1.0f, 1.0f, 1.0f } );
	effectAdmin.SetLightColorDiffuse( { 1.0f, 1.0f, 1.0f, 1.0f } );
	effectAdmin.SetLightDirection	( data.directionalLight.direction.XYZ() );
	effectAdmin.ClearInstances();
}
void SceneResult::Uninit()
{
	Effect::Admin::Get().ClearInstances();
	Donya::Sound::Stop( Music::BGM_Result );
}

Scene::Result SceneResult::Update( float elapsedTime )
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F2 ) && !Fader::Get().IsExist() )
	{
		Donya::Sound::Play( Music::DEBUG_Strong );

		StartFade();
	}
	if ( Donya::Keyboard::Trigger( VK_F5 ) )
	{
		nowDebugMode = !nowDebugMode;

		if ( nowDebugMode )
		{
			iCamera.ChangeMode( Donya::ICamera::Mode::Satellite );
		}
		else
		{
			iCamera.SetOrientation( Donya::Quaternion::Identity() );
			iCamera.Init( Donya::ICamera::Mode::Look );
		}
	}
#endif // DEBUG_MODE

#if USE_IMGUI
	UseImGui();
	
	// Apply for be able to see an adjustment immediately
	{
		if ( pBloomer ) { pBloomer->AssignParameter( FetchParameter().bloomParam ); }

		Effect::Admin::Get().SetProjectionMatrix( iCamera.GetProjectionMatrix() );
	}
#endif // USE_IMGUI

	PointLightStorage::Get().Clear();

	controller.Update();

	previousTimer = currentTimer;
	currentTimer += elapsedTime;

	if ( pMap ) { pMap->Update( elapsedTime ); }
	const Map emptyMap{}; // Used for empty argument. Fali safe.
	const Map &mapRef = ( pMap ) ? *pMap : emptyMap;

	PlayerUpdate( elapsedTime, mapRef );
	EnemyUpdate( elapsedTime, GetPlayerPosition() );
	Bullet::Admin::Get().Update( elapsedTime, currentScreen );

	PlayerPhysicUpdate( elapsedTime, mapRef );
	EnemyPhysicUpdate( elapsedTime, mapRef );
	Bullet::Admin::Get().PhysicUpdate( elapsedTime, mapRef );

	if ( !Fader::Get().IsExist() )
	{
		bool shouldSkip = false;
		if ( controller.IsConnected() )
		{
			shouldSkip = controller.Trigger( Donya::Gamepad::Button::A );
		}
		else
		{
			//shouldSkip = Donya::Keyboard::Trigger( 'Z' );
		}

		if ( shouldSkip )
		{
			StartFade();
		}
	}

	// CameraUpdate() depends the currentScreen, so I should update that before CameraUpdate().
	currentScreen = CalcCurrentScreenPlane();
	CameraUpdate();

	return ReturnResult();
}

namespace
{
	enum class DrawTarget
	{
		Bullet	= 1 << 0,
		Player	= 1 << 1,
		Enemy	= 1 << 2,

		All		= Bullet | Player | Enemy
	};
	DEFINE_ENUM_FLAG_OPERATORS( DrawTarget )
}
void SceneResult::Draw( float elapsedTime )
{
	ClearBackGround();

	if ( !AreRenderersReady() ) { return; }
	// else

	auto UpdateSceneConstant	= [&]( const Donya::Model::Constants::PerScene::DirectionalLight &directionalLight, const Donya::Vector4 &eyePos, const Donya::Vector4x4 &viewMatrix, const Donya::Vector4x4 &viewProjectionMatrix )
	{
		Donya::Model::Constants::PerScene::Common constant{};
		constant.directionalLight	= directionalLight;
		constant.eyePosition		= eyePos;
		constant.viewMatrix			= viewMatrix;
		constant.viewProjMatrix		= viewProjectionMatrix;
		pRenderer->UpdateConstant( constant );
	};
	auto DrawObjects			= [&]( DrawTarget option, bool castShadow )
	{
		using Kind = DrawTarget;
		auto Drawable = [&option]( Kind verify )
		{
			return scast<int>( option & verify ) != 0;
		};

		// The drawing priority is determined by the priority of the information.

		( castShadow )
		? pRenderer->ActivateShaderShadowStatic()
		: pRenderer->ActivateShaderNormalStatic();

		( castShadow )
		? pRenderer->DeactivateShaderShadowStatic()
		: pRenderer->DeactivateShaderNormalStatic();


		( castShadow )
		? pRenderer->ActivateShaderShadowSkinning()
		: pRenderer->ActivateShaderNormalSkinning();

		if ( Drawable( Kind::Player	) && pPlayer		) { pPlayer->Draw( pRenderer.get() ); }
		if ( Drawable( Kind::Enemy	) ) { EnemyDraw( pRenderer.get() ); }
		if ( Drawable( Kind::Bullet	) ) { Bullet::Admin::Get().Draw( pRenderer.get() );}

		( castShadow )
		? pRenderer->DeactivateShaderShadowSkinning()
		: pRenderer->DeactivateShaderNormalSkinning();
	};
	
	const Donya::Vector4   cameraPos = Donya::Vector4{ iCamera.GetPosition(), 1.0f };
	const Donya::Vector4x4 V = iCamera.CalcViewMatrix();
	const Donya::Vector4x4 VP = V * iCamera.GetProjectionMatrix();

	const Donya::Vector4   lightPos = Donya::Vector4{ lightCamera.GetPosition(), 1.0f };
	const Donya::Vector4x4 LV  = lightCamera.CalcViewMatrix();
	const Donya::Vector4x4 LVP = LV * lightCamera.GetProjectionMatrix();

	const auto &data = FetchParameter();

	Effect::Admin::Get().SetViewMatrix( V );

	Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLess );
	Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullBack_CCW );
	pRenderer->ActivateSamplerModel( Donya::Sampler::Defined::Aniso_Wrap );
	pRenderer->ActivateSamplerNormal( Donya::Sampler::Defined::Point_Wrap );

	pShadowMap->SetRenderTarget();
	pShadowMap->SetViewport();
	// Make the shadow map
	{
		// Update scene constant as light source
		{
			Donya::Model::Constants::PerScene::DirectionalLight tmpDirLight{};
			tmpDirLight.direction = Donya::Vector4{ data.shadowMap.projectDirection.Unit(), 0.0f };
			UpdateSceneConstant( tmpDirLight, lightPos, LV, LVP );
		}
		pRenderer->ActivateConstantScene();

		DrawObjects( DrawTarget::All, /* castShadow = */ true );

		pRenderer->DeactivateConstantScene();
	}
	Donya::Surface::ResetRenderTarget();

	pScreenSurface->SetRenderTarget();
	pScreenSurface->SetViewport();
	// Draw normal scene with shadow map
	{
		RenderingHelper::ShadowConstant shadowConstant{};

		// Update scene and shadow constants
		{
			UpdateSceneConstant( data.directionalLight, cameraPos, V, VP );

			shadowConstant.lightProjMatrix	= LVP;
			shadowConstant.shadowColor		= data.shadowMap.color;
			shadowConstant.shadowBias		= data.shadowMap.bias;
			pRenderer->UpdateConstant( shadowConstant );
		}
		// Update point light constant
		{
			pRenderer->UpdateConstant( PointLightStorage::Get().GetStorage() );
		}

		pRenderer->ActivateConstantScene();
		pRenderer->ActivateConstantPointLight();
		pRenderer->ActivateConstantShadow();
		pRenderer->ActivateSamplerShadow( Donya::Sampler::Defined::Point_Border_White );
		pRenderer->ActivateShadowMap( *pShadowMap );

		constexpr DrawTarget option = DrawTarget::All ^ DrawTarget::Bullet;
		DrawObjects( option, /* castShadow = */ false );

		// Disable shadow
		{
			pRenderer->DeactivateConstantShadow();
			shadowConstant.shadowBias = 1.0f; // Make the pixel to nearest
			pRenderer->UpdateConstant( shadowConstant );
			pRenderer->ActivateConstantShadow();
		}

		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::NoTest_Write );
		DrawObjects( DrawTarget::Bullet, /* castShadow = */ false );
		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLess );

		pRenderer->DeactivateShadowMap( *pShadowMap );
		pRenderer->DeactivateSamplerShadow();
		pRenderer->DeactivateConstantShadow();
		pRenderer->DeactivateConstantPointLight();
		pRenderer->DeactivateConstantScene();
	}
	Donya::Surface::ResetRenderTarget();

	pRenderer->DeactivateSamplerModel();
	Donya::Rasterizer::Deactivate();
	Donya::DepthStencil::Deactivate();

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

	Donya::SetDefaultRenderTargets();

	const Donya::Vector2 screenSurfaceSize = pScreenSurface->GetSurfaceSizeF();

	Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLessEq );
	Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullNone );
	// Draw the scene to screen
	{
		Donya::Sampler::SetPS( Donya::Sampler::Defined::Aniso_Wrap, 0 );
		Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );

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

		Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );
		Donya::Sampler::ResetPS( 0 );
	}

	// Add the bloom buffers
	Donya::Blend::Activate( Donya::Blend::Mode::ADD_NO_ATC );
	pBloomer->DrawBlurBuffers( screenSurfaceSize );
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );

	Donya::Rasterizer::Deactivate();
	Donya::DepthStencil::Deactivate();


#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		if ( pPlayer	) { pPlayer->DrawHitBox( pRenderer.get(), VP );					}
		if ( pMap		) { pMap->DrawHitBoxes( currentScreen, pRenderer.get(), VP );	}
		Bullet::Admin::Get().DrawHitBoxes( pRenderer.get(), VP );
		for ( const auto &pIt : enemies ) { if ( pIt ) { pIt->DrawHitBox( pRenderer.get(), VP ); } }
	}
#endif // DEBUG_MODE

	const auto pFontRenderer = FontHelper::GetRendererOrNullptr( FontAttribute::Main );
	if ( pFontRenderer )
	{
		constexpr Donya::Vector2 pivot{ 0.5f, 0.5f };
		constexpr Donya::Vector2 center{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };

		pFontRenderer->Draw
		(
			L"YOU GOT",
			center + Donya::Vector2{ 0.0f, -320.0f },
			pivot
		);
		pFontRenderer->Draw
		(
			L"SPECIAL WEAPON",
			center + Donya::Vector2{ 0.0f, -256.0f },
			pivot
		);
		
		if ( controller.IsConnected() )
		{
			pFontRenderer->Draw
			(
				L"PRESS A TO SKIP",
				center + Donya::Vector2{ 0.0f, 256.0f },
				pivot
			);
		}
		else
		{
			pFontRenderer->Draw
			(
				L"PRESS Z TO SKIP",
				center + Donya::Vector2{ 0.0f, 256.0f },
				pivot
			);
		}

		Donya::Sprite::Flush();
	}

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= VP;
		constant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 0.6f };
		constant.lightDirection = Donya::Vector3{ 0.0f, -1.0f, 0.0f };
		
		auto DrawCube = [&]( const Donya::Vector3 &pos, const Donya::Vector3 &scale = { 1.0f, 1.0f, 1.0f } )
		{
			constant.matWorld._11 = scale.x * 2.0f;
			constant.matWorld._22 = scale.y * 2.0f;
			constant.matWorld._33 = scale.z * 2.0f;
			constant.matWorld._41 = pos.x;
			constant.matWorld._42 = pos.y;
			constant.matWorld._43 = pos.z;
			// pRenderer->ProcessDrawingCube( constant );
		};

	}
#endif // DEBUG_MODE
}

bool SceneResult::CreateRenderers( const Donya::Int2 &wholeScreenSize )
{
	bool succeeded = true;

	pRenderer = std::make_unique<RenderingHelper>();
	if ( !pRenderer->Init() ) { succeeded = false; }

	pDisplayer = std::make_unique<Donya::Displayer>();
	if ( !pDisplayer->Init() ) { succeeded = false; }

	pBloomer = std::make_unique<BloomApplier>();
	if ( !pBloomer->Init( wholeScreenSize ) ) { succeeded = false; }
	pBloomer->AssignParameter( FetchParameter().bloomParam );

	return succeeded;
}
bool SceneResult::CreateSurfaces( const Donya::Int2 &wholeScreenSize )
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
bool SceneResult::CreateShaders()
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
bool SceneResult::AreRenderersReady() const
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

Donya::Vector4x4 SceneResult::MakeScreenTransform() const
{
	constexpr Donya::Vector4x4 matViewport = Donya::Vector4x4::MakeViewport( { Common::ScreenWidthF(), Common::ScreenHeightF() } );

	const Donya::Vector4x4 matViewProj = iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix();
	return matViewProj * matViewport;
}
Donya::Collision::Box3F SceneResult::CalcCurrentScreenPlane() const
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
	auto CalcWorldPos	= [&]( const Donya::Vector2 &ssPos )
	{
		const Donya::Vector3 ssRayStart{ ssPos, 0.0f };
		const Donya::Vector3 ssRayEnd  { ssPos, 1.0f };

		const Donya::Vector3 wsRayStart	= Transform( ssRayStart,	1.0f, toWorld );
		const Donya::Vector3 wsRayEnd	= Transform( ssRayEnd,		1.0f, toWorld );

		const auto result = Donya::CalcIntersectionPoint( wsRayStart, wsRayEnd, xyPlane );
		return result.intersection;
	};

	constexpr Donya::Vector2 screenSize
	{
		Common::ScreenWidthF(),
		Common::ScreenHeightF(),
	};
	constexpr Donya::Vector2 LT{ 0.0f,			0.0f			};
	constexpr Donya::Vector2 RB{ screenSize.x,	screenSize.y	};

	const Donya::Vector3 nowLT = CalcWorldPos( LT );
	const Donya::Vector3 nowRB = CalcWorldPos( RB );

	const float halfWidth	= fabsf( nowRB.x - nowLT.x ) * 0.5f;
	const float halfHeight	= fabsf( nowRB.y - nowLT.y ) * 0.5f;

	Donya::Collision::Box3F nowScreen;
	nowScreen.pos.x  = nowLT.x + halfWidth;		// Specify center
	nowScreen.pos.y  = nowLT.y - halfHeight;	// Specify center
	nowScreen.pos.z  = 0.0f;
	nowScreen.size.x = halfWidth;
	nowScreen.size.y = halfHeight;
	nowScreen.size.z = FLT_MAX;
	return nowScreen;
}

void SceneResult::CameraInit()
{
	const auto &data = FetchParameter();

	constexpr Donya::Vector2 screenSize{ Common::ScreenWidthF(), Common::ScreenHeightF() };
	constexpr Donya::Vector2 defaultZRange{ 0.1f, 500.0f };

	iCamera.Init				( Donya::ICamera::Mode::Look );
	iCamera.SetZRange			( defaultZRange.x, defaultZRange.y );
	iCamera.SetFOV				( ToRadian( data.camera.fovDegree ) );
	iCamera.SetScreenSize		( screenSize );

	lightCamera.Init			( Donya::ICamera::Mode::Look );
	lightCamera.SetZRange		( data.shadowMap.nearDistance, data.shadowMap.projectDistance.z );
	lightCamera.SetScreenSize	( data.shadowMap.projectDistance.XY() );

	AssignCameraPos();

	iCamera.SetProjectionPerspective();
	lightCamera.SetProjectionOrthographic();

	Effect::Admin::Get().SetProjectionMatrix( iCamera.GetProjectionMatrix() );

	// I can setting a configuration,
	// but current data is not changed immediately.
	// So update here.
	Donya::ICamera::Controller moveInitPoint{};
	moveInitPoint.SetNoOperation();
	moveInitPoint.slerpPercent = 1.0f;
	iCamera.Update( moveInitPoint );
	lightCamera.Update( moveInitPoint );
}
void SceneResult::AssignCameraPos()
{
	const auto &data = FetchParameter();
	
	iCamera.SetPosition  ( data.camera.offsetPos   );
	iCamera.SetFocusPoint( data.camera.offsetFocus );
	
	const Donya::Vector3 playerPos = GetPlayerPosition();
	const Donya::Vector3 offset = -data.shadowMap.projectDirection * data.shadowMap.offsetDistance;
	lightCamera.SetPosition  ( playerPos + offset );
	lightCamera.SetFocusPoint( playerPos );
}
void SceneResult::CameraUpdate()
{
	const auto &data = FetchParameter();

#if USE_IMGUI
	// Apply for be able to see an adjustment immediately
	{
		iCamera.SetFOV( ToRadian( data.camera.fovDegree ) );
		iCamera.SetProjectionPerspective();
		
		if ( nowDebugMode ) // Don't call AssignCameraPos() when debug-mode
		{
			const Donya::Vector3 playerPos	= GetPlayerPosition();
			const Donya::Vector3 offset		= -data.shadowMap.projectDirection * data.shadowMap.offsetDistance;
			lightCamera.SetPosition  ( playerPos + offset );
			lightCamera.SetFocusPoint( playerPos );
		}

		lightCamera.SetZRange( data.shadowMap.nearDistance, data.shadowMap.projectDistance.z );
		lightCamera.SetScreenSize( data.shadowMap.projectDistance.XY() );
		lightCamera.SetProjectionOrthographic();
	}
#endif // USE_IMGUI

	Donya::ICamera::Controller input{};
	input.SetNoOperation();
	input.slerpPercent = 1.0f;

	lightCamera.Update( input );

#if !DEBUG_MODE
	AssignCameraPos();
	iCamera.Update( input );
#else
	if ( !nowDebugMode )
	{
		AssignCameraPos();
		iCamera.Update( input );
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

void SceneResult::PlayerInit( const PlayerInitializer &initializer, const Map &terrain )
{
	if ( pPlayer )
	{
		pPlayer->Uninit();
		pPlayer.reset();
	}

	pPlayer = std::make_unique<Player>();
	pPlayer->Init( initializer, terrain, /* withAppearPerformance = */ false );
}
void SceneResult::PlayerUpdate( float elapsedTime, const Map &terrain )
{
	if ( !pPlayer ) { return; }
	// else

	Player::Input input{};
	if ( Donya::SignBit( previousTimer ) == 0 )
	{
		input.shiftGuns.front() = true;
	}
	pPlayer->Update( elapsedTime, input, terrain );
}
void SceneResult::PlayerPhysicUpdate( float elapsedTime, const Map &terrain )
{
	if ( !pPlayer ) { return; }
	// else

	pPlayer->PhysicUpdate( elapsedTime, terrain, -FLT_MAX, FLT_MAX );
}
Donya::Vector3 SceneResult::GetPlayerPosition() const
{
	return ( pPlayer ) ? pPlayer->GetPosition() : playerIniter.GetWorldInitialPos();
}

void SceneResult::RegenerateEnemies( const Donya::Vector3 &targetPos )
{
	const auto &source = FetchParameter().enemies;
	const size_t count = source.size();

	enemies.resize( count );
	for ( size_t i = 0; i < count; ++i )
	{
		auto &pIt = enemies[i];

		if ( pIt ) { pIt->Uninit(); }
		pIt = std::make_unique<Enemy::SuperBallMachine>();
		pIt->Init( source[i], targetPos, unlimitedScreen );
	}

	extinctTime = -1.0f;
}
void SceneResult::EnemyUpdate( float elapsedTime, const Donya::Vector3 &targetPos )
{
	const auto &data	= FetchParameter();
	const auto &source	= data.enemies;
	const size_t count	= source.size();

	if ( enemies.size() != count )
	{
		enemies.resize( count );
	}

	auto NowTiming = [&]( float verifySecond )
	{
		const int currSign = Donya::SignBit( verifySecond - currentTimer  );
		const int prevSign = Donya::SignBit( verifySecond - previousTimer );
		return ( currSign != prevSign || !currSign );
	};
	if ( NowTiming( data.firstGenerateEnemySecond ) )
	{
		RegenerateEnemies( targetPos );
	}
	else
	if ( 0.0f <= extinctTime && NowTiming( data.generateEnemyIntervalSecond + extinctTime ) )
	{
		if ( data.firstGenerateEnemySecond < currentTimer )
		{
			RegenerateEnemies( targetPos );
		}
	}

	bool extincted = true;
	
	for ( size_t i = 0; i < count; ++i )
	{
		auto &pIt = enemies[i];
		if ( !pIt ) { continue; }
		// else

		if ( !pIt->NowWaiting() && !pIt->WillDie() )
		{
			extincted = false;
		}

		pIt->Update( elapsedTime, targetPos, unlimitedScreen );
	}

	if ( extincted && extinctTime < 0.0f )
	{
		extinctTime = currentTimer;
	}
}
void SceneResult::EnemyPhysicUpdate( float elapsedTime, const Map &terrain )
{
	for ( auto &pIt : enemies )
	{
		if ( !pIt ) { continue; }
		// else
		pIt->PhysicUpdate( elapsedTime, terrain );
	}
}
void SceneResult::EnemyDraw( RenderingHelper *pRenderer )
{
	for ( auto &pIt : enemies )
	{
		if ( !pIt ) { continue; }
		// else
		pIt->Draw( pRenderer );
	}
}

void SceneResult::ClearBackGround() const
{
	constexpr Donya::Vector3 gray = Donya::Color::MakeColor( Donya::Color::Code::GRAY );
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );

	if ( pShadowMap		) { pShadowMap->Clear( Donya::Color::Code::BLACK );			}
	if ( pScreenSurface	) { pScreenSurface->Clear( Donya::Vector4{ gray, 1.0f } );	}
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
void SceneResult::StartFade()
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeSecond	= Fader::GetDefaultCloseSecond();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneResult::ReturnResult()
{
	// TODO: Temporary condition, should fix this
	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Title;
		return change;
	}

	bool requestPause	= controller.Trigger( Donya::Gamepad::Button::START ) || controller.Trigger( Donya::Gamepad::Button::SELECT ) || Donya::Keyboard::Trigger( 'P' );
	bool allowPause		= !Fader::Get().IsExist();
	if ( 0 && requestPause && allowPause )
	{
	#if DEBUG_MODE
		Donya::Sound::Play( Music::DEBUG_Weak );
	#endif // DEBUG_MODE

		Scene::Result pause{};
		pause.AddRequest( Scene::Request::ADD_SCENE );
		pause.sceneType = Scene::Type::Pause;
		return pause;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
void SceneResult::UseImGui()
{	
	ImGui::SetNextWindowBgAlpha( 0.6f );
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else

	constexpr int stageNo = Definition::StageNumber::Result();

	sceneParam.ShowImGuiNode( u8"リザルトシーンのパラメータ" );

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
			playerIniter.RemakeByCSV( loadedData );

			if ( thenSave )
			{
				playerIniter.SaveBin ( stageNo );
				playerIniter.SaveJson( stageNo );
			}

			const Map emptyMap{}; // Used for empty argument. Fali safe.
			const Map &mapRef = ( pMap ) ? *pMap : emptyMap;
			PlayerInit( playerIniter, mapRef );
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
			static BufferType bufferMap			{ "Map"						};
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
				ProcessOf( bufferMap,	ApplyToPlayer	);

				if ( pMap ) { pMap->ReloadModel( readStageNumber ); }
			}

			ImGui::InputInt ( u8"読み込むステージ番号",	&readStageNumber );
			ImGui::InputText( u8"接頭辞",				bufferPrefix.data(),	bufferSize );
			ImGui::InputText( u8"ディレクトリ",			bufferDirectory.data(),	bufferSize );
			ImGui::InputText( u8"識別子・マップ＆自機",	bufferMap.data(),		bufferSize );
			ImGui::InputText( u8"拡張子",				bufferExtension.data(),	bufferSize );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"個別ロード" ) )
		{
			static bool applyMap	= true;
			static bool applyPlayer	= true;

			ImGui::Checkbox( u8"マップに適用",	&applyMap		); ImGui::SameLine();
			ImGui::Checkbox( u8"自機に適用",		&applyPlayer	);
		
			if ( ImGui::Button( u8"CSVファイルを読み込む" ) )
			{
				const auto filePath	= FetchStageFilePathByCommonDialog();
				const auto loader	= PrepareCSVData( filePath );
				if ( IsValidData( loader ) )
				{
					Bullet::Admin::Get().ClearInstances();

					if ( applyMap		) { ApplyToMap		( loader );	}
					if ( applyPlayer	) { ApplyToPlayer	( loader ); }
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

		if ( pMap ) { pMap->ShowImGuiNode( u8"マップの現在", stageNo ); }
		ImGui::Text( "" );

		if ( pPlayer ) { pPlayer->ShowImGuiNode( u8"自機の現在" ); }
		Player::UpdateParameter( u8"自機のパラメータ" );
		ImGui::Text( "" );

		Bullet::Admin::Get().ShowImGuiNode( u8"弾の現在" );
		Bullet::Parameter::Update( u8"弾のパラメータ" );
		ImGui::Text( "" );

		const Donya::Vector3 playerPos = GetPlayerPosition();
		if ( ImGui::TreeNode( u8"敵たちの現在" ) )
		{
			const size_t count = enemies.size();
			for ( size_t i = 0; i < count; ++i )
			{
				auto &pIt = enemies[i];
				if ( !pIt ) { continue; }
				// else
				pIt->ShowImGuiNode( Donya::MakeArraySuffix( i ) );
			}

			ImGui::TreePop();
		}
		Enemy::Parameter::Update( u8"敵のパラメータ" );
		ImGui::Text( "" );

		Meter::Parameter::Update( u8"メータのパラメータ" );
		ImGui::Text( "" );

		Effect::Admin::Get().ShowImGuiNode( u8"エフェクトのパラメータ" );

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

	if ( ImGui::Button( u8"リザルトシーンを初期化" ) )
	{
		Init();
	}

	ImGui::End();
}
#endif // USE_IMGUI
