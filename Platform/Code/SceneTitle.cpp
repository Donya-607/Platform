#include "SceneTitle.h"

#include <vector>
#include <unordered_map>			// Use for imgui

#include "Donya/Blend.h"
#include "Donya/Color.h"			// Use ClearBackGround(), StartFade().
#include "Donya/Easing.h"
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
#include "Bosses/Skull.h"
#include "Bullet.h"
#include "Common.h"
#include "Enemy.h"
#include "Effect/EffectAdmin.h"
#include "Fader.h"
#include "FilePath.h"
#include "FontHelper.h"
#include "Input.h"
#include "Item.h"
#include "Math.h"					// Use CalcBezierCurve()
#include "ModelHelper.h"			// Use serialize methods
#include "Music.h"
#include "Parameter.h"
#include "PlayerParam.h"			// Use for reset the remaining
#include "PointLightStorage.h"
#include "StageNumber.h"

#undef max
#undef min

namespace
{
	constexpr const char *GetCameraStateName( SceneTitle::CameraState status )
	{
		switch ( status )
		{
		case SceneTitle::CameraState::Attract:			return "Attract";
		case SceneTitle::CameraState::Controllable:		return "Controllable";
		case SceneTitle::CameraState::StartPerformance:	return "StartPerformance";
		default: break;
		}

		// Fail safe
		return GetCameraStateName( SceneTitle::CameraState::Attract );
	}
	constexpr const char *GetPerformanceStateName( SceneTitle::PerformanceState status )
	{
		switch ( status )
		{
		case SceneTitle::PerformanceState::NotPerforming:	return "NotPerforming";
		case SceneTitle::PerformanceState::ToLeave:			return "ToLeave";
		case SceneTitle::PerformanceState::ToFade:			return "ToFade";
		default: break;
		}

		// Fail safe
		return GetPerformanceStateName( SceneTitle::PerformanceState::NotPerforming );
	}
	constexpr const char *GetChoiceItemName( SceneTitle::Choice item )
	{
		switch ( item )
		{
		case SceneTitle::Choice::Start:		return "START";
		case SceneTitle::Choice::Option:	return "OPTION";
		default: break;
		}

		// Fail safe
		return GetChoiceItemName( SceneTitle::Choice::Option );
	}

	struct Member
	{
		Donya::Model::Constants::PerScene::DirectionalLight directionalLight;

		float scrollTakeSecond	= 1.0f;	// The second required for scrolling
		
		struct ShadowMap
		{
			Donya::Vector3	color;			// RGB
			float			bias = 0.03f;	// Ease an acne

			float			offsetDistance	= 10.0f;	// From the player position
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

		RenderingHelper::VoxelizeConstant voxelize;

		BloomApplier::Parameter bloomParam;

		std::vector<Donya::Vector2> itemPositions; // size() == SceneTitle::Choice::ItemCount
		float chooseItemMagni = 1.5f;

		struct Camera
		{
			bool				coordIsRelative		= true; // By player position
			Donya::Vector3		pos		{ 0.0f, 0.0f,-1.0f };
			Donya::Vector3		focus	{ 0.0f, 0.0f, 0.0f };
			float				fovDegree			= 30.0f;
			float				lerpSecFromOther	= 1.0f; // Taking second for transition from other state
			Donya::Easing::Kind	easeKind			= Donya::Easing::Kind::Linear;
			Donya::Easing::Type	easeType			= Donya::Easing::Type::In;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( coordIsRelative		),
					CEREAL_NVP( pos					),
					CEREAL_NVP( focus				),
					CEREAL_NVP( fovDegree			),
					CEREAL_NVP( lerpSecFromOther	)
				);

