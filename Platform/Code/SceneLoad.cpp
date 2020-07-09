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
#include "Fader.h"
#include "FilePath.h"
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

			if ( ImGui::TreeNode( u8"�X�v���C�g�̒���" ) )
			{
				if ( ImGui::TreeNode( u8"���[�h��" ) )
				{
					ImGui::DragFloat( u8"�X�P�[��",			&sprLoadScale,				0.1f );
					ImGui::DragFloat( u8"�_�Ŏ����i�b�j",		&sprLoadFlushingInterval,	0.1f );
					ImGui::DragFloat( u8"�_�Ŕ͈�",			&sprLoadFlushingRange,		0.1f );
					ImGui::DragFloat( u8"�Œ�A���t�@�l",		&sprLoadMinAlpha,			0.1f );
					ImGui::DragFloat2( u8"�X�N���[�����W",	&sprLoadPos.x );

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

void SceneLoad::Init()
{
	sceneParam.LoadParameter();
	
	constexpr auto CoInitValue = COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE;
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
			std::string	filePath;
			bool		isEnableLoop;
		public:
			Bundle( ID id, const char *filePath, bool isEnableLoop )
				: id( id ), filePath( filePath ), isEnableLoop( isEnableLoop ) {}
		};

		const std::array<Bundle, ID::MUSIC_COUNT> bundles
		{
			// ID, FilePath, isEnableLoop

			// { ID::BGM_Title,				"./Data/Sounds/BGM/.wav",					true	},

			Bundle{ ID::Bullet_HitBuster,	"./Data/Sounds/SE/Bullet/HitBuster.wav",	false	},
			
			Bundle{ ID::Player_Damage,		"./Data/Sounds/SE/Player/Damage.wav",		false	},
			Bundle{ ID::Player_Jump,		"./Data/Sounds/SE/Player/Jump.wav",			false	},
			Bundle{ ID::Player_Landing,		"./Data/Sounds/SE/Player/Landing.wav",		false	},
			Bundle{ ID::Player_Miss,		"./Data/Sounds/SE/Player/Miss.wav",			false	},
			Bundle{ ID::Player_Shot,		"./Data/Sounds/SE/Player/Shot.wav",			false	},

			#if DEBUG_MODE
			Bundle{ ID::DEBUG_Strong,		"./Data/Sounds/SE/_DEBUG/Strong.wav",		false	},
			Bundle{ ID::DEBUG_Weak,			"./Data/Sounds/SE/_DEBUG/Weak.wav",			false	},
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
			StartFade();
		}
		else
		{
			const HWND hWnd = Donya::GetHWnd();
			MessageBox
			(
				hWnd,
				TEXT( "���\�[�X�̓ǂݍ��݂����s���܂����B�A�v�����I�����܂��B" ),
				TEXT( "���\�[�X�ǂݍ��݃G���[" ),
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

	sprNowLoading.Draw();
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

	constexpr size_t MAX_INSTANCE_COUNT = 1U;
	if ( !sprNowLoading.LoadSprite( GetSpritePath( SpriteAttribute::NowLoading ), MAX_INSTANCE_COUNT ) )
	{ succeeded = false; }

	sprNowLoading.pos		= data.sprLoadPos;
	sprNowLoading.drawScale	= data.sprLoadScale;
	sprNowLoading.alpha		= 1.0f;

	flushingTimer			= 0.0f;

	return succeeded;
}
void SceneLoad::SpritesUpdate( float elapsedTime )
{
	const auto &data = FetchParameter();

	const float cycle = data.sprLoadFlushingInterval * elapsedTime;
	if ( !IsZero( cycle ) )
	{
		const float sinIncrement = 360.0f / ( 60.0f * cycle );
		flushingTimer += sinIncrement;

		const float sin_01 = ( sinf( ToRadian( flushingTimer ) ) + 1.0f ) * 0.5f;
		const float shake  = sin_01 * data.sprLoadFlushingRange;

		sprNowLoading.alpha = std::max( data.sprLoadMinAlpha, std::min( 1.0f, shake ) );
	}
}

bool SceneLoad::IsFinished() const
{
	return ( finishModels && finishSprites && finishSounds );
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
	config.closeFrame	= Fader::GetDefaultCloseFrame();;
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
		if ( ImGui::TreeNode( u8"���[�h��ʂ̃����o" ) )
		{
			auto GetBoolStr = []( bool v )->std::string
			{
				return ( v ) ? "True" : "False";
			};

			sceneParam.ShowImGuiNode( u8"�p�����[�^����" );

			ImGui::Text( u8"�I���t���O�E���f��[%s]",		GetBoolStr( finishModels	).c_str() );
			ImGui::Text( u8"�I���t���O�E�X�v���C�g[%s]",	GetBoolStr( finishSprites	).c_str() );
			ImGui::Text( u8"�I���t���O�E�T�E���h[%s]",	GetBoolStr( finishSounds	).c_str() );
			
			ImGui::Text( u8"�o�ߎ��ԁF[%6.3f]", elapsedTimer );

			sprNowLoading.ShowImGuiNode	( u8"�摜�����E���[�h��" );

			ImGui::TreePop();
		}

		ImGui::End();
	}
}
#endif // USE_IMGUI
