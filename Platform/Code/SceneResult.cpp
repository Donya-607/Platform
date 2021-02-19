#include "SceneResult.h"

#include <algorithm>				// Use std::find

#undef max
#undef min
#include "cereal/types/vector.hpp"

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

#include "Bloom.h"
#include "Common.h"
#include "Effect/EffectAdmin.h"
#include "Enemies/SuperBallMachine.h"
#include "Fader.h"
#include "FilePath.h"
#include "FontHelper.h"
#include "Music.h"
#include "Parameter.h"
#include "PointLightStorage.h"
#include "RenderingStuff.h"
#include "SaveData.h"
#include "SceneConstant.h"
#include "StageNumber.h"

#if DEBUG_MODE
#include "CSVLoader.h"
#pragma comment( lib, "comdlg32.lib" ) // Used for common-dialog
#endif // DEBUG_MOD

namespace
{
#if DEBUG_MODE
	constexpr bool IOFromBinary = false;
#else
	constexpr bool IOFromBinary = true;
#endif // DEBUG_MODE

#if USE_IMGUI
	static bool dontAdvanceTimer = false;
#endif // USE_IMGUI
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

		std::vector<float> useShotTimings; // Second. The timing counts from beginning.
		
		float finishPerformanceSecond		= 1.0f;
		float waitSecUntilFade				= 1.0f;

