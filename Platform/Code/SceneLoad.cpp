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
#include "Door.h"
#include "Enemy.h"
#include "Effect/EffectAdmin.h"
#include "Fader.h"
#include "FilePath.h"
#include "Item.h"
#include "Input.h"
#include "Meter.h"
#include "Music.h"
#include "RenderingStuff.h"
#include "Parameter.h"
#include "Player.h"
#include "SaveData.h"

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
	SaveData::Admin::Get().Load();
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

		if ( !Boss	::LoadResource() ) { succeeded = false; }
		if ( !Bullet::LoadResource() ) { succeeded = false; }
		if ( !Door	::LoadResource() ) { succeeded = false; }
		if ( !Enemy	::LoadResource() ) { succeeded = false; }
		if ( !Item	::LoadResource() ) { succeeded = false; }
		if ( !Meter	::LoadResource() ) { succeeded = false; }
		if ( !Player::LoadResource() ) { succeeded = false; }

		_ASSERT_EXPR( succeeded, L"Failed: Models load is failed." );

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
			const char *filePath;
			bool		isEnableLoop;
		public:
			constexpr Bundle( ID id, const char *filePath, bool isEnableLoop )
				: id( id ), filePath( filePath ), isEnableLoop( isEnableLoop )
			{}
		};

		constexpr std::array<Bundle, ID::MUSIC_COUNT> bundles
		{
			// ID, FilePath, isEnableLoop

			Bundle{ ID::BGM_Title,						"./Data/Sounds/BGM/Title.ogg",							true	},
			Bundle{ ID::BGM_Game,						"./Data/Sounds/BGM/Game.ogg",							true	},
			Bundle{ ID::BGM_Boss,						"./Data/Sounds/BGM/BossBattle.ogg",						true	},
			Bundle{ ID::BGM_Over,						"./Data/Sounds/BGM/GameOver.ogg",						false	},
			Bundle{ ID::BGM_Result,						"./Data/Sounds/BGM/Result.ogg",							true	},

			Bundle{ ID::Bullet_HitBone,					"./Data/Sounds/SE/Bullet/Hit_Bone.ogg",					false	},
			Bundle{ ID::Bullet_HitBuster,				"./Data/Sounds/SE/Bullet/Hit_Buster.wav",				false	},
			Bundle{ ID::Bullet_HitShield,				"./Data/Sounds/SE/Bullet/Hit_Shield.wav",				false	},
			Bundle{ ID::Bullet_HitSuperBall,			"./Data/Sounds/SE/Bullet/Hit_SuperBall.ogg",			false	},
			Bundle{ ID::Bullet_Protected,				"./Data/Sounds/SE/Bullet/Protected.wav",				false	},
			Bundle{ ID::Bullet_ShotBone,				"./Data/Sounds/SE/Bullet/Shot_Bone.ogg",				false	},
			Bundle{ ID::Bullet_ShotBuster,				"./Data/Sounds/SE/Bullet/Shot_Buster.wav",				false	},
			Bundle{ ID::Bullet_ShotShield_Expand,		"./Data/Sounds/SE/Bullet/Shot_Shield_Expand.wav",		false	},
			Bundle{ ID::Bullet_ShotShield_Throw,		"./Data/Sounds/SE/Bullet/Shot_Shield_Throw.wav",		false	},
			Bundle{ ID::Bullet_ShotSkullBuster,			"./Data/Sounds/SE/Bullet/Shot_Skull_Buster.wav",		false	},

			Bundle{ ID::CatchItem,						"./Data/Sounds/SE/Effect/CatchItem.ogg",				false	},

			Bundle{ ID::Charge_Complete,				"./Data/Sounds/SE/Effect/Charge_Complete.wav",			false	},
			Bundle{ ID::Charge_Loop,					"./Data/Sounds/SE/Effect/Charge_Loop.ogg",				true	},
			Bundle{ ID::Charge_Start,					"./Data/Sounds/SE/Effect/Charge_Start.wav",				false	},

			Bundle{ ID::Door_OpenClose,					"./Data/Sounds/SE/Map/Door_OpenClose.ogg",				false	},

			Bundle{ ID::Performance_AppearBoss,			"./Data/Sounds/SE/Performance/AppearBoss.ogg",			false	},
			Bundle{ ID::Performance_ClearStage,			"./Data/Sounds/SE/Performance/ClearStage.ogg",			false	},

			Bundle{ ID::Player_1UP,						"./Data/Sounds/SE/Player/ExtraLife.wav",				false	},
			Bundle{ ID::Player_Appear,					"./Data/Sounds/SE/Player/Appear.ogg",					false	},
			Bundle{ ID::Player_Damage,					"./Data/Sounds/SE/Player/Damage.wav",					false	},
			Bundle{ ID::Player_Dash,					"./Data/Sounds/SE/Player/Dash.wav",						false	},
			Bundle{ ID::Player_Jump,					"./Data/Sounds/SE/Player/Jump.wav",						false	},
			Bundle{ ID::Player_Landing,					"./Data/Sounds/SE/Player/Landing.wav",					false	},
			Bundle{ ID::Player_Leave,					"./Data/Sounds/SE/Player/Leave.ogg",					false	},
			Bundle{ ID::Player_Miss,					"./Data/Sounds/SE/Player/Miss.wav",						false	},
			Bundle{ ID::Player_ShiftGun,				"./Data/Sounds/SE/Player/ShiftGun.ogg",					false	},
			Bundle{ ID::Player_Shoryuken,				"./Data/Sounds/SE/Player/Shoryuken.ogg",				false	},

			Bundle{ ID::RecoverHP,						"./Data/Sounds/SE/Effect/RecoverHP.wav",				false	},

			Bundle{ ID::SkeletonJoe_Break,				"./Data/Sounds/SE/Enemy/SklJoe_Break.ogg",				false	},
			Bundle{ ID::SkeletonJoe_ReAssemble_Begin,	"./Data/Sounds/SE/Enemy/SklJoe_ReAssemble_Begin.ogg",	false	},
			Bundle{ ID::SkeletonJoe_ReAssemble_End,		"./Data/Sounds/SE/Enemy/SklJoe_ReAssemble_End.ogg",		false	},

			Bundle{ ID::Skull_Landing,					"./Data/Sounds/SE/Boss/Skull_Landing.wav",				false	},
			Bundle{ ID::Skull_Jump,						"./Data/Sounds/SE/Boss/Skull_Jump.wav",					false	},
			Bundle{ ID::Skull_Roar,						"./Data/Sounds/SE/Boss/Skull_Roar.wav",					false	},

			Bundle{ ID::SuperBallMachine_Shot,			"./Data/Sounds/SE/Enemy/SBM_Shot.wav",					false	},

			Bundle{ ID::UI_Choose,						"./Data/Sounds/SE/UI/Choose.ogg",						false	},
			Bundle{ ID::UI_Decide,						"./Data/Sounds/SE/UI/Decide.ogg",						false	},

			#if DEBUG_MODE
			Bundle{ ID::DEBUG_Strong,					"./Data/Sounds/SE/UI/Decide.ogg",						false	},
			Bundle{ ID::DEBUG_Weak,						"./Data/Sounds/SE/UI/Choose.ogg",						false	},
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
			return  ( handle == NULL ) ? false : true;
		};

		bool succeeded = true;
		if ( !MakeTextureCache( Attr::TitleLogo		) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::InputButtons	) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::Meter			) ) { succeeded = false; }

		_ASSERT_EXPR( succeeded, L"Failed: Sprites load is failed." );

		pResult->WriteResult( succeeded );

		CoUninitialize();
	};
	auto CreateRenderer	= [coInitValue]( Thread::Result *pResult )
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

		const bool succeeded = RenderingStuffInstance::Get().Initialize();

		_ASSERT_EXPR( succeeded, L"Failed: Rendering stuffs creation is failed." );

		pResult->WriteResult( succeeded );

		CoUninitialize();
	};

	using FunctionType = std::function<void( Thread::Result * )>;
	std::array<FunctionType, ThreadKind::ThreadCount> functions
	{
		LoadingEffects,
		LoadingModels,
		LoadingSounds,
		LoadingSprites,
		CreateRenderer
	};