				if ( 1 <= version )
				{
					archive
					(
						CEREAL_NVP( easeKind ),
						CEREAL_NVP( easeType )
					);
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		std::vector<Camera> cameras; // size() == SceneTitle::StateCount

		Boss::InitializeParam bossIniter;

		float leaveDelaySec			= 1.0f;
		float fadeDelaySec			= 1.0f;
		float fadeBGMSec			= 1.0f;
		float resetCameraWaitSec	= 1.0f;

		float			logoScale = 1.0f;
		Donya::Vector2	logoPos{ 800.0f, 450.0f };

		float						pressedItemAddDestScale			= 1.0f;
		float						pressedItemAddFadeSecond		= 1.0f;
		Donya::Easing::Kind			pressedItemAddFadeEaseKind		= Donya::Easing::Kind::Linear;
		Donya::Easing::Type			pressedItemAddFadeEaseType		= Donya::Easing::Type::In;
		float						pressedItemScalingDelaySecond	= 0.0f;
		float						pressedItemScalingTakeSecond	= 1.0f;
		std::vector<Donya::Vector2>	pressedItemScalingPoints; // With bezier-curve
		float						pressedLogoScalingDelaySecond	= 0.0f;
		float						pressedLogoScalingTakeSecond	= 1.0f;
		std::vector<Donya::Vector2>	pressedLogoScalingPoints; // With bezier-curve

		float			leaveBossDelaySec = 1.0f;
		Donya::Vector3	leaveBossOffset{}; // World space
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
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
				archive( CEREAL_NVP( cameras ) );
			}
			if ( 3 <= version )
			{
				archive( CEREAL_NVP( bossIniter ) );
			}
			if ( 4 <= version )
			{
				archive
				(
					CEREAL_NVP( leaveDelaySec	),
					CEREAL_NVP( fadeDelaySec	)
				);
			}
			if ( 5 <= version )
			{
				archive( CEREAL_NVP( fadeBGMSec ) );
			}
			if ( 6 <= version )
			{
				archive( CEREAL_NVP( resetCameraWaitSec ) );
			}
			if ( 7 <= version )
			{
				archive
				(
					CEREAL_NVP( logoScale	),
					CEREAL_NVP( logoPos		)
				);
			}
			if ( 8 <= version )
			{
				archive
				(
					CEREAL_NVP( pressedItemScalingDelaySecond	),
					CEREAL_NVP( pressedItemScalingTakeSecond	),
					CEREAL_NVP( pressedItemScalingPoints		),
					CEREAL_NVP( pressedLogoScalingDelaySecond	),
					CEREAL_NVP( pressedLogoScalingTakeSecond	),
					CEREAL_NVP( pressedLogoScalingPoints		)
				);
			}
			if ( 9 <= version )
			{
				archive
				(
					CEREAL_NVP( pressedItemAddDestScale		),
					CEREAL_NVP( pressedItemAddFadeSecond	),
					CEREAL_NVP( pressedItemAddFadeEaseKind	),
					CEREAL_NVP( pressedItemAddFadeEaseType	)
				);
			}
			if ( 10 <= version )
			{
				archive
				(
					CEREAL_NVP( leaveBossDelaySec	),
					CEREAL_NVP( leaveBossOffset		)
				);
			}
			if ( 11 <= version )
			{
				archive( CEREAL_NVP( voxelize ) );
			}
			if ( 12 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode()
		{
			if ( cameras.size() != SceneTitle::cameraStateCount )
			{
				cameras.resize( SceneTitle::cameraStateCount );
			}
			if ( ImGui::TreeNode( u8"カメラ関連" ) )
			{
				auto Show = []( const char *nodeCaption, Camera *p )
				{
					if ( !ImGui::TreeNode( nodeCaption ) ) { return; }
					// else

					ImGui::Checkbox		( u8"自機からの相対座標にする", &p->coordIsRelative );
					ImGui::DragFloat3	( u8"カメラ座標",	&p->pos.x,				0.01f );
					ImGui::DragFloat3	( u8"注視点座標",	&p->focus.x,			0.01f );
					ImGui::SliderFloat	( u8"FOV(degree)",	&p->fovDegree,			0.01f, 90.0f );
					ImGui::DragFloat	( u8"遷移に使う秒数",	&p->lerpSecFromOther,	0.01f );
					ImGui::Helper::ShowEaseParam( u8"遷移に使うイージング", &p->easeKind, &p->easeType );

					ImGui::TreePop();
				};
				for ( int i = 0; i < SceneTitle::cameraStateCount; ++i )
				{
					const auto status = scast<SceneTitle::CameraState>( i );
					Show( GetCameraStateName( status ), &cameras[i] );
				}

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
				shadowMap.offsetDistance	= std::max( 0.01f, shadowMap.offsetDistance		);
				shadowMap.projectDistance.x	= std::max( 0.01f, shadowMap.projectDistance.x	);
				shadowMap.projectDistance.y	= std::max( 0.01f, shadowMap.projectDistance.y	);
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

			voxelize.ShowImGuiNode( u8"ボクセライズ関連" );

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

			if ( ImGui::TreeNode( u8"タイトルロゴ" ) )
			{
				ImGui::DragFloat ( u8"スケール",			&logoScale, 0.01f );
				ImGui::DragFloat2( u8"スクリーン座標",	&logoPos.x );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"演出関連" ) )
			{
				bossIniter.ShowImGuiNode( u8"ボスの初期化設定" );

				if ( ImGui::TreeNode( u8"秒数関連" ) )
				{
					ImGui::DragFloat ( u8"スクロールに要する秒数",		&scrollTakeSecond,		0.01f );
					ImGui::DragFloat ( u8"自機退場までの遅延秒数",		&leaveDelaySec,			0.01f );
					ImGui::DragFloat ( u8"ボス退場までの遅延秒数",		&leaveBossDelaySec,		0.01f );
					ImGui::DragFloat3( u8"ボス退場時の目標座標（相対）",	&leaveBossOffset.x,		0.01f );
					ImGui::DragFloat ( u8"フェードまでの遅延秒数",		&fadeDelaySec,			0.01f );
					ImGui::DragFloat ( u8"BGMのフェードにかける秒数",		&fadeBGMSec,			0.01f );
					ImGui::DragFloat ( u8"無操作でカメラを戻すまでの秒数",	&resetCameraWaitSec,	0.01f );
					scrollTakeSecond	= std::max( 0.01f,	scrollTakeSecond	);
					leaveDelaySec		= std::max( 0.0f,	leaveDelaySec		);
					leaveBossDelaySec	= std::max( 0.0f,	leaveBossDelaySec	);
					fadeDelaySec		= std::max( 0.0f,	fadeDelaySec		);
					fadeBGMSec			= std::max( 0.0f,	fadeBGMSec			);
					resetCameraWaitSec	= std::max( 0.01f, resetCameraWaitSec	);

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"レスポンス関連" ) )
			{
				constexpr Donya::Vector2 scaleRange { -5.0f, 5.0f };
				constexpr Donya::Vector2 sliderSize { 32.0f, 128.0f };
				constexpr Donya::Vector2 appendScale{ 1.0f, 1.0f };

				auto Show = [&appendScale]( const char *nodeCaption, const Donya::Vector2 &rangeMinMax, float *pDelaySecond, float *pTakeSecond, std::vector<Donya::Vector2> *pCtrlPoints )
				{
					if ( !ImGui::TreeNode( nodeCaption ) ) { return; }
					// else

					ImGui::DragFloat( u8"開始の遅延秒数", pDelaySecond, 0.01f );
					*pDelaySecond = std::max( 0.0f, *pDelaySecond );
					ImGui::DragFloat( u8"必要秒数", pTakeSecond, 0.01f );
					*pTakeSecond = std::max( 0.001f, *pTakeSecond );

					ImGui::Helper::ShowBezier2DNode( u8"ベジェ曲線の制御点", pCtrlPoints, rangeMinMax.x, rangeMinMax.y );

					/*
					ImGui::Helper::ResizeByButton( pCtrlPoints, appendScale );
					if ( pCtrlPoints->size() < 2 )
					{
						pCtrlPoints->resize( 2 );
					}

					const int pointCount = pCtrlPoints->size();
					std::string caption;
					for ( int i = 0; i < pointCount; ++i )
					{
						caption = Donya::MakeArraySuffix( i );
						ImGui::SliderFloat2( caption.c_str(), &pCtrlPoints->at( i ).x, rangeMinMax.x, rangeMinMax.y );
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

					Donya::Vector2 result = Math::CalcBezierCurve( *pCtrlPoints, timer );
					ImGui::SliderFloat2( u8"ベジェ曲線適用結果", &result.x, rangeMinMax.x, rangeMinMax.y );
					*/

					ImGui::TreePop();
				};

				Show( u8"ロゴ", scaleRange, &pressedLogoScalingDelaySecond, &pressedLogoScalingTakeSecond, &pressedLogoScalingPoints );
				Show( u8"項目", scaleRange, &pressedItemScalingDelaySecond, &pressedItemScalingTakeSecond, &pressedItemScalingPoints );

				ImGui::Helper::ShowEaseParam( u8"加算項目のフェード・イージング設定", &pressedItemAddFadeEaseKind, &pressedItemAddFadeEaseType );
				ImGui::DragFloat( u8"加算項目のフェード・終点スケール",	&pressedItemAddDestScale,  0.01f );
				ImGui::DragFloat( u8"加算項目のフェード・秒数",			&pressedItemAddFadeSecond, 0.01f );
				pressedItemAddFadeSecond = std::max( 0.01f, pressedItemAddFadeSecond );

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

	Member::Camera FetchCameraOrDefault( SceneTitle::CameraState key )
	{
		const auto &data	= FetchParameter().cameras;

		const int intKey	= scast<int>( key );
		const int size		= scast<int>( data.size() );
		return ( intKey < size ) ? data[intKey] : Member::Camera{};
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
CEREAL_CLASS_VERSION( Member,				11 )
CEREAL_CLASS_VERSION( Member::ShadowMap,	0  )
CEREAL_CLASS_VERSION( Member::Camera,		1  )

void SceneTitle::Init()
{
	sceneParam.LoadParameter();
	const auto &data = FetchParameter();

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

		pSky = std::make_unique<Sky>();
		result = pSky->Init();
		assert( result );
		pSky->AdvanceHourTo( 0.0f, 0.0f );

		playerIniter.LoadParameter( stageNo );

		using Spr = SpriteAttribute;
		result = sprTitleLogo.LoadSprite( GetSpritePath( Spr::TitleLogo ), GetSpriteInstanceCount( Spr::TitleLogo ) );
		assert( result );
	}

	{
		/*
		The dependencies of initializations:
		Enemies:		[CurrentScreen, Player]	- depends on the player position as target, and current screen for decide the initial state
		CurrentScreen:	[Camera]				- depends on the view and projection matrix of the camera
		Camera:			[Player, House]			- depends on the player position(camera), and room position(light camera)
		CurrentRoomID	[Player, House]			- the current room indicates what the player belongs map
		House - it is free
		Player:			[Map]					- depends on the map because decide the ground state
		Map - it is free
		*/

		// Initialize a dependent objects

		pHouse = std::make_unique<House>();
		pHouse->Init( stageNo );

		pMap = std::make_unique<Map>();
		pMap->Init( stageNo, /* reloadModel = */ true );

		Player::Remaining::Set( Player::Parameter().Get().initialRemainCount );
		PlayerInit( *pMap );

		currentRoomID = CalcCurrentRoomID();

		CameraInit();
		currentScreen = CalcCurrentScreenPlane();

		pBoss = std::make_unique<Boss::Skull>();
		pBoss->Init( data.bossIniter, currentRoomID, /* withAppearPerformance = */ false );

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

	auto &effectAdmin = Effect::Admin::Get();
	effectAdmin.ClearInstances();
	effectAdmin.SetLightColorAmbient( Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } );
	effectAdmin.SetLightColorDiffuse( Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } );
	effectAdmin.SetLightDirection	( data.directionalLight.direction.XYZ() );

	Donya::Sound::Play( Music::BGM_Title );
}
void SceneTitle::Uninit()
{
	if ( pMap		) { pMap->Uninit();		}
	if ( pHouse		) { pHouse->Uninit();	}
	if ( pPlayer	) { pPlayer->Uninit();	}
	pMap.reset();
	pSky.reset();
	pHouse.reset();
	pPlayer.reset();

	Bullet::Admin::Get().ClearInstances();
	Enemy::Admin::Get().ClearInstances();
	Item::Admin::Get().ClearInstances();

	Donya::Sound::Stop( Music::BGM_Title );
}

Scene::Result SceneTitle::Update( float elapsedTime )
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F2 ) )
	{
		Donya::Sound::Play( Music::DEBUG_Strong );
		StartFade();
	}
	if ( Donya::Keyboard::Trigger( VK_F5 ) )
	{
		nowDebugMode = !nowDebugMode;

		if ( nowDebugMode )
		{
			for ( auto &it : stateCameras )
			{
				it.ChangeMode( Donya::ICamera::Mode::Satellite );
			}
		}
		else
		{
			for ( auto &it : stateCameras )
			{
				it.SetOrientation( Donya::Quaternion::Identity() );
			}
			CameraInit();
		}
	}
#endif // DEBUG_MODE

#if USE_IMGUI
	UseImGui();