		Donya::Vector2 ssMeterDrawPos;
		Donya::Vector2 ssMeterDrawScale{ 1.0f, 1.0f };
		struct ShiftInput
		{
			Donya::Vector2 ssPos;
			Donya::Vector2 ssScale{ 1.0f, 1.0f };
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( ssPos	),
					CEREAL_NVP( ssScale	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		ShiftInput drawingShiftBack;
		ShiftInput drawingShiftAdvance;

		float firstShiftGunSecond			= 1.0f;

		float beSkippableSecond				= 1.0f; // The user can be skip this result scene
		float acceptSkipSecond				= 1.0f; // The skip request is accept, but can not skip until "beSkippableSecond".
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
					CEREAL_NVP( generateEnemyIntervalSecond	),
					CEREAL_NVP( useShotTimings				),
					CEREAL_NVP( finishPerformanceSecond		)
				);
			}
			if ( 2 <= version )
			{
				archive( CEREAL_NVP( waitSecUntilFade ) );
			}
			if ( 3 <= version )
			{
				archive
				(
					CEREAL_NVP( ssMeterDrawPos		),
					CEREAL_NVP( ssMeterDrawScale	),
					CEREAL_NVP( drawingShiftBack	),
					CEREAL_NVP( drawingShiftAdvance	)
				);
			}
			if ( 4 <= version )
			{
				archive( CEREAL_NVP( firstShiftGunSecond ) );
			}
			if ( 5 <= version )
			{
				archive
				(
					CEREAL_NVP( beSkippableSecond	),
					CEREAL_NVP( acceptSkipSecond	)
				);
			}
			if ( 6 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode()
		{
			if ( ImGui::TreeNode( u8"�J����" ) )
			{
				ImGui::DragFloat ( u8"��p�iDegree�j",	&camera.fovDegree,		0.1f  );
				ImGui::DragFloat3( u8"���g�̍��W",		&camera.offsetPos.x,	0.01f );
				ImGui::DragFloat3( u8"�����_�̍��W",		&camera.offsetFocus.x,	0.01f );

				ImGui::TreePop();
			}

			ImGui::Helper::ShowDirectionalLightNode( u8"���s��", &directionalLight );

			if ( ImGui::TreeNode( u8"�V���h�E�}�b�v�֘A" ) )
			{
				ImGui::ColorEdit3( u8"�e�̐F",					&shadowMap.color.x );
				ImGui::DragFloat ( u8"�A�N�l�p�̃o�C�A�X",		&shadowMap.bias,				0.01f );
				ImGui::DragFloat ( u8"���@����̋���",			&shadowMap.offsetDistance,		0.1f  );
				ImGui::DragFloat3( u8"�ʂ������i�P�ʃx�N�g���j",	&shadowMap.projectDirection.x,	0.01f );
				ImGui::DragFloat3( u8"�ʂ��͈͂w�x�y",			&shadowMap.projectDistance.x,	1.0f  );
				ImGui::DragFloat ( u8"Z-Near",					&shadowMap.nearDistance,		1.0f  );

				shadowMap.offsetDistance	= std::max( 0.01f, shadowMap.offsetDistance );
				shadowMap.projectDistance.x	= std::max( 0.01f, shadowMap.projectDistance.x );
				shadowMap.projectDistance.y	= std::max( 0.01f, shadowMap.projectDistance.y );
				shadowMap.projectDistance.z	= std::max( shadowMap.nearDistance + 1.0f, shadowMap.projectDistance.z );

				static bool alwaysNormalize = false;
				if ( ImGui::Button( u8"�ʂ������𐳋K��" ) || alwaysNormalize )
				{
					shadowMap.projectDirection.Normalize();
				}
				ImGui::Checkbox( u8"��ɐ��K������", &alwaysNormalize );

				ImGui::TreePop();
			}

			bloomParam.ShowImGuiNode( u8"�u���[���֘A" );

			if ( enemies.empty() )
			{
				Enemy::InitializeParam arg{};
				enemies.emplace_back( arg );
			}
			if ( ImGui::TreeNode( u8"�G�֘A" ) )
			{
				ImGui::DragFloat( u8"���񐶐��܂ł̑ҋ@�b��",			&firstGenerateEnemySecond,		0.01f );
				ImGui::DragFloat( u8"�S�Ō�ɍĐ�������܂ł̕b��",	&generateEnemyIntervalSecond,	0.01f );
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
			if ( ImGui::TreeNode( u8"�^�C�~���O�֘A" ) )
			{
				ImGui::DragFloat( u8"�����؂�ւ���܂ł̑ҋ@�b��",		&firstShiftGunSecond,		0.01f );
				ImGui::DragFloat( u8"���o���I����܂ł̕b��",				&finishPerformanceSecond,	0.01f );
				ImGui::DragFloat( u8"���o��t�F�[�h�̂܂ł̕b��",			&waitSecUntilFade,			0.01f );
				ImGui::DragFloat( u8"���o�X�L�b�v���\�ɂȂ�܂ł̕b��",	&beSkippableSecond,			0.01f );
				ImGui::DragFloat( u8"���o�X�L�b�v���͂̎�t�J�n�b��",		&acceptSkipSecond,			0.01f );
				firstShiftGunSecond		= std::max( 0.0f, firstShiftGunSecond		);
				finishPerformanceSecond	= std::max( 0.0f, finishPerformanceSecond	);
				waitSecUntilFade		= std::max( 0.0f, waitSecUntilFade			);
				beSkippableSecond		= std::max( 0.0f, beSkippableSecond			);
				acceptSkipSecond		= std::max( 0.0f, acceptSkipSecond			);

				if ( ImGui::TreeNode( u8"�V���b�g���͂���" ) )
				{
					ImGui::Helper::ResizeByButton( &useShotTimings, 0.0f );

					const int count = useShotTimings.size();
					for ( int i = 0; i < count; ++i )
					{
						ImGui::DragFloat( Donya::MakeArraySuffix( i ).c_str(), &useShotTimings[i], 0.01f );
						useShotTimings[i] = std::max( 0.0f, useShotTimings[i] );
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"�A�C�R���֘A" ) )
			{
				ImGui::DragFloat2( u8"��`����W�i���[�^�̂��́j",	&ssMeterDrawPos.x );
				ImGui::DragFloat2( u8"��`��X�P�[��",			&ssMeterDrawScale.x, 0.01f );

				auto Show = []( const char *nodeCaption, ShiftInput *p )
				{
					if ( !ImGui::TreeNode( nodeCaption ) ) { return; }
					// else

					ImGui::DragFloat2( u8"���W",		&p->ssPos.x );
					ImGui::DragFloat2( u8"�X�P�[��",	&p->ssScale.x, 0.01f );

					ImGui::TreePop();
				};
				Show( u8"�O�ցE����ւ��\��", &drawingShiftBack );
				Show( u8"���ցE����ւ��\��", &drawingShiftAdvance );

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

	bool NowTiming( float verifySecond, float currTime, float prevTime )
	{
		const int currSign = Donya::SignBit( verifySecond - currTime );
		const int prevSign = Donya::SignBit( verifySecond - prevTime );
		return ( currSign != prevSign || !currSign );
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
CEREAL_CLASS_VERSION( SceneParam,				5 )
CEREAL_CLASS_VERSION( SceneParam::ShiftInput,	0 )

void SceneResult::Init()
{
	SaveData::Admin::Get().Load();

	Donya::Sound::Play( Music::BGM_Result );
#if DEBUG_MODE
	// Donya::Sound::AppendFadePoint( Music::BGM_Result, 0.0f, 0.0f, true );
#endif // DEBUG_MODE

	sceneParam.LoadParameter();
	Input::LoadParameter();
	const auto &data = FetchParameter();

	auto &renderer = RenderingStuffInstance::Get();
	renderer.AssignBloomParameter( data.bloomParam );
	renderer.ClearBuffers();

	constexpr int stageNo = Definition::StageNumber::Result();

	bool result{};

	pInputExplainer = std::make_unique<Input::Explainer>();
	result = pInputExplainer->Init();
	assert( result );

	willSkip		= false;
	currentTimer	= 0.0f;
	previousTimer	= 0.0f;
	extinctTime		= -1.0f;
	arriveTime		= 0.0f;

	pMap = std::make_unique<Map>();
	pMap->Init( stageNo, /* reloadModel = */ false );
	centerPos = CalcCenterPoint( *pMap );

	playerIniter.LoadParameter( stageNo );
	PlayerInit( playerIniter, *pMap );

	pMeter = std::make_unique<Meter::Drawer>();
	pMeter->Init( 0.0f, 0.0f, 0.0f );
	pMeter->SetDrawOption( data.ssMeterDrawPos, pPlayer->GetThemeColor(), data.ssMeterDrawScale );

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
		RenderingStuffInstance::Get().AssignBloomParameter( FetchParameter().bloomParam );

		Effect::Admin::Get().SetProjectionMatrix( iCamera.GetProjectionMatrix() );
	}
#endif // USE_IMGUI

	PointLightStorage::Get().Clear();

	controller.Update();

	previousTimer = currentTimer;
	currentTimer += elapsedTime;
#if USE_IMGUI
	if ( dontAdvanceTimer )
	{
		currentTimer = previousTimer;
	}
#endif // USE_IMGUI

	const auto &data = FetchParameter();

	// State advacing
	if ( status == State::Performance )
	{
		if ( NowTiming( data.finishPerformanceSecond, currentTimer, previousTimer ) )
		{
			status = State::Wait;
		}
	}
	else if ( !Fader::Get().IsExist() )
	{
		const float fadeTiming = arriveTime + data.waitSecUntilFade;
		if ( NowTiming( fadeTiming, currentTimer, previousTimer ) )
		{
			StartFade();
		}
	}

	// Skip this scene
	if ( !Fader::Get().IsExist() )
	{
		if ( data.acceptSkipSecond <= currentTimer && !willSkip )
		{
			willSkip = WantToSkip();
		}

		if ( data.beSkippableSecond <= currentTimer && willSkip )
		{
			StartFade();
		}
	}


	if ( pMap ) { pMap->Update( elapsedTime ); }
	const Map emptyMap{}; // Used for empty argument. Fali safe.
	const Map &mapRef = ( pMap ) ? *pMap : emptyMap;

	PlayerUpdate( elapsedTime, mapRef );
	EnemyUpdate( elapsedTime, GetPlayerPosition() );
	Bullet::Admin::Get().Update( elapsedTime, currentScreen );

	PlayerPhysicUpdate( elapsedTime, mapRef );
	EnemyPhysicUpdate( elapsedTime, mapRef );
	Bullet::Admin::Get().PhysicUpdate( elapsedTime, mapRef );

	currentScreen = CalcCurrentScreenPlane();
	CameraUpdate();

	Collision_BulletVSBullet();
	Collision_BulletVSEnemy();

	if ( pInputExplainer ) { pInputExplainer->Update( elapsedTime ); }

	return ReturnResult();
}

void SceneResult::Draw( float elapsedTime )
{
	ClearBackGround();

	RenderingStuff *p = RenderingStuffInstance::Get().Ptr();
	if ( !p ) { return; }
	// else

	using Target = Definition::DrawTarget;

	auto UpdateSceneConstant	= [&]( const Donya::Model::Constants::PerScene::DirectionalLight &directionalLight, const Donya::Vector4 &eyePos, const Donya::Vector4x4 &viewMatrix, const Donya::Vector4x4 &viewProjectionMatrix )
	{
		Donya::Model::Constants::PerScene::Common constant{};
		constant.directionalLight	= directionalLight;
		constant.eyePosition		= eyePos;
		constant.viewMatrix			= viewMatrix;
		constant.viewProjMatrix		= viewProjectionMatrix;
		p->renderer.UpdateConstant( constant );
	};
	auto DrawObjects			= [&]( Target option, bool castShadow )
	{
		auto Drawable = [&option]( Target verify )
		{
			return scast<int>( option & verify ) != 0;
		};

		// The drawing priority is determined by the priority of the information.

		/*
		( castShadow )
		? p->renderer.ActivateShaderShadowStatic()
		: p->renderer.ActivateShaderNormalStatic();

		( castShadow )
		? p->renderer.DeactivateShaderShadowStatic()
		: p->renderer.DeactivateShaderNormalStatic();
		*/


		( castShadow )
		? p->renderer.ActivateShaderShadowSkinning()
		: p->renderer.ActivateShaderNormalSkinning();

		if ( Drawable( Target::Player	) && pPlayer	)	{ pPlayer->Draw				( &p->renderer ); }
		if ( Drawable( Target::Vision	) && pPlayer	)	{ pPlayer->DrawVision		( &p->renderer ); }
		if ( Drawable( Target::Enemy	) )					{ EnemyDraw					( &p->renderer ); }
		if ( Drawable( Target::Bullet	) )					{ Bullet::Admin::Get().Draw	( &p->renderer ); }

		( castShadow )
		? p->renderer.DeactivateShaderShadowSkinning()
		: p->renderer.DeactivateShaderNormalSkinning();
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
	p->renderer.ActivateSamplerModel( Donya::Sampler::Defined::Aniso_Wrap );
	p->renderer.ActivateSamplerNormal( Donya::Sampler::Defined::Point_Wrap );

	p->shadowMap.SetRenderTarget();
	p->shadowMap.SetViewport();
	// Make the shadow map
	{
		// Update scene constant as light source
		{
			Donya::Model::Constants::PerScene::DirectionalLight tmpDirLight{};
			tmpDirLight.direction = Donya::Vector4{ data.shadowMap.projectDirection.Unit(), 0.0f };
			UpdateSceneConstant( tmpDirLight, lightPos, LV, LVP );
		}
		p->renderer.ActivateConstantScene();

		DrawObjects( Target::All ^ Target::Vision, /* castShadow = */ true );

		p->renderer.DeactivateConstantScene();
	}
	Donya::Surface::ResetRenderTarget();

	p->screenSurface.SetRenderTarget();
	p->screenSurface.SetViewport();
	// Draw normal scene with shadow map
	{
		RenderingHelper::ShadowConstant shadowConstant{};

		// Update scene and shadow constants
		{
			UpdateSceneConstant( data.directionalLight, cameraPos, V, VP );

			shadowConstant.lightProjMatrix	= LVP;
			shadowConstant.shadowColor		= data.shadowMap.color;
			shadowConstant.shadowBias		= data.shadowMap.bias;
			p->renderer.UpdateConstant( shadowConstant );
		}
		// Update point light constant
		{
			p->renderer.UpdateConstant( PointLightStorage::Get().GetStorage() );
		}

		p->renderer.ActivateConstantScene();
		p->renderer.ActivateConstantPointLight();
		p->renderer.ActivateConstantShadow();
		p->renderer.ActivateSamplerShadow( Donya::Sampler::Defined::Point_Border_White );
		p->renderer.ActivateShadowMap( p->shadowMap );

		constexpr Target option = Target::All ^ Target::Bullet ^ Target::Vision;
		DrawObjects( option, /* castShadow = */ false );

		// Disable shadow
		{
			p->renderer.DeactivateConstantShadow();
			shadowConstant.shadowBias = 1.0f; // Make the pixel to nearest
			p->renderer.UpdateConstant( shadowConstant );
			p->renderer.ActivateConstantShadow();
		}

		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::NoTest_Write );
		DrawObjects( Target::Bullet, /* castShadow = */ false );
		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLess );

		DrawObjects( Target::Vision, /* castShadow = */ false );

		p->renderer.DeactivateShadowMap( p->shadowMap );
		p->renderer.DeactivateSamplerShadow();
		p->renderer.DeactivateConstantShadow();
		p->renderer.DeactivateConstantPointLight();
		p->renderer.DeactivateConstantScene();
	}
	Donya::Surface::ResetRenderTarget();

	p->renderer.DeactivateSamplerModel();
	Donya::Rasterizer::Deactivate();
	Donya::DepthStencil::Deactivate();

	// Generate the buffers of bloom
	{
		constexpr Donya::Vector4 black{ 0.0f, 0.0f, 0.0f, 1.0f };
		p->bloomer.ClearBuffers( black );

		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLessEq );
		Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullNone );

		const float oldDepth = Donya::Sprite::GetDrawDepth();
		Donya::Sprite::SetDrawDepth( 0.0f );
		Donya::Sampler::SetPS( Donya::Sampler::Defined::Linear_Border_Black, 0 );
		p->bloomer.WriteLuminance( p->screenSurface );
		Donya::Sampler::ResetPS( 0 );
		Donya::Sprite::SetDrawDepth( oldDepth );

		Donya::Sampler::SetPS( Donya::Sampler::Defined::Aniso_Wrap, 0 );
		p->bloomer.WriteBlur();
		Donya::Sampler::ResetPS( 0 );

		Donya::Rasterizer::Deactivate();
		Donya::DepthStencil::Deactivate();
	}

	p->screenSurface.SetRenderTarget();
	p->screenSurface.SetViewport();
	// Draw effects
	{
		// Ignore the depth of game,
		// And do not affect the luminance of game.
		p->screenSurface.ClearDepthStencil();
		Effect::Admin::Get().Draw();
	}
	Donya::Surface::ResetRenderTarget();

	Donya::SetDefaultRenderTargets();

	const Donya::Vector2 screenSurfaceSize = p->screenSurface.GetSurfaceSizeF();

	Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLessEq );
	Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullNone );
	// Draw the scene to screen
	{
		Donya::Sampler::SetPS( Donya::Sampler::Defined::Aniso_Wrap, 0 );
		Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );

		p->quadShader.VS.Activate();
		p->quadShader.PS.Activate();

		p->screenSurface.SetRenderTargetShaderResourcePS( 0U );

		p->displayer.Draw
		(
			screenSurfaceSize,
			Donya::Vector2::Zero()
		);

		p->screenSurface.ResetShaderResourcePS( 0U );

		p->quadShader.PS.Deactivate();
		p->quadShader.VS.Deactivate();

		Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );
		Donya::Sampler::ResetPS( 0 );
	}
	Donya::DepthStencil::Deactivate();

	Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::NoTest_NoWrite );
	// Add the bloom buffers
	{
		const float oldDepth = Donya::Sprite::GetDrawDepth();
		Donya::Sprite::SetDrawDepth( 0.0f );

		Donya::Blend::Activate( Donya::Blend::Mode::ADD_NO_ATC );
		p->bloomer.DrawBlurBuffers( screenSurfaceSize );
		Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );

		Donya::Sprite::SetDrawDepth( oldDepth );
	}
	Donya::DepthStencil::Deactivate();

	Donya::Rasterizer::Deactivate();

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		if ( pPlayer	) { pPlayer->DrawHitBox	( &p->renderer, VP );					}
		if ( pMap		) { pMap->DrawHitBoxes	( currentScreen, &p->renderer, VP );	}
		Bullet::Admin::Get().DrawHitBoxes		( &p->renderer, VP );
		for ( const auto &pIt : enemies ) { if ( pIt ) { pIt->DrawHitBox( &p->renderer, VP ); } }
	}
