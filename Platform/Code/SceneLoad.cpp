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
#include "Input.h"
#include "Meter.h"
#include "Music.h"
#include "Parameter.h"
#include "Player.h"

#define LOAD_EFFECT_BY_ANOTHER_THREAD ( false )


namespace
{
	struct Member
	{
		Donya::Vector2 ssLoadingDrawPos{ 800.0f, 450.0f };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( ssLoadingDrawPos ) );

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode()
		{
			ImGui::DragFloat2( u8"Loading：表示座標", &ssLoadingDrawPos.x );
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
	Performer::LoadPart::LoadParameter();
	Input::LoadParameter();

	loadPerformer.Init();
	loadPerformer.Start( FetchParameter().ssLoadingDrawPos, Donya::Color::Code::GRAY );
	
	constexpr auto coInitValue = COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE;
	auto LoadingEffects	= [coInitValue]( Thread::Result *pResult )
	{
		if ( !pResult ) { assert( !"HUMAN ERROR" ); return; }
		// else

	#if LOAD_EFFECT_BY_ANOTHER_THREAD
		HRESULT hr = CoInitializeEx( NULL, coInitValue );
		if ( FAILED( hr ) )
		{
			pResult->WriteResult( /* wasSucceeded = */ false );
			return;
		}
		// else
	#endif // LOAD_EFFECT_BY_ANOTHER_THREAD

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

		pResult->WriteResult( succeeded );

	#if LOAD_EFFECT_BY_ANOTHER_THREAD
		CoUninitialize();
	#endif // LOAD_EFFECT_BY_ANOTHER_THREAD
	};
	auto LoadingModels	= [coInitValue]( Thread::Result *pResult )
	{
		if ( !pResult ) { assert( !"HUMAN ERROR" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, coInitValue );
		if ( FAILED( hr ) )
		{
			pResult->WriteResult( /* wasSucceeded = */ false );
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

		pResult->WriteResult( succeeded );

		CoUninitialize();
	};
	auto LoadingSprites	= [coInitValue]( Thread::Result *pResult )
	{
		if ( !pResult ) { assert( !"HUMAN ERROR" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, coInitValue );
		if ( FAILED( hr ) )
		{
			pResult->WriteResult( /* wasSucceeded = */ false );
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
		if ( !MakeTextureCache( Attr::TitleLogo		) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::InputButtons	) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::Meter			) ) { succeeded = false; }
		
		_ASSERT_EXPR( succeeded, L"Failed: Sprites load is failed." );

		pResult->WriteResult( succeeded );

		CoUninitialize();
	};
	auto LoadingSounds	= [coInitValue]( Thread::Result *pResult )
	{
		if ( !pResult ) { assert( !"HUMAN ERROR" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, coInitValue );
		if ( FAILED( hr ) )
		{
			pResult->WriteResult( /* wasSucceeded = */ false );
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
			Bundle{ ID::Player_ShiftGun,			"./Data/Sounds/SE/Player/ShiftGun.ogg",				false	},
			
			Bundle{ ID::RecoverHP,					"./Data/Sounds/SE/Effect/RecoverHP.wav",			false	},
			
			Bundle{ ID::Skull_Landing,				"./Data/Sounds/SE/Boss/Skull_Landing.wav",			false	},
			Bundle{ ID::Skull_Jump,					"./Data/Sounds/SE/Boss/Skull_Jump.wav",				false	},
			Bundle{ ID::Skull_Roar,					"./Data/Sounds/SE/Boss/Skull_Roar.wav",				false	},
			
			Bundle{ ID::SuperBallMachine_Shot,		"./Data/Sounds/SE/Enemy/SBM_Shot.wav",				false	},
			
			Bundle{ ID::UI_Choose,					"./Data/Sounds/SE/UI/Choose.ogg",					false	},
			Bundle{ ID::UI_Decide,					"./Data/Sounds/SE/UI/Decide.ogg",					false	},
			
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

		pResult->WriteResult( succeeded );

		CoUninitialize();
	};

#if LOAD_EFFECT_BY_ANOTHER_THREAD
	thEffects.pThread	= std::make_unique<std::thread>( LoadingModels, &thEffects.result );
#else
	LoadingEffects( &thEffects.result );
#endif // LOAD_EFFECT_BY_ANOTHER_THREAD

	thModels.pThread	= std::make_unique<std::thread>( LoadingModels,		&thModels.result  );
	thSounds.pThread	= std::make_unique<std::thread>( LoadingSounds,		&thSounds.result  );
	thSprites.pThread	= std::make_unique<std::thread>( LoadingSprites,	&thSprites.result );
}
void SceneLoad::Uninit()
{
	ReleaseAllThread();

	loadPerformer.Stop();
	loadPerformer.Uninit();
}

Scene::Result SceneLoad::Update( float elapsedTime )
{
#if USE_IMGUI
	UseImGui();
#endif // USE_IMGUI

#if DEBUG_MODE
	elapsedTimer += elapsedTime;
#endif // DEBUG_MODE

	loadPerformer.UpdateIfActive( elapsedTime );

	if ( !Fader::Get().IsExist() && AllFinished() )
	{
		if ( AllSucceeded() )
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

			exit( -1 );
		}
	}

	return ReturnResult();
}

void SceneLoad::Draw( float elapsedTime )
{
	ClearBackGround();

	loadPerformer.DrawIfActive( 0.0f );
}

void SceneLoad::ReleaseAllThread()
{
	thEffects.JoinThenRelease();
	thModels.JoinThenRelease();
	thSounds.JoinThenRelease();
	thSprites.JoinThenRelease();
}

bool SceneLoad::AllFinished() const
{
	if ( !thEffects	.result.Finished() ) { return false; }
	if ( !thModels	.result.Finished() ) { return false; }
	if ( !thSounds	.result.Finished() ) { return false; }
	if ( !thSprites	.result.Finished() ) { return false; }

	return true;
}
bool SceneLoad::AllSucceeded() const
{
	if ( !thEffects	.result.Succeeded() ) { return false; }
	if ( !thModels	.result.Succeeded() ) { return false; }
	if ( !thSounds	.result.Succeeded() ) { return false; }
	if ( !thSprites	.result.Succeeded() ) { return false; }

	return true;
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
		// change.sceneType = Scene::Type::Title;
		change.sceneType = Scene::Type::Result;
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

			ImGui::Text( u8"終了フラグ・エフェクト[%s]",	GetBoolStr( thEffects	.result.Finished() ).c_str() );
			ImGui::Text( u8"終了フラグ・モデル[%s]",		GetBoolStr( thModels	.result.Finished() ).c_str() );
			ImGui::Text( u8"終了フラグ・スプライト[%s]",	GetBoolStr( thSounds	.result.Finished() ).c_str() );
			ImGui::Text( u8"終了フラグ・サウンド[%s]",	GetBoolStr( thSprites	.result.Finished() ).c_str() );
			
			ImGui::Text( u8"経過時間：[%6.3f]", elapsedTimer );

			ImGui::TreePop();
		}

		ImGui::End();
	}
}
#endif // USE_IMGUI