	// Apply for be able to see an adjustment immediately
	{
		const auto &data = FetchParameter();
		if ( pBloomer ) { pBloomer->AssignParameter( data.bloomParam ); }

		auto &effectAdmin = Effect::Admin::Get();
		effectAdmin.SetLightColorAmbient( Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } );
		effectAdmin.SetLightColorDiffuse( Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } );
		effectAdmin.SetLightDirection	( data.directionalLight.direction.XYZ() );
	}
#endif // USE_IMGUI

	PointLightStorage::Get().Clear();

	UpdateInput();

	if ( transCameraTime < 1.0f )
	{
		const float takeSecond = FetchCameraOrDefault( currCameraStatus ).lerpSecFromOther;
		const float advance = 1.0f / takeSecond;
		transCameraTime += advance * elapsedTime;
		if ( 1.0f <= transCameraTime )
		{
			beforeCameraStatus = currCameraStatus;
			transCameraTime = 1.0f;
		}
	}
	
	UpdateChooseItem();

	elapsedSecond += elapsedTime;
	if ( wasDecided ) { afterDecidedTimer += elapsedTime; }

	UpdatePerformance( elapsedTime );

	const float deltaTimeForMove = elapsedTime;

	if ( pSky ) { pSky->Update( elapsedTime ); }

	if ( pMap ) { pMap->Update( deltaTimeForMove ); }
	const Map  emptyMap{}; // Used for empty argument. Fali safe.
	const Map  &mapRef = ( pMap ) ? *pMap : emptyMap;

	currentRoomID = CalcCurrentRoomID();
	const Room *pCurrentRoom = pHouse->FindRoomOrNullptr( currentRoomID );
	
	PlayerUpdate( deltaTimeForMove, mapRef );
	const Donya::Vector3 playerPos = GetPlayerPosition();