#endif // DEBUG_MODE

	const float oldDepth = Donya::Sprite::GetDrawDepth();
	Donya::Sprite::SetDrawDepth( 0.0f );

	const auto pFontRenderer = FontHelper::GetRendererOrNullptr( FontAttribute::Main );
	if ( pFontRenderer )
	{
		constexpr Donya::Vector2 pivot{ 0.5f, 0.5f };
		constexpr Donya::Vector2 center{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };

		pFontRenderer->Draw
		(
			L"YOU GOT A NEW WEAPON!",
			center + Donya::Vector2{ 0.0f, -320.0f },
			pivot
		);

		Donya::Sprite::Flush();
	}

	if ( pMeter )
	{
		pMeter->DrawIcon( Meter::Icon::Player );
	}
	if ( pInputExplainer )
	{
		const auto ssOffset			= data.ssMeterDrawPos;
		const bool showController	= controller.IsConnected();
		auto Draw = [&]( Input::Type type, const SceneParam::ShiftInput &data )
		{
			pInputExplainer->Draw( type, showController, data.ssPos + ssOffset, data.ssScale );
		};
		Draw( Input::Type::ShiftWeapon_Back,	data.drawingShiftBack		);
		Draw( Input::Type::ShiftWeapon_Advance,	data.drawingShiftAdvance	);
	}

	Donya::Sprite::SetDrawDepth( oldDepth );

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
			p->renderer.ProcessDrawingCube( constant );
		};

		constant.drawColor = { 1.0f, 0.0f, 0.0f, 0.7f };
		currentScreen.size.z = 1.0f;
		//DrawCube( currentScreen.pos, currentScreen.size );
		currentScreen.size.z = FLT_MAX * 0.5f;
	}
