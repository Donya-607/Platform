#include "SceneLoad.h"

#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Blend.h"
#include "Donya/Color.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/GeometricPrimitive.h"
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Boss.h"
#include "Bullet.h"
#include "Common.h"
#include "Enemy.h"
#include "Effect/EffectAdmin.h"
#include "Fader.h"
#include "FilePath.h"
#include "Item.h"
#include "Meter.h"
#include "Music.h"
#include "Parameter.h"
#include "Player.h"


namespace
{
	struct Member
	{
		float sprLoadScale				= 1.0f;
		float sprLoadFlushingInterval	= 1.0f;
		float sprLoadFlushingRange		= 1.0f;
		float sprLoadMinAlpha			= 0.0f;
		Donya::Vector2 sprLoadPos{ 960.0f, 540.0f };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( sprLoadScale			),
				CEREAL_NVP( sprLoadFlushingInterval	),
				CEREAL_NVP( sprLoadFlushingRange	),
				CEREAL_NVP( sprLoadMinAlpha			),
				CEREAL_NVP( sprLoadPos				)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode()
		{
			auto Clamp = []( auto *v, const auto &min, const auto &max )
			{
				*v = std::max( min, std::max( min, *v ) );
			};

			if ( ImGui::TreeNode( u8"スプライトの調整" ) )
			{
				if ( ImGui::TreeNode( u8"ロード中" ) )
				{
					ImGui::DragFloat( u8"スケール",			&sprLoadScale,				0.1f );
					ImGui::DragFloat( u8"点滅周期（秒）",		&sprLoadFlushingInterval,	0.1f );
					ImGui::DragFloat( u8"点滅範囲",			&sprLoadFlushingRange,		0.1f );
					ImGui::DragFloat( u8"最低アルファ値",		&sprLoadMinAlpha,			0.1f );
					ImGui::DragFloat2( u8"スクリーン座標",	&sprLoadPos.x );

					Clamp( &sprLoadMinAlpha, 0.0f, sprLoadMinAlpha );

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
		}
	#endif // USE_IMGUI
	};

	static ParamOperator<Member> sceneParam{ "SceneLoad" };
	Member FetchParameter()
	{
		return sceneParam.Get();
	}
}
CEREAL_CLASS_VERSION( Member, 0 )


#if USE_IMGUI
namespace
{
	static bool stopFadeout = false;
}
#endif // USE_IMGUI


void SceneLoad::Init()
{
#if USE_IMGUI
	stopFadeout = false;
#endif // USE_IMGUI

	sceneParam.LoadParameter();
	
	constexpr auto CoInitValue = COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE;
	auto LoadingEffects	= [&CoInitValue]( bool *pFinishFlag, bool *pSucceedFlag, std::mutex *pSucceedMutex )
	{
		if ( !pFinishFlag || !pSucceedFlag ) { assert( !"Error: Flag ptr is null!" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, CoInitValue );
		if ( FAILED( hr ) )
		{
			std::lock_guard<std::mutex> lock( *pSucceedMutex );

			*pFinishFlag  = true;
			*pSucceedFlag = false;
			return;
		}
		// else

		bool succeeded = true;
		constexpr size_t kindCount = scast<size_t>( Effect::Kind::KindCount );
		for ( size_t i = 0; i < kindCount; ++i )
		{
			if ( !Effect::Admin::Get().LoadEffect( scast<Effect::Kind>( i ) ) )
			{
				succeeded = false;
			}
		}
		
		_ASSERT_EXPR( succeeded, L"Failed: Effects load is failed." );

		std::lock_guard<std::mutex> lock( *pSucceedMutex );
		*pFinishFlag  = true;
		*pSucceedFlag = succeeded;

		CoUninitialize();
	};
	auto LoadingModels	= [&CoInitValue]( bool *pFinishFlag, bool *pSucceedFlag, std::mutex *pSucceedMutex )
	{
		if ( !pFinishFlag || !pSucceedFlag ) { assert( !"Error: Flag ptr is null!" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, CoInitValue );
		if ( FAILED( hr ) )
		{
			std::lock_guard<std::mutex> lock( *pSucceedMutex );

			*pFinishFlag  = true;
			*pSucceedFlag = false;
			return;
		}
		// else

		bool succeeded = true;

		if ( !Boss	::LoadResource()		) { succeeded = false; }
		if ( !Bullet::LoadResource()		) { succeeded = false; }
		if ( !Enemy	::LoadResource()		) { succeeded = false; }
		if ( !Item	::LoadResource()		) { succeeded = false; }
		if ( !Meter	::LoadResource()		) { succeeded = false; }
		if ( !Player::LoadResource()		) { succeeded = false; }
		
		_ASSERT_EXPR( succeeded, L"Failed: Models load is failed." );

		std::lock_guard<std::mutex> lock( *pSucceedMutex );
		*pFinishFlag  = true;
		*pSucceedFlag = succeeded;

		CoUninitialize();
	};
	auto LoadingSprites	= [&CoInitValue]( bool *pFinishFlag, bool *pSucceedFlag, std::mutex *pSucceedMutex )
	{
		if ( !pFinishFlag || !pSucceedFlag ) { assert( !"Error: Flag ptr is null!" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, CoInitValue );
		if ( FAILED( hr ) )
		{
			std::lock_guard<std::mutex> lock( *pSucceedMutex );

			*pFinishFlag  = true;
			*pSucceedFlag = false;
			return;
		}
		// else

		using Attr = SpriteAttribute;

		auto MakeTextureCache = []( Attr attr )->bool
		{
			const auto handle = Donya::Sprite::Load( GetSpritePath( attr ), GetSpriteInstanceCount( attr ) );
			return  (  handle == NULL ) ? false : true;
		};

		bool succeeded = true;
		if ( !MakeTextureCache( Attr::TitleLogo ) ) { succeeded = false; }
		
		_ASSERT_EXPR( succeeded, L"Failed: Sprites load is failed." );

		std::lock_guard<std::mutex> lock( *pSucceedMutex );
		*pFinishFlag  = true;
		*pSucceedFlag = succeeded;

		CoUninitialize();
	};
	auto LoadingSounds	= [&CoInitValue]( bool *pFinishFlag, bool *pSucceedFlag, std::mutex *pSucceedMutex )
	{
		if ( !pFinishFlag || !pSucceedFlag ) { assert( !"Error: Flag ptr is null!" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, CoInitValue );
		if ( FAILED( hr ) )
		{
			std::lock_guard<std::mutex> lock( *pSucceedMutex );

			*pFinishFlag  = true;
			*pSucceedFlag = false;
			return;
		}
		// else

		using Music::ID;

		struct Bundle
		{
			ID			id;
			const char	*filePath;
			bool		isEnableLoop;
		public:
			constexpr Bundle( ID id, const char *filePath, bool isEnableLoop )
				: id( id ), filePath( filePath ), isEnableLoop( isEnableLoop ) {}
		};

		constexpr std::array<Bundle, ID::MUSIC_COUNT> bundles
		{
			// ID, FilePath, isEnableLoop

			Bundle{ ID::BGM_Title,					"./Data/Sounds/BGM/Title.ogg",						true	},
			Bundle{ ID::BGM_Game,					"./Data/Sounds/BGM/Game.ogg",						true	},
			Bundle{ ID::BGM_Boss,					"./Data/Sounds/BGM/BossBattle.ogg",					true	},
			Bundle{ ID::BGM_Over,					"./Data/Sounds/BGM/GameOver.ogg",					false	},
			Bundle{ ID::BGM_Result,					"./Data/Sounds/BGM/Result.ogg",						true	},

			Bundle{ ID::Boss_Appear,				"./Data/Sounds/SE/Boss/BossPerformance.ogg",		false	},
			Bundle{ ID::Boss_Defeated,				"./Data/Sounds/SE/Boss/BossDefeated.ogg",			false	},

			Bundle{ ID::Bullet_HitBuster,			"./Data/Sounds/SE/Bullet/Hit_Buster.wav",			false	},
			Bundle{ ID::Bullet_HitShield,			"./Data/Sounds/SE/Bullet/Hit_Shield.wav",			false	},
			Bundle{ ID::Bullet_Protected,			"./Data/Sounds/SE/Bullet/Protected.wav",			false	},
			Bundle{ ID::Bullet_ShotBuster,			"./Data/Sounds/SE/Bullet/Shot_Buster.wav",			false	},
			Bundle{ ID::Bullet_ShotShield_Expand,	"./Data/Sounds/SE/Bullet/Shot_Shield_Expand.wav",	false	},
			Bundle{ ID::Bullet_ShotShield_Throw,	"./Data/Sounds/SE/Bullet/Shot_Shield_Throw.wav",	false	},
			Bundle{ ID::Bullet_ShotSkullBuster,		"./Data/Sounds/SE/Bullet/Shot_Skull_Buster.wav",	false	},
			
			Bundle{ ID::Charge_Complete,			"./Data/Sounds/SE/Effect/Charge_Complete.wav",		false	},
			Bundle{ ID::Charge_Loop,				"./Data/Sounds/SE/Effect/Charge_Loop.ogg",			true	},
			Bundle{ ID::Charge_Start,				"./Data/Sounds/SE/Effect/Charge_Start.wav",			false	},
			
			Bundle{ ID::Performance_AppearBoss,		"./Data/Sounds/SE/Performance/AppearBoss.ogg",		false	},
			Bundle{ ID::Performance_ClearStage,		"./Data/Sounds/SE/Performance/ClearStage.ogg",		false	},
			
			Bundle{ ID::Player_1UP,					"./Data/Sounds/SE/Player/ExtraLife.wav",			false	},
			Bundle{ ID::Player_Appear,				"./Data/Sounds/SE/Player/Appear.ogg",				false	},
			Bundle{ ID::Player_Damage,				"./Data/Sounds/SE/Player/Damage.wav",				false	},
			Bundle{ ID::Player_Dash,				"./Data/Sounds/SE/Player/Dash.wav",					false	},
			Bundle{ ID::Player_Jump,				"./Data/Sounds/SE/Player/Jump.wav",					false	},
			Bundle{ ID::Player_Landing,				"./Data/Sounds/SE/Player/Landing.wav",				false	},
			Bundle{ ID::Player_Leave,				"./Data/Sounds/SE/Player/Leave.ogg",				false	},
			Bundle{ ID::Player_Miss,				"./Data/Sounds/SE/Player/Miss.wav",					false	},
			
			Bundle{ ID::RecoverHP,					"./Data/Sounds/SE/Effect/RecoverHP.wav",			false	},
			
			Bundle{ ID::Skull_Landing,				"./Data/Sounds/SE/Boss/Skull_Landing.wav",			false	},
			Bundle{ ID::Skull_Jump,					"./Data/Sounds/SE/Boss/Skull_Jump.wav",				false	},
			Bundle{ ID::Skull_Roar,					"./Data/Sounds/SE/Boss/Skull_Roar.wav",				false	},
			
			Bundle{ ID::SuperBallMachine_Shot,		"./Data/Sounds/SE/Enemy/SBM_Shot.wav",				false	},
			
			#if DEBUG_MODE
			Bundle{ ID::DEBUG_Strong,				"./Data/Sounds/SE/_DEBUG/Strong.wav",				false	},
			Bundle{ ID::DEBUG_Weak,					"./Data/Sounds/SE/_DEBUG/Weak.wav",					false	},
			#endif // DEBUG_MODE
		};

		bool succeeded = true;
		for ( size_t i = 0; i < ID::MUSIC_COUNT; ++i )
		{
			bool result = Donya::Sound::Load
			(
				bundles[i].id,
				bundles[i].filePath,
				bundles[i].isEnableLoop
			);
			if ( !result ) { succeeded = false; }
		}

		_ASSERT_EXPR( succeeded, L"Failed: Sounds load is failed." );

		std::lock_guard<std::mutex> lock( *pSucceedMutex );
		*pFinishFlag  = true;
		*pSucceedFlag = succeeded;

		CoUninitialize();
	};

	finishModels	= false;
	finishSprites	= false;
	finishSounds	= false;
	allSucceeded	= true;

	// pThreadEffects	= std::make_unique<std::thread>( LoadingEffects,	&finishEffects,	&allSucceeded, &succeedMutex );
	LoadingEffects( &finishEffects,	&allSucceeded, &succeedMutex );

	pThreadModels	= std::make_unique<std::thread>( LoadingModels,		&finishModels,	&allSucceeded, &succeedMutex );
	pThreadSounds	= std::make_unique<std::thread>( LoadingSounds,		&finishSounds,	&allSucceeded, &succeedMutex );
	pThreadSprites	= std::make_unique<std::thread>( LoadingSprites,	&finishSprites,	&allSucceeded, &succeedMutex );

	if ( !SpritesInit() )
	{
		_ASSERT_EXPR( 0, L"Error: Loading sprites does not works!" );
	}
}
void SceneLoad::Uninit()
{
	ReleaseAllThread();
}

Scene::Result SceneLoad::Update( float elapsedTime )
{
#if USE_IMGUI
	UseImGui();
#endif // USE_IMGUI

#if DEBUG_MODE
	elapsedTimer += elapsedTime;
#endif // DEBUG_MODE

	SpritesUpdate( elapsedTime );

	if ( !Fader::Get().IsExist() && IsFinished() )
	{
		if ( allSucceeded )
		{
		#if USE_IMGUI
			if ( !stopFadeout )
		#endif // USE_IMGUI
			StartFade();
		}
		else
		{
			const HWND hWnd = Donya::GetHWnd();
			MessageBox
			(
				hWnd,
				TEXT( "リソースの読み込みに失敗しました。アプリを終了します。" ),
				TEXT( "リソース読み込みエラー" ),
				MB_ICONERROR | MB_OK
			);

			PostMessage( hWnd, WM_CLOSE, 0, 0 );

			ReleaseAllThread();

			// Prevent a true being returned by IsFinished().
			finishSprites	= false;
			finishSounds	= false;
		}
	}

	return ReturnResult();
}

void SceneLoad::Draw( float elapsedTime )
{
	ClearBackGround();

	const auto &data = FetchParameter();
	constexpr Donya::Vector2 pivot{ 0.5f, 0.5f };
	pFontRenderer->DrawExt
	(
		L"Now Loading...",
		data.sprLoadPos, pivot,
		data.sprLoadScale,
		{ 1.0f, 1.0f, 1.0f, fontAlpha }
	);
}

void SceneLoad::ReleaseAllThread()
{
	auto JoinThenRelease = []( std::unique_ptr<std::thread> &p )
	{
		if ( p )
		{ 
			if ( p->joinable() )
			{
				p->join();
			}

			p.reset();
		}
	};

	JoinThenRelease( pThreadModels	);
	JoinThenRelease( pThreadSprites	);
	JoinThenRelease( pThreadSounds	);
}

bool SceneLoad::SpritesInit()
{
	const auto &data = FetchParameter();

	bool succeeded = true;

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

	fontAlpha		= 1.0f;
	flushingTimer	= 0.0f;

	return succeeded;
}
void SceneLoad::SpritesUpdate( float elapsedTime )
{
	const auto &data = FetchParameter();

	if ( !IsZero( data.sprLoadFlushingInterval ) )
	{
		const float sinIncrement = 360.0f / ( 60.0f * data.sprLoadFlushingInterval );
		flushingTimer += sinIncrement * elapsedTime;

		const float sin_01 = ( sinf( flushingTimer ) + 1.0f ) * 0.5f;
		const float shake  = sin_01 * data.sprLoadFlushingRange;

		fontAlpha = std::max( data.sprLoadMinAlpha, std::min( 1.0f, shake ) );
	}
}

bool SceneLoad::IsFinished() const
{
	return ( finishEffects && finishModels && finishSprites && finishSounds );
}

void SceneLoad::ClearBackGround() const
{
	constexpr Donya::Vector3 gray{ Donya::Color::MakeColor( Donya::Color::Code::GRAY ) };
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );
}

void SceneLoad::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeSecond	= Fader::GetDefaultCloseSecond();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneLoad::ReturnResult()
{
	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
	#if DEBUG_MODE
		// change.sceneType = Scene::Type::Game;
		change.sceneType = Scene::Type::Title;
		// change.sceneType = Scene::Type::Result;
	#else
		change.sceneType = Scene::Type::Title;
	#endif // DEBUG_MODE
		return change;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
void SceneLoad::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"ロード画面のメンバ" ) )
		{
			ImGui::Checkbox( u8"フェードアウトを止めるか", &stopFadeout );

			sceneParam.ShowImGuiNode( u8"パラメータ調整" );

			auto GetBoolStr = []( bool v )->std::string
			{
				return ( v ) ? "True" : "False";
			};

			ImGui::Text( u8"終了フラグ・エフェクト[%s]",	GetBoolStr( finishEffects	).c_str() );
			ImGui::Text( u8"終了フラグ・モデル[%s]",		GetBoolStr( finishModels	).c_str() );
			ImGui::Text( u8"終了フラグ・スプライト[%s]",	GetBoolStr( finishSprites	).c_str() );
			ImGui::Text( u8"終了フラグ・サウンド[%s]",	GetBoolStr( finishSounds	).c_str() );
			
			ImGui::Text( u8"経過時間：[%6.3f]", elapsedTimer );

			ImGui::TreePop();
		}

		ImGui::End();
	}
}
#endif // USE_IMGUI