	BossUpdate( deltaTimeForMove, playerPos );

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
		}

		if ( pBoss ) { pBoss->PhysicUpdate( deltaTimeForMove, mapRef ); }
		Bullet::Admin::Get().PhysicUpdate( deltaTimeForMove, mapRef );
		Enemy::Admin::Get().PhysicUpdate( deltaTimeForMove, mapRef );
		Item::Admin::Get().PhysicUpdate( deltaTimeForMove, mapRef );
	}

	// CameraUpdate() depends the currentScreen, so I should update that before CameraUpdate().
	currentScreen = CalcCurrentScreenPlane();
	CameraUpdate( elapsedTime );

	const auto &currCamera = GetCurrentCamera( currCameraStatus );
	Effect::Admin::Get().SetViewMatrix( currCamera.CalcViewMatrix() );
	Effect::Admin::Get().SetProjectionMatrix( currCamera.GetProjectionMatrix() );

	return ReturnResult();
}

namespace
{
	enum class DrawTarget
	{
		Map		= 1 << 0,
		Bullet	= 1 << 1,
		Player	= 1 << 2,
		Boss	= 1 << 3,
		Enemy	= 1 << 4,
		Item	= 1 << 5,

		All		= Map | Bullet | Player | Boss | Enemy | Item
	};
	DEFINE_ENUM_FLAG_OPERATORS( DrawTarget )
}
void SceneTitle::Draw( float elapsedTime )
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

		if ( Drawable( Kind::Map ) && pMap ) { pMap->Draw( pRenderer.get() ); }

		( castShadow )
		? pRenderer->DeactivateShaderShadowStatic()
		: pRenderer->DeactivateShaderNormalStatic();


		( castShadow )
		? pRenderer->ActivateShaderShadowSkinning()
		: pRenderer->ActivateShaderNormalSkinning();

		if ( Drawable( Kind::Player	) && pPlayer	) { pPlayer->Draw( pRenderer.get() );	}
		if ( Drawable( Kind::Boss	) && pBoss		) { pBoss->Draw( pRenderer.get() );		}
		if ( Drawable( Kind::Enemy	) ) { Enemy::Admin::Get().Draw( pRenderer.get() );		}
		if ( Drawable( Kind::Item	) ) { Item::Admin::Get().Draw( pRenderer.get() );		}
		if ( Drawable( Kind::Bullet	) ) { Bullet::Admin::Get().Draw( pRenderer.get() );		}

		( castShadow )
		? pRenderer->DeactivateShaderShadowSkinning()
		: pRenderer->DeactivateShaderNormalSkinning();
	};

	Donya::Vector4   cameraPos{};
	Donya::Vector4x4 V{};
	Donya::Vector4x4 VP{};
	if ( 1.0f <= transCameraTime ) // Lerp is not needed
	{
		const auto &camera = GetCurrentCamera( currCameraStatus );
		cameraPos = Donya::Vector4{ camera.GetPosition(), 1.0f };
		V  = camera.CalcViewMatrix();
		VP = V * camera.GetProjectionMatrix();
	}
	else
	{
		const auto &old  = GetCurrentCamera( beforeCameraStatus		);
		const auto &curr = GetCurrentCamera( currCameraStatus	);
		const auto data  = FetchCameraOrDefault( currCameraStatus );

		const float lerpFactor = Donya::Easing::Ease( data.easeKind, data.easeType, transCameraTime );

		// I prefer lerp-ed view matrix than slerp-ed view matrix
		const auto oldV  = old.CalcViewMatrix();
		const auto currV = curr.CalcViewMatrix();
		for ( int r = 0; r < 4; ++r )
		{
			for ( int c = 0; c < 3; ++c )
			{
				V.m[r][c] = Donya::Lerp( oldV.m[r][c], currV.m[r][c], lerpFactor );
			}
		}
		const auto invV = V.Inverse();
		cameraPos.x = invV._41;
		cameraPos.y = invV._42;
		cameraPos.z = invV._43;

		const float lerpedFOV	= Donya::Lerp( old.GetFOV(), curr.GetFOV(), lerpFactor );
		// These are same between old and current
		const auto screenSize	= curr.GetScreenSize();
		const auto zNear		= curr.GetZNear();
		const auto zFar			= curr.GetZFar();
		const auto P			= Donya::Vector4x4::MakePerspectiveFovLH( lerpedFOV, screenSize.x / screenSize.y, zNear, zFar );
		VP = V * P;

		Effect::Admin::Get().SetViewMatrix( V );
		Effect::Admin::Get().SetProjectionMatrix( P );
	}

	const Donya::Vector4   lightPos = Donya::Vector4{ lightCamera.GetPosition(), 1.0f };
	const Donya::Vector4x4 LV  = CalcLightViewMatrix();
	const Donya::Vector4x4 LVP = LV * lightCamera.GetProjectionMatrix();
	const auto &data = FetchParameter();

	// Draw the back-ground
	if ( pSky )
	{
		pScreenSurface->SetRenderTarget();
		pScreenSurface->SetViewport();

		pSky->Draw( cameraPos.XYZ(), VP );

		Donya::Surface::ResetRenderTarget();
	}

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
		// Update voxelize constant
		{
			pRenderer->UpdateConstant( data.voxelize );
		}
		// Update point light constant
		{
			pRenderer->UpdateConstant( PointLightStorage::Get().GetStorage() );
		}

		pRenderer->ActivateConstantScene();
		pRenderer->ActivateConstantPointLight();
		pRenderer->ActivateConstantShadow();
		pRenderer->ActivateConstantVoxelize();
		pRenderer->ActivateSamplerShadow( Donya::Sampler::Defined::Point_Border_White );
		pRenderer->ActivateShadowMap( *pShadowMap );

		constexpr DrawTarget option = DrawTarget::All ^ DrawTarget::Bullet ^ DrawTarget::Item;
		DrawObjects( option, /* castShadow = */ false );

		// Disable shadow
		{
			pRenderer->DeactivateConstantShadow();
			shadowConstant.shadowBias = 1.0f; // Make the pixel to nearest
			pRenderer->UpdateConstant( shadowConstant );
			pRenderer->ActivateConstantShadow();
		}

		DrawObjects( DrawTarget::Item, /* castShadow = */ false );

		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::NoTest_Write );
		DrawObjects( DrawTarget::Bullet, /* castShadow = */ false );
		Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::Write_PassLess );

		pRenderer->DeactivateShadowMap( *pShadowMap );
		pRenderer->DeactivateSamplerShadow();
		pRenderer->DeactivateConstantVoxelize();
		pRenderer->DeactivateConstantShadow();
		pRenderer->DeactivateConstantPointLight();
		pRenderer->DeactivateConstantScene();
	}
	
	pRenderer->DeactivateSamplerNormal();
	pRenderer->DeactivateSamplerModel();
	Donya::Rasterizer::Deactivate();
	Donya::DepthStencil::Deactivate();

	// Draw logo
	{
		const float &delaySec = data.pressedLogoScalingDelaySecond;
		const float &takeSec  = data.pressedLogoScalingTakeSecond;
		float currBezierTime  = ( IsZero( takeSec ) ) ? 1.0f : ( afterDecidedTimer - delaySec ) / takeSec;
		currBezierTime = Donya::Clamp( currBezierTime, 0.0f, 1.0f );

		const Donya::Vector2 pressedScale =
			( data.pressedLogoScalingPoints.size() < 2 )
			? Donya::Vector2{ 1.0f, 1.0f }
			: Math::CalcBezierCurve( data.pressedLogoScalingPoints, currBezierTime );

		sprTitleLogo.pos	= data.logoPos;
		sprTitleLogo.scale	= data.logoScale * pressedScale;
		sprTitleLogo.alpha	= 1.0f;
		sprTitleLogo.origin	= { 0.5f, 0.5f };
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

	// Draw fonts
	{
		const auto pFontRenderer = FontHelper::GetRendererOrNullptr( FontAttribute::Main );
		if ( pFontRenderer )
		{
			pScreenSurface->SetRenderTarget();
			pScreenSurface->SetViewport();

			constexpr Donya::Vector2 pivot			{ 0.5f, 0.5f };
			constexpr Donya::Vector4 selectColor	{ 1.0f, 1.0f, 1.0f, 1.0f };
			constexpr Donya::Vector4 unselectColor	{ 0.4f, 0.4f, 0.4f, 1.0f };

			const float &delaySec = data.pressedItemScalingDelaySecond;
			const float &takeSec  = data.pressedItemScalingTakeSecond;
			const float currBezierTime =
				( IsZero( takeSec ) )
				? 1.0f
				: Donya::Clamp( ( afterDecidedTimer - delaySec ) / takeSec, 0.0f, 1.0f );

			const Donya::Vector2 pressedScale =
				( data.pressedItemScalingPoints.size() < 2 )
				? Donya::Vector2{ 1.0f, 1.0f }
				: Math::CalcBezierCurve( data.pressedItemScalingPoints, currBezierTime );

			const Donya::Vector2 selectScale   = data.chooseItemMagni	* pressedScale;
			const Donya::Vector2 unselectScale = 1.0f					* pressedScale;

			const int positionCount = scast<int>( data.itemPositions.size() );
			auto DrawImpl	= [&]( const wchar_t *itemName, Choice item, const Donya::Vector2 &scale, const Donya::Vector4 &color )
			{
				const int itemIndex = scast<int>( item );
				const Donya::Vector2 pos =
					( positionCount <= itemIndex )
					? Donya::Vector2::Zero()
					: data.itemPositions[itemIndex];

				pFontRenderer->DrawExt( itemName, pos, pivot, scale, color );
			};
			auto Draw		= [&]( const wchar_t *itemName, Choice item, bool selected )
			{
				const Donya::Vector2 &scale =
					( selected )
					? selectScale
					: unselectScale;

				const Donya::Vector4 &color =
					( selected )
					? selectColor
					: unselectColor;

				DrawImpl( itemName, item, scale, color );
			};

			Donya::Sprite::SetDrawDepth( 0.0f );
			Draw( L"ＳＴＡＲＴ",		Choice::Start,	( chooseItem == Choice::Start  ) );
			// Draw( L"ＯＰＴＩＯＮ",	Choice::Option,	( chooseItem == Choice::Option ) );
			Donya::Sprite::Flush();

			Donya::Blend::Activate( Donya::Blend::Mode::ADD_NO_ATC );
			if ( chooseItem == Choice::Start )
			{
				const float &fadeSec = data.pressedItemAddFadeSecond;
				const float currFadeTime =
					( IsZero( fadeSec ) )
					? 1.0f
					: Donya::Clamp( afterDecidedTimer / fadeSec, 0.0f, 1.0f );
				const float easeFactor = Donya::Easing::Ease( data.pressedItemAddFadeEaseKind, data.pressedItemAddFadeEaseType, currFadeTime );
				
				const float additionScale = data.pressedItemAddDestScale * easeFactor + data.chooseItemMagni;

				const float additionAlpha = Donya::Clamp( 1.0f - easeFactor, 0.0f, 1.0f );
				const Donya::Vector4 additionColor = Donya::Vector4{ selectColor.XYZ(), additionAlpha };
				DrawImpl( L"ＳＴＡＲＴ", Choice::Start, additionScale, additionColor );
			}

			Donya::Sprite::Flush();
			Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );

			Donya::Surface::ResetRenderTarget();
		}
	}

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

		Donya::Sprite::SetDrawDepth( 1.0f );
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
		if ( pBoss		) { pBoss->DrawHitBox( pRenderer.get(), VP );					}
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