#if LOAD_EFFECT_BY_ANOTHER_THREAD
	for ( int i = 0; i < ThreadKind::ThreadCount; ++i )
	{
		threads[i].pThread =
		std::make_unique<std::thread>( functions[i], &threads[i].result );
	}
#else
	for ( int i = 0; i < ThreadKind::ThreadCount; ++i )
	{
		if ( i == Effect ) { continue; }
		// else

		threads[i].pThread =
		std::make_unique<std::thread>( functions[i], &threads[i].result );
	}

	LoadingEffects( &threads[Effect].result );
#endif // LOAD_EFFECT_BY_ANOTHER_THREAD
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
	for ( auto &th : threads )
	{
		th.JoinThenRelease();
	}
}

bool SceneLoad::AllFinished() const
{
	for ( auto &th : threads )
	{
		if ( !th.result.Finished() ) { return false; }
	}

	return true;
}
bool SceneLoad::AllSucceeded() const
{
	for ( auto &th : threads )
	{
		if ( !th.result.Succeeded() ) { return false; }
	}

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
		change.sceneType = Scene::Type::Game;
		// change.sceneType = Scene::Type::Title;
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
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else

	ImGui::Checkbox( u8"フェードアウトを止める", &stopFadeout );

	ImGui::Text( u8"経過時間：[%6.3f]", elapsedTimer );

	sceneParam.ShowImGuiNode( u8"ロード画面のパラメータ" );

	auto GetBoolStr = []( bool v )->std::string
	{
		return ( v ) ? "True" : "False";
	};
	auto GetNameStr = []( ThreadKind kind )->std::string
	{
		std::string str = u8"終了フラグ・";

		switch ( kind )
		{
		case SceneLoad::Effect:		str += u8"エフェクト";	break;
		case SceneLoad::Models:		str += u8"モデル";		break;
		case SceneLoad::Sounds:		str += u8"スプライト";	break;
		case SceneLoad::Sprites:	str += u8"サウンド";		break;
		case SceneLoad::Renderer:	str += u8"レンダラ";		break;
		default: str += u8"!ERROR_KIND!"; break;
		}

		str += u8"[%s]";
		return str;
	};

	for ( int i = 0; i < ThreadKind::ThreadCount; ++i )
	{
		ImGui::Text
		(
			GetNameStr( scast<ThreadKind>( i )			).c_str(),
			GetBoolStr( threads[i].result.Finished()	).c_str()
		);
	}

	ImGui::End();
}
#endif // USE_IMGUI