#endif // DEBUG_MODE
}

Donya::Vector3 SceneResult::CalcCenterPoint( const Map &terrain ) const
{
	const auto &tiles = terrain.GetTiles();

	Donya::Vector3 max{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
	Donya::Vector3 min{ +FLT_MAX, +FLT_MAX, +FLT_MAX };
	Donya::Vector3 var;
	for ( const auto &row : tiles )
	{
		for ( const auto &col : row )
		{
			if ( !col ) { continue; }
			// else

			var = col->GetPosition();

			max.x = std::max( var.x, max.x );
			max.y = std::max( var.y, max.y );
			max.z = std::max( var.z, max.z );

			min.x = std::min( var.x, min.x );
			min.y = std::min( var.y, min.y );
			min.z = std::min( var.z, min.z );
		}
	}

	var.x = fabsf( max.x - min.x ) * 0.5f;
	var.y = fabsf( max.y - min.y ) * 0.5f;
	var.z = fabsf( max.z - min.z ) * 0.5f;
	return min + var;
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

bool SceneResult::WantToSkip() const
{
	if ( controller.IsConnected() )
	{
		using Btn = Donya::Gamepad::Button;
		if ( controller.Press( Btn::START  ) ) { return true; }
		if ( controller.Press( Btn::SELECT ) ) { return true; }
	}
	else
	{
		// More than two buttons are pressed
		{
			constexpr unsigned int considerCount = 2;
			unsigned int pressedCount = 0;

			const auto input = Input::MakeCurrentInput( controller, Donya::Vector2::Zero() );

			pressedCount += Input::CountTrue( input.useJumps );
			if ( considerCount <= pressedCount ) { return true; }

			pressedCount += Input::CountTrue( input.useShots );
			if ( considerCount <= pressedCount ) { return true; }

			pressedCount += Input::CountTrue( input.useDashes );
			if ( considerCount <= pressedCount ) { return true; }

			pressedCount += Input::CountTrue( input.shiftGuns );
			if ( considerCount <= pressedCount ) { return true; }
		}

		// Or the direction keys are pressed as conflicting
		{
			const int L = ( Donya::Keyboard::Press( VK_LEFT	) ) ? 1 : 0;
			const int R = ( Donya::Keyboard::Press( VK_RIGHT) ) ? 1 : 0;
			const int U = ( Donya::Keyboard::Press( VK_UP	) ) ? 1 : 0;
			const int D = ( Donya::Keyboard::Press( VK_DOWN	) ) ? 1 : 0;
			if ( L && R ) { return true; }
			if ( U && D ) { return true; }
		}
	}

	return false;
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
	prevPlayerPos = pPlayer->GetPosition();
}
void SceneResult::PlayerUpdate( float elapsedTime, const Map &terrain )
{
	if ( !pPlayer ) { return; }
	// else

	const auto &data = FetchParameter();

	Player::Input input{};
	if ( NowTiming( data.firstShiftGunSecond, currentTimer, previousTimer ) )
	{
		input.shiftGuns.front() = 1;
		if ( pInputExplainer )
		{
			pInputExplainer->Notify();
		}
	}

#if DEBUG_MODE
	input.useShots = Input::MakeCurrentInput( controller, {} ).useShots;
#endif // DEBUG_MODE

	switch ( status )
	{
	case State::Performance:
		{
			const auto &timings = data.useShotTimings;
			for ( const auto &sec : timings )
			{
				if ( NowTiming( sec, currentTimer, previousTimer ) )
				{
					input.useShots.front() = true;
					break;
				}
			}
		}
		break;
	case State::Wait:
		{
			const auto playerPos = pPlayer->GetPosition();

			// Center of screen
			Donya::Vector3 destination = centerPos;
			destination.y = playerPos.y;

			const auto prevSign = Donya::SignBit( destination.x - prevPlayerPos.x	);
			const auto currSign = Donya::SignBit( destination.x - playerPos.x		);

			// Arriving to destination
			if ( currSign != prevSign || !currSign )
			{
				// In first time
				if ( !IsZero( prevPlayerPos.x - playerPos.x ) )
				{
					pPlayer->PerformWinning();
					arriveTime = currentTimer;
					if ( pInputExplainer )
					{
						pInputExplainer->Notify();
					}
				}
			}
			// Head to initial position
			else
			{
				input.headToDestination = true;
				input.wsDestination = destination;
			}
		}
		break;
	default: break;
	}

	pPlayer->Update( elapsedTime, input, terrain );

	if ( pMeter )
	{
		pMeter->SetDrawOption( data.ssMeterDrawPos, pPlayer->GetThemeColor(), data.ssMeterDrawScale );
	}
}
void SceneResult::PlayerPhysicUpdate( float elapsedTime, const Map &terrain )
{
	if ( !pPlayer ) { return; }
	// else

	prevPlayerPos = pPlayer->GetPosition();
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

	if ( NowTiming( data.firstGenerateEnemySecond, currentTimer, previousTimer ) )
	{
		RegenerateEnemies( targetPos );
	}
	else
	if ( 0.0f <= extinctTime && NowTiming( data.generateEnemyIntervalSecond + extinctTime, currentTimer, previousTimer ) )
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


namespace
{
	Donya::Collision::IDType ExtractPlayerID( const std::unique_ptr<Player> &pPlayer )
	{
		return ( pPlayer ) ? pPlayer->GetHurtBox().id : Donya::Collision::invalidID;
	}
	bool IsPlayerBullet( const Donya::Collision::IDType playerCollisionID, const std::shared_ptr<const Bullet::Base> &pBullet )
	{
		if ( playerCollisionID == Donya::Collision::invalidID ) { return false; }
		// else

		const auto bulletAABB	= pBullet->GetHitBox();
		const auto bulletSphere	= pBullet->GetHitSphere();
		const auto activeOwnerID= ( bulletSphere.exist ) ? bulletSphere.ownerID : bulletAABB.ownerID;
		return ( activeOwnerID == playerCollisionID ) ? true : false;
	}
	bool IsEnemyBullet( const Donya::Collision::IDType playerCollisionID, const std::shared_ptr<const Bullet::Base> &pBullet )
	{
		return !IsPlayerBullet( playerCollisionID, pBullet );
	}

	enum class SubtractorState
	{
		Not,
		HasBox,
		HasSphere
	};
	SubtractorState HasSubtractor( const std::shared_ptr<const Bullet::Base> &pBullet )
	{
		using State = SubtractorState;

		if ( !pBullet ) { return State::Not; }
		// else

		if ( pBullet->GetHitBoxSubtractor().exist ) { return State::HasBox; }
		// else

		if ( pBullet->GetHitSphereSubtractor().exist ) { return State::HasSphere; }
		// else

		return State::Not;
	}

	template<typename ObjectHitBox, typename BulletHitBox>
	bool IsHit( const ObjectHitBox &objHitBox, const std::shared_ptr<const Bullet::Base> &pBullet, const BulletHitBox &bulletHitBox, bool considerExistFlag = true )
	{
		namespace Col = Donya::Collision;

		const SubtractorState hasSubtract = HasSubtractor( pBullet );

		// Do not use subtraction version
		if ( hasSubtract == SubtractorState::Not )
		{
			return Col::IsHit( objHitBox, bulletHitBox, considerExistFlag );
		}
		// else

		// Can not use subtraction version
		if ( !pBullet )
		{
			return Col::IsHit( objHitBox, bulletHitBox, considerExistFlag );
		}
		// else

		auto IsHitImpl = [&]( const auto &hitBoxSubtractor )
		{
			const Col::Solid<BulletHitBox, decltype( hitBoxSubtractor )> a
			{
				bulletHitBox,
				hitBoxSubtractor
			};
			return Col::IsHitVSSubtracted( objHitBox, a, considerExistFlag );
		};

		if ( hasSubtract == SubtractorState::HasBox )
		{
			return IsHitImpl( pBullet->GetHitBoxSubtractor() );
		}
		// else
		if ( hasSubtract == SubtractorState::HasSphere )
		{
			return IsHitImpl( pBullet->GetHitSphereSubtractor() );
		}
		// else

		_ASSERT_EXPR( 0, L"Error: Unexpected condition!" );
		return false;
	}
	template<typename HitBoxA, typename HitBoxB>
	bool IsHit( const std::shared_ptr<const Bullet::Base> &pBulletA, const HitBoxA &hitBoxA, const std::shared_ptr<const Bullet::Base> &pBulletB, const HitBoxB &hitBoxB, bool considerExistFlag = true )
	{
		if ( !pBulletA || !pBulletB ) { return false; }
		// else

		namespace Col = Donya::Collision;

		const SubtractorState subA = HasSubtractor( pBulletA );
		const SubtractorState subB = HasSubtractor( pBulletB );

		// Do not use subtraction version
		if ( subA == SubtractorState::Not && subB == SubtractorState::Not )
		{
			return Col::IsHit( hitBoxA, hitBoxB );
		}
		// else
		
		// Do not support yet
		if ( subA != SubtractorState::Not && subB != SubtractorState::Not )
		{
			return Col::IsHit( hitBoxA, hitBoxB );
		}
		// else

		if ( subA != SubtractorState::Not )
		{
			return IsHit( hitBoxB, pBulletA, hitBoxA, considerExistFlag );
		}
		// else
		if ( subB != SubtractorState::Not )
		{
			return IsHit( hitBoxA, pBulletB, hitBoxB, considerExistFlag );
		}
		// else
		_ASSERT_EXPR( 0, L"Error: Unexpected condition!" );
		return false;
	}
}
void SceneResult::Collision_BulletVSBullet()
{
	auto &bulletAdmin = Bullet::Admin::Get();
	const size_t bulletCount = bulletAdmin.GetInstanceCount();

	std::shared_ptr<const Bullet::Base> pA = nullptr;
	std::shared_ptr<const Bullet::Base> pB = nullptr;
	auto Protectible	= []( const std::shared_ptr<const Bullet::Base> &pBullet )
	{
		using D = Definition::Damage;
		return ( pBullet ) ? D::Contain( D::Type::Protection, pBullet->GetDamage().type ) : false;
	};
	auto HitProcess		= [&]( const auto &hitBoxA, const auto &hitBoxB )
	{
		if ( !pA || !pB ) { return; }
		// else

		const bool protectibleA = Protectible( pA );
		const bool protectibleB = Protectible( pB );
		if ( protectibleA ) { pB->ProtectedBy( hitBoxA ); }
		if ( protectibleB ) { pA->ProtectedBy( hitBoxB ); }

		// Currently, there is not the hard(e.g. destructible and a lot of HP) bullet.
		// So tells "pierced" to the collided bullet(s) for easily process, in the collision between bullet and bullet.
		constexpr bool toPierce			= true;
		constexpr bool otherIsBullet	= true;
		
		// Do not call the protected bullet's method.
		// But should call the not protected one's method for play the hit SE.
		if ( !protectibleA ) { pB->CollidedToObject( toPierce, otherIsBullet ); }
		if ( !protectibleB ) { pA->CollidedToObject( toPierce, otherIsBullet ); }
	};
	
	const auto playerID = ExtractPlayerID( pPlayer );

	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pA = bulletAdmin.GetInstanceOrNullptr( i );
		if ( !pA ) { continue; }

		// Disallow collision between a protected bullet.
		// Because if allowed hitting to multiple objects in the same timing,
		// the protection attribute does not affect to some another object that collided in the same timing.
		// That bullet's collision will be disabled at next update, but I wanna apply immediately.
		if ( pA->WasProtected() ) { continue; }
		// else

		const auto aabbA		= pA->GetHitBox();
		const auto sphereA		= pA->GetHitSphere();

		const bool ownerA		= IsPlayerBullet( playerID, pA );
		const bool protectibleA	= Protectible( pA );

		for ( size_t j = i + 1; j < bulletCount; ++j )
		{
			pB = bulletAdmin.GetInstanceOrNullptr( j );
			if ( !pB ) { continue; }
			// else
			if ( pB->WasProtected() ) { continue; }
			// else

			const bool ownerB		= IsPlayerBullet( playerID, pB );
			const bool protectibleB	= Protectible( pB );
			if ( ownerA == ownerB ) { continue; }
			// else

			// Do collide if either one of bullet is destructible or protectible
			const bool wantCollide	=  pA->Destructible()	|| pB->Destructible()
									|| protectibleA			|| protectibleB;
			if ( !wantCollide ) { continue; }
			// else
			
			const auto aabbB	= pB->GetHitBox();
			const auto sphereB	= pB->GetHitSphere();

			if ( aabbA.exist )
			{
				if ( IsHit( pA, aabbA, pB, aabbB ) )
				{
					HitProcess( aabbA, aabbB );
					continue;
				}
				// else
				if ( IsHit( pA, aabbA, pB, sphereB ) )
				{
					HitProcess( aabbA, sphereB );
					continue;
				}
			}
			else
			if ( sphereA.exist )
			{
				if ( IsHit( pA, sphereA, pB, aabbB ) )
				{
					HitProcess( sphereA, aabbB );
					continue;
				}
				// else
				if ( IsHit( pA, sphereA, pB, sphereB ) )
				{
					HitProcess( sphereA, sphereB );
					continue;
				}
			}
		}
	}
}
void SceneResult::Collision_BulletVSEnemy()
{
	auto &enemyAdmin	= Enemy::Admin::Get();
	auto &bulletAdmin	= Bullet::Admin::Get();
	const size_t enemyCount		= enemies.size();
	const size_t bulletCount	= bulletAdmin.GetInstanceCount();

	// Makes every call the "FindCollidingEnemyOrNullptr" returns another enemy
	std::vector<size_t> collidedEnemyIndices{};
	auto IsAlreadyCollided				= [&]( size_t enemyIndex )
	{
		const auto result = std::find( collidedEnemyIndices.begin(), collidedEnemyIndices.end(), enemyIndex );
		return ( result != collidedEnemyIndices.end() );
	};
	auto FindCollidingEnemyOrNegative	= [&]( const std::shared_ptr<const Bullet::Base> &pOtherBullet, const auto &otherHitBox )
	{
		for ( size_t i = 0; i < enemyCount; ++i )
		{
			if ( IsAlreadyCollided( i ) ) { continue; }
			// else

			auto &pIt = enemies[i];
			if ( !pIt ) { continue; }
			// else

			if ( IsHit( pIt->GetHurtBox(), pOtherBullet, otherHitBox ) )
			{
				collidedEnemyIndices.emplace_back( i );
				return scast<int>( i );
			}
		}

		return -1;
	};

	// The bullet's hit box is only either AABB or Sphere is valid
	// (Invalid one's exist flag will false, so IsHit() will returns false)
	Donya::Collision::Box3F		otherAABB;
	Donya::Collision::Sphere3F	otherSphere;

	std::shared_ptr<const Bullet::Base> pBullet = nullptr;

	struct ProccessResult
	{
		bool collided	= false;
		bool pierced	= false;
	};
	auto Process = [&]( const auto &bulletBody )
	{
		ProccessResult result{};
		result.collided = false;
		result.pierced  = false;

		if ( !pBullet ) { return result; }
		// else

		result.pierced = true;

		int enemyIndex = FindCollidingEnemyOrNegative( pBullet, bulletBody );
		while ( 0 <= enemyIndex )
		{
			result.collided = true;
			auto &pEnemy = enemies[enemyIndex];

			pEnemy->GiveDamage( pBullet->GetDamage() );
			if ( !pEnemy->WillDie() )
			{
				result.pierced = false;
			}

			enemyIndex = FindCollidingEnemyOrNegative( pBullet, bulletBody );
		}

		return result;
	};

	const auto playerID = ExtractPlayerID( pPlayer );

	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pBullet = bulletAdmin.GetInstanceOrNullptr( i );
		if ( !pBullet ) { continue; }
		// else
		if ( pBullet->WasProtected() ) { continue; }
		// else

		if ( IsEnemyBullet( playerID, pBullet ) ) { continue; }
		// else

		otherAABB	= pBullet->GetHitBox();
		otherSphere	= pBullet->GetHitSphere();
		if ( !otherAABB.exist && !otherSphere.exist ) { continue; }
		// else

		const auto result	= ( otherAABB.exist )
							? Process( otherAABB	)
							: Process( otherSphere	);
		if ( result.collided )
		{
			pBullet->CollidedToObject( result.pierced, /* otherIsBullet = */ false );
		}

		collidedEnemyIndices.clear();
	}
}

void SceneResult::ClearBackGround() const
{
	RenderingStuffInstance::Get().ClearBuffers();

	constexpr Donya::Vector3 gray = Donya::Color::MakeColor( Donya::Color::Code::GRAY );
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );

#if DEBUG_MODE
	if ( nowDebugMode )
	{
		constexpr Donya::Vector3 teal = Donya::Color::MakeColor( Donya::Color::Code::CYAN );
		constexpr FLOAT DEBUG_COLOR[4]{ teal.x, teal.y, teal.z, 1.0f };
		Donya::ClearViews( DEBUG_COLOR );
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
	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
	#if DEBUG_MODE
		change.sceneType = Scene::Type::Title;
		// change.sceneType = Scene::Type::Result;
	#else
		change.sceneType = Scene::Type::Title;
	#endif // DEBUG_MODE
		return change;
	}

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

	sceneParam.ShowImGuiNode( u8"���U���g�V�[���̃p�����[�^" );
	ImGui::Checkbox( u8"�^�C�}�̍X�V���~�߂�", &dontAdvanceTimer );

	if ( ImGui::TreeNode( u8"�X�e�[�W�t�@�C���̓ǂݍ���" ) )
	{
		static bool thenSave = true;
		ImGui::Checkbox( u8"�ǂݍ��݁E�K�p��ɃZ�[�u����", &thenSave );

		auto PrepareCSVData	= []( const std::string &filePath )
		{
			CSVLoader loader;
			loader.Clear();

			if ( filePath.empty() || !Donya::IsExistFile( filePath ) )
			{
				std::string msg = u8"�t�@�C���I�[�v���Ɏ��s���܂����B\n";
				msg += u8"�t�@�C�����F[" + filePath + u8"]";
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
					u8"�p�[�X�Ɏ��s���܂����B",
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
		
		if ( ImGui::TreeNode( u8"�o�b�`���[�h" ) )
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

			if ( ImGui::Button( u8"�ǂݍ��݊J�n" ) )
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

				if ( pMap ) { pMap->LoadModel( readStageNumber ); }
			}

			ImGui::InputInt ( u8"�ǂݍ��ރX�e�[�W�ԍ�",	&readStageNumber );
			ImGui::InputText( u8"�ړ���",				bufferPrefix.data(),	bufferSize );
			ImGui::InputText( u8"�f�B���N�g��",			bufferDirectory.data(),	bufferSize );
			ImGui::InputText( u8"���ʎq�E�}�b�v�����@",	bufferMap.data(),		bufferSize );
			ImGui::InputText( u8"�g���q",				bufferExtension.data(),	bufferSize );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"�ʃ��[�h" ) )
		{
			static bool applyMap	= true;
			static bool applyPlayer	= true;

			ImGui::Checkbox( u8"�}�b�v�ɓK�p",	&applyMap		); ImGui::SameLine();
			ImGui::Checkbox( u8"���@�ɓK�p",		&applyPlayer	);
		
			if ( ImGui::Button( u8"CSV�t�@�C����ǂݍ���" ) )
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
			ImGui::InputInt( u8"�}�b�v���f���ɓK�p����X�e�[�W�ԍ�", &loadMapNumber );
			if ( ImGui::Button( u8"�}�b�v���f����ǂݍ���" ) && pMap )
			{
				pMap->LoadModel( loadMapNumber );
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"�e�I�u�W�F�N�g�̒���" ) )
	{
		ImGui::InputInt( u8"���݂̃��[���ԍ�", &currentRoomID );

		if ( ImGui::TreeNode( u8"�J�����̌���" ) )
		{
			ImGui::Checkbox( u8"�ړ������𔽓]����E�w", &isReverseCameraMoveX );
			ImGui::Checkbox( u8"�ړ������𔽓]����E�x", &isReverseCameraMoveY );
			ImGui::Checkbox( u8"��]�����𔽓]����E�w", &isReverseCameraRotX );
			ImGui::Checkbox( u8"��]�����𔽓]����E�x", &isReverseCameraRotY );

			auto ShowVec3 = []( const std::string &prefix, const Donya::Vector3 &v )
			{
				ImGui::Text( ( prefix + u8"[X:%5.2f][Y:%5.2f][Z:%5.2f]" ).c_str(), v.x, v.y, v.z );
			};

			const Donya::Vector3 cameraPos = iCamera.GetPosition();
			ShowVec3( u8"���݈ʒu", cameraPos );
			ImGui::Text( "" );

			const Donya::Vector3 focusPoint = iCamera.GetFocusPoint();
			ShowVec3( u8"�����_�ʒu", focusPoint );
			ImGui::Text( "" );
			ImGui::TreePop();
		}
		ImGui::Text( "" );

		if ( pMap ) { pMap->ShowImGuiNode( u8"�}�b�v�̌���", stageNo ); }
		ImGui::Text( "" );

		if ( pPlayer ) { pPlayer->ShowImGuiNode( u8"���@�̌���" ); }
		Player::UpdateParameter( u8"���@�̃p�����[�^" );
		ImGui::Text( "" );

		Bullet::Admin::Get().ShowImGuiNode( u8"�e�̌���" );
		Bullet::Parameter::Update( u8"�e�̃p�����[�^" );
		ImGui::Text( "" );

		const Donya::Vector3 playerPos = GetPlayerPosition();
		if ( ImGui::TreeNode( u8"�G�����̌���" ) )
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
		Enemy::Parameter::Update( u8"�G�̃p�����[�^" );
		ImGui::Text( "" );

		Meter::Parameter::Update( u8"���[�^�̃p�����[�^" );
		ImGui::Text( "" );

		Effect::Admin::Get().ShowImGuiNode( u8"�G�t�F�N�g�̃p�����[�^" );
		ImGui::Text( "" );

		SaveData::Admin::Get().ShowImGuiNode( u8"�Z�[�u�f�[�^�̌���" );
		ImGui::Text( "" );
		
		if ( pInputExplainer && ImGui::Button( u8"�C���v�b�g���A�s�[��" ) )
		{
			pInputExplainer->Notify();
		}
		Input::UpdateParameter( u8"�C���v�b�g�̃p�����[�^" );
		ImGui::Text( "" );

		ImGui::TreePop();
	}

	RenderingStuffInstance::Get().ShowSurfacesToImGui( u8"�T�[�t�F�X�`��" );

	if ( ImGui::Button( u8"���U���g�V�[����������" ) )
	{
		Init();
	}

	ImGui::End();
}
#endif // USE_IMGUI