void SceneTitle::UpdateInput()
{
	controller.Update();

	static const Donya::Vector2 deadZone
	{
		Donya::XInput::GetDeadZoneLeftStick(),
		Donya::XInput::GetDeadZoneLeftStick()
	};
	previousInput = currentInput;
	currentInput  = Input::MakeCurrentInput( controller, deadZone );
}

void SceneTitle::UpdateChooseItem()
{
	if ( wasDecided ) { return; }
	// else

	auto Tilted = []( float value, int sign )
	{
		return Donya::SignBit( value ) == sign;
	};

	const auto &curr = currentInput;
	const auto &prev = previousInput;
	const bool trgLeft		= Tilted( curr.moveVelocity.x, -1 ) && !Tilted( prev.moveVelocity.x, -1 );
	const bool trgRight		= Tilted( curr.moveVelocity.x, +1 ) && !Tilted( prev.moveVelocity.x, +1 );
	const bool trgUp		= Tilted( curr.moveVelocity.y, +1 ) && !Tilted( prev.moveVelocity.y, +1 );
	const bool trgDown		= Tilted( curr.moveVelocity.y, -1 ) && !Tilted( prev.moveVelocity.y, -1 );
	const bool trgDecide	= Input::HasTrue( curr.useShots ) && !Input::HasTrue( prev.useShots );

#if 1 // CAN_NOT_SELECT_OPTION
	const bool willChoice = ( trgLeft || trgRight || trgUp || trgDown || trgDecide );
	if ( willChoice && chooseItem != Choice::Start )
	{
		chooseItem = Choice::Start;
	}
#else
	// TODO: Play choice SE

	// If do not selected
	if ( chooseItem == Choice::ItemCount )
	{
		if ( trgUp		) { chooseItem = Choice::Start;  }
		else
		if ( trgDown	) { chooseItem = Choice::Option; }
		else
		if ( trgLeft || trgRight ) { chooseItem = Choice::Start; }

		return;
	}
	// else

	if ( trgUp		) { chooseItem = Choice::Start;  }
	else
	if ( trgDown	) { chooseItem = Choice::Option; }
#endif // CAN_NOT_SELECT_OPTION

	const bool chooseSelectableItem = ( chooseItem == Choice::Start ) ? true : false;
	if ( trgDecide && chooseSelectableItem )
	{
		wasDecided = trgDecide;
		Donya::Sound::Play( Music::UI_Decide );
	}
}

void SceneTitle::UpdatePerformance( float elapsedTime )
{
	if ( performanceStatus == PerformanceState::NotPerforming )
	{
		if ( wasDecided )
		{
			if ( pPlayer ) { pPlayer->ChargeFully(); }
			ChangeCameraState( CameraState::StartPerformance );
			Donya::Sound::AppendFadePoint( Music::BGM_Title, FetchParameter().fadeBGMSec, 0.0f );

			performTimer = 0.0f;
			performanceStatus = PerformanceState::ToLeave;
		}

		return;
	}
	// else

	if ( performanceStatus == PerformanceState::Wait ) { return; }
	// else

	// Skip performance
	if ( Input::HasButtonInput( currentInput ) && !Input::HasButtonInput( previousInput ) )
	{
		bool skipable = true;
		if ( pBoss )
		{
			if ( Donya::Collision::IsHit( pBoss->GetHurtBox(), currentScreen ) )
			{
				skipable = false;
			}
		}

		if ( skipable )
		{
			StartFade();
		}
	}

	performTimer += elapsedTime;

	const auto &data = FetchParameter();
	switch ( performanceStatus )
	{
	case PerformanceState::ToLeave:
		if ( data.leaveDelaySec <= performTimer )
		{
			if ( pPlayer ) { pPlayer->PerformLeaving(); }

			performTimer = 0.0f;
			performanceStatus = PerformanceState::ToFade;
		}
		return;
	case PerformanceState::ToFade:
		if ( data.fadeDelaySec <= performTimer )
		{
			StartFade();

			performTimer = 0.0f;
			performanceStatus = PerformanceState::Wait;
		}
		return;
	default:
		_ASSERT_EXPR( 0, L"Error: Unexpected PerformanceStatus!" );
		return;
	}
}

void SceneTitle::ChangeCameraState( CameraState next )
{
	beforeCameraStatus		= currCameraStatus;
	currCameraStatus	= next;
	transCameraTime	= 0.0f;
}

Donya::Vector4x4 SceneTitle::MakeScreenTransform() const
{
	const auto &currCamera = GetCurrentCamera( currCameraStatus );
	const Donya::Vector4x4 matViewProj = currCamera.CalcViewMatrix() * currCamera.GetProjectionMatrix();
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

	constexpr Donya::Vector2 screenSize{ Common::ScreenWidthF(), Common::ScreenHeightF() };
	constexpr Donya::Vector2 defaultZRange{ 0.1f, 500.0f };
	
	Member::Camera cameraData;
	for ( int i = 0; i < cameraStateCount; ++i )
	{
		cameraData = FetchCameraOrDefault( scast<CameraState>( i ) );

		auto &it = stateCameras[i];
		it.Init				( Donya::ICamera::Mode::Look );
		it.SetZRange		( defaultZRange.x, defaultZRange.y );
		it.SetFOV			( ToRadian( cameraData.fovDegree ) );
		it.SetScreenSize	( screenSize );

		it.SetProjectionPerspective();
	}

	lightCamera.Init			( Donya::ICamera::Mode::Look );
	lightCamera.SetZRange		( data.shadowMap.nearDistance, data.shadowMap.projectDistance.z );
	lightCamera.SetScreenSize	( data.shadowMap.projectDistance.XY() );
	lightCamera.SetProjectionOrthographic();

	AssignCameraPos();

	// I can setting a configuration,
	// but current data is not changed immediately.
	// So update here.
	Donya::ICamera::Controller moveInitPoint{};
	moveInitPoint.SetNoOperation();
	moveInitPoint.slerpPercent = 1.0f;

	for ( auto &it : stateCameras )
	{ it.Update( moveInitPoint ); }
	lightCamera.Update( moveInitPoint );
}
void SceneTitle::AssignCameraPos()
{
	const auto &data = FetchParameter();
	const Donya::Vector3 playerPos = GetPlayerPosition();

	Member::Camera cameraData;
	Donya::Vector3 basePos;
	for ( int i = 0; i < cameraStateCount; ++i )
	{
		cameraData = FetchCameraOrDefault( scast<CameraState>( i ) );

		if ( cameraData.coordIsRelative )
		{
			basePos = playerPos;
		}
		else
		{
			basePos = Donya::Vector3::Zero();
		}

		auto &it = stateCameras[i];
		it.SetPosition	( basePos + cameraData.pos		);
		it.SetFocusPoint( basePos + cameraData.focus	);
	}

	const Donya::Vector3 offset = -data.shadowMap.projectDirection * data.shadowMap.offsetDistance;
	lightCamera.SetPosition  ( playerPos + offset );
	lightCamera.SetFocusPoint( playerPos );
}
void SceneTitle::CameraUpdate( float elapsedTime )
{
	const auto &data = FetchParameter();

#if USE_IMGUI
	// Apply for be able to see an adjustment immediately
	{
		Member::Camera cameraData;
		for ( int i = 0; i < cameraStateCount; ++i )
		{
			cameraData = FetchCameraOrDefault( scast<CameraState>( i ) );

			auto &it = stateCameras[i];
			it.SetFOV( ToRadian( cameraData.fovDegree ) );
			it.SetProjectionPerspective();
		}

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
	for ( auto &it : stateCameras )
	{ it.Update( input ); }
#else
	if ( !nowDebugMode )
	{
		AssignCameraPos();
		for ( auto &it : stateCameras )
		{ it.Update( input ); }

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
		for ( auto &it : stateCameras )
		{ it.Update( input ); }

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

	for ( auto &it : stateCameras )
	{ it.Update( input ); }
	
	input.SetNoOperation();

#endif // !DEBUG_MODE
}
const Donya::ICamera &SceneTitle::GetCurrentCamera( CameraState key ) const
{
	const int intKey = scast<int>( key );
	if ( intKey < 0 || cameraStateCount <= intKey )
	{
		_ASSERT_EXPR( 0, L"Error: Unexpected CameraState!" );
		return stateCameras.front();
	}
	// e;se
	
	return stateCameras[intKey];
}

Donya::Vector4x4 SceneTitle::CalcLightViewMatrix() const
{
	return lightCamera.CalcViewMatrix();
}

void SceneTitle::PlayerInit( const Map &terrain )
{
	if ( pPlayer )
	{
		pPlayer->Uninit();
		pPlayer.reset();
	}

	pPlayer = std::make_unique<Player>();
	pPlayer->Init( playerIniter, terrain, /* withAppearPerformance = */ false );
}
void SceneTitle::PlayerUpdate( float elapsedTime, const Map &terrain )
{
	if ( !pPlayer ) { return; }
	// else

	const Player::Input input = MakePlayerInput( elapsedTime );

	pPlayer->Update( elapsedTime, input, terrain );
}
Player::Input SceneTitle::MakePlayerInput( float elapsedTime )
{
	Player::Input input{};

	if ( performanceStatus != PerformanceState::NotPerforming )
	{
		// First frame of performancing
		if ( IsZero( performTimer ) && performanceStatus == PerformanceState::ToLeave )
		{
			input.useShots.fill( true );

			// Make to look to left(center direction) side
			const auto front = pPlayer->GetOrientation();
			if ( 0.0f <= front.x )
			{
				input.moveVelocity.x = -1.0f;
			}
		}

		return input;
	}
	// else

	static const Donya::Vector2 deadZone
	{
		Donya::XInput::GetDeadZoneLeftStick(),
		Donya::XInput::GetDeadZoneLeftStick()
	};

	input = currentInput;
	input.shiftGuns.fill( 0 );

	auto ActivateReturning		= [&]()
	{
		returnToAttract = true;
	};
	auto DeactivateReturning	= [&]()
	{
		returnToAttract = false;
		horizDiffSignFromInitialPos = 0;
	};

	if ( currCameraStatus == CameraState::Attract )
	{
		if ( Input::HasButtonInput( input ) || Input::HasStickInput( input ) )
		{
			ChangeCameraState( CameraState::Controllable );
		}

		DeactivateReturning();
		elapsedSecondSinceLastInput = 0.0f;
	}
	else if ( currCameraStatus == CameraState::Controllable )
	{
		if ( Input::HasButtonInput( input ) || Input::HasStickInput( input ) )
		{
			DeactivateReturning();
			elapsedSecondSinceLastInput = 0.0f;
		}
		else
		{
			elapsedSecondSinceLastInput += elapsedTime;
			if ( FetchParameter().resetCameraWaitSec <= elapsedSecondSinceLastInput )
			{
				ActivateReturning();
			}
			else
			{
				DeactivateReturning();
			}
		}
	}

	if ( returnToAttract )
	{
		const auto diff = pPlayer->GetPosition().x - playerIniter.GetWorldInitialPos().x;
		const int  currSign = Donya::SignBit( diff );

		// It condition is the first frame when the returnToAttract is true.
		// I should initialize the sign.
		if ( !horizDiffSignFromInitialPos )
		{
			horizDiffSignFromInitialPos = currSign;
		}

		// Arrive to initial position
		if ( horizDiffSignFromInitialPos != currSign || !currSign )
		{
			DeactivateReturning();

			// Make to look to left(center direction) side
			const auto front = pPlayer->GetOrientation();
			if ( 0.0f <= front.x )
			{
				input.moveVelocity.x = -1.0f;
			}

			ChangeCameraState( CameraState::Attract );
		}
		// Head to initial position
		else
		{
			horizDiffSignFromInitialPos = currSign;
			input.moveVelocity.x = scast<float>( -horizDiffSignFromInitialPos );
		}
	}

	return input;
}
Donya::Vector3 SceneTitle::GetPlayerPosition() const
{
	return ( pPlayer ) ? pPlayer->GetPosition() : playerIniter.GetWorldInitialPos();
}

void SceneTitle::BossUpdate( float elapsedTime, const Donya::Vector3 &targetPos )
{
	if ( !pBoss ) { return; }
	// else

	const auto &data = FetchParameter();
	const bool toLeave = data.leaveBossDelaySec <= afterDecidedTimer;

	Boss::Input input;
	if ( toLeave )
	{
		// Make to jump
		input.wsTargetPos	= pBoss->GetPosition() + data.leaveBossOffset;
		input.pressShot		= true;
	}
	else
	{
		input.wsTargetPos	= targetPos;
		input.dontMove		= true;
	}

	pBoss->Update( elapsedTime, input );

	if ( toLeave )
	{
		const float targetSign = Donya::SignBitF( targetPos.x - pBoss->GetPosition().x );
		pBoss->UpdateOrientation( targetSign );
	}
}

int  SceneTitle::CalcCurrentRoomID() const
{
	if ( !pHouse ) { return currentRoomID; }
	// else

	const int newID =  pHouse->CalcBelongRoomID( GetPlayerPosition() );
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
	if ( Fader::Get().IsExist() ) { return; }
	// else

	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeSecond	= Fader::GetDefaultCloseSecond();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneTitle::ReturnResult()
{
	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
	#if DEBUG_MODE
	#if 1 // USUALLY
		change.sceneType = Scene::Type::Game;
	#else
		change.sceneType = ( Donya::Keyboard::Press( VK_CONTROL ) )
		? Scene::Type::Game
		: Scene::Type::Title;
	#endif // USUALLY
	#else
		change.sceneType = Scene::Type::Game;
	#endif // DEBUG_MODE

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

	bool withinInside = false;
	if ( pBoss )
	{
		const auto hitbox = pBoss->GetHurtBox();
		if ( Donya::Collision::IsHit( hitbox, currentScreen ) )
		{
			withinInside = true;
		}
	}
	ImGui::Checkbox( u8"ボスは画面内におさまっているか", &withinInside );

	if ( ImGui::TreeNode( u8"ステート関連" ) )
	{
		int intCamStatus = scast<int>( currCameraStatus );
		if ( Donya::Keyboard::Trigger( 'R' ) ) { intCamStatus--; }
		if ( Donya::Keyboard::Trigger( 'T' ) ) { intCamStatus++; }
		
		int intPfmStatus = scast<int>( performanceStatus );
		if ( Donya::Keyboard::Trigger( 'F' ) ) { intPfmStatus--; }
		if ( Donya::Keyboard::Trigger( 'G' ) ) { intPfmStatus++; }

		ImGui::Text( u8"[R, T]でカメラステートの前後が，" );
		ImGui::Text( u8"[F, Gで演出ステートの前後が，" );
		ImGui::Text( u8"できます。" );

		if ( ImGui::TreeNode( u8"カメラ" ) )
		{
			ImGui::SliderInt( u8"現在", &intCamStatus, 0, cameraStateCount - 1 );
			intCamStatus = Donya::Clamp( intCamStatus, 0, cameraStateCount - 1 );
			const auto prev = currCameraStatus;
			currCameraStatus = scast<CameraState>( intCamStatus );
			if ( currCameraStatus != prev )
			{
				beforeCameraStatus = prev;
				transCameraTime = 0.0f;
			}
			ImGui::Text( GetCameraStateName( currCameraStatus ) );

			ImGui::SliderFloat( u8"遷移時間", &transCameraTime, 0.0f, 1.0f );
			ImGui::Text( u8"前回：" ); ImGui::SameLine();
			ImGui::Text( GetCameraStateName( beforeCameraStatus ) );
			ImGui::Text( u8"現在：" ); ImGui::SameLine();
			ImGui::Text( GetCameraStateName( currCameraStatus ) );

			const auto &old  = GetCurrentCamera( beforeCameraStatus	);
			const auto &curr = GetCurrentCamera( currCameraStatus	);
			Donya::Vector3		lerpedPos = Donya::Lerp( old.GetPosition(), curr.GetPosition(), transCameraTime );
			Donya::Quaternion	lerpedRot = Donya::Quaternion::Slerp( old.GetOrientation(), curr.GetOrientation(), transCameraTime );
			ImGui::DragFloat3( u8"補間中：座標", &lerpedPos.x );
			ImGui::DragFloat4( u8"補間中：姿勢", &lerpedRot.x );

			ImGui::TreePop();
		}
		
		if ( ImGui::TreeNode( u8"演出" ) )
		{
			ImGui::SliderInt( u8"現在", &intPfmStatus, 0, performanceStateCount - 1 );
			intPfmStatus = Donya::Clamp( intPfmStatus, 0, performanceStateCount - 1 );
			const auto prev = performanceStatus;
			performanceStatus = scast<PerformanceState>( intPfmStatus );
			if ( performanceStatus != prev )
			{
				performTimer = 0.0f;
			}
			ImGui::Text( GetPerformanceStateName( performanceStatus ) );

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

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
			const Donya::Vector3 playerPos = GetPlayerPosition();

			Enemy::Admin::Get().ClearInstances();
			Enemy::Admin::Get().RemakeByCSV( loadedData, playerPos, currentScreen );

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
			playerIniter.RemakeByCSV( loadedData );

			if ( thenSave )
			{
				playerIniter.SaveBin ( stageNo );
				playerIniter.SaveJson( stageNo );
			}

			const Map emptyMap{}; // Used for empty argument. Fali safe.
			const Map &mapRef = ( pMap ) ? *pMap : emptyMap;
			PlayerInit( mapRef );
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

				if ( pMap ) { pMap->LoadModel( readStageNumber ); }

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
				pMap->LoadModel( loadMapNumber );
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

			static CameraState	showState	= CameraState::Attract;
			static bool			showCurrent	= true;
			if ( showCurrent )
			{
				showState = currCameraStatus;
			}
			
			const auto &showCamera = stateCameras[scast<int>( showState )];

			const Donya::Vector3 cameraPos = showCamera.GetPosition();
			ShowVec3( u8"現在位置", cameraPos );
			ImGui::Text( "" );

			const Donya::Vector3 focusPoint = showCamera.GetFocusPoint();
			ShowVec3( u8"注視点位置", focusPoint );
			ImGui::Text( "" );
			ImGui::TreePop();
		}
		ImGui::Text( "" );

		if ( pPlayer ) { pPlayer->ShowImGuiNode( u8"自機の現在" ); }
		Player::UpdateParameter( u8"自機のパラメータ" );
		ImGui::Text( "" );

		if ( pSky ) { pSky->ShowImGuiNode( u8"空の現在" ); }
		ImGui::Text( "" );

		if ( pMap    ) { pMap->ShowImGuiNode( u8"マップの現在", stageNo ); }
		if ( pHouse  ) { pHouse->ShowImGuiNode( u8"部屋の現在", stageNo ); }
		ImGui::Text( "" );

		Bullet::Admin::Get().ShowImGuiNode( u8"弾の現在" );
		Bullet::Parameter::Update( u8"弾のパラメータ" );
		ImGui::Text( "" );

		const Donya::Vector3 playerPos = GetPlayerPosition();
		Enemy::Admin::Get().ShowImGuiNode( u8"敵の現在", stageNo, playerPos, currentScreen );
		Enemy::Parameter::Update( u8"敵のパラメータ" );
		ImGui::Text( "" );

		Item::Admin::Get().ShowImGuiNode( u8"アイテムの現在", stageNo );
		Item::Parameter::Update( u8"アイテムのパラメータ" );
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
	
	ImGui::End();
}
#endif // USE_IMGUI
