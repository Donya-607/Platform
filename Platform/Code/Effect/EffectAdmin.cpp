#include "EffectAdmin.h"

#include "EffekseerRendererDX11.h"

#include "../Common.h"
#include "EffectUtil.h"

namespace Fx			= Effekseer;
namespace FxRenderer	= EffekseerRendererDX11;

namespace Effect
{
	class EffectWrapper final
	{
	private:
		Fx::Effect *pFx = nullptr;
	public:
		EffectWrapper( Fx::Manager *pManager, const std::basic_string<EFK_CHAR> &filePath, float magnification = 1.0f, const std::basic_string<EFK_CHAR> &materialPath = {} )
		{
			const EFK_CHAR *materialPathOrNullptr = ( materialPath.empty() ) ? nullptr : materialPath.c_str();
			pFx = Fx::Effect::Create( pManager, filePath.c_str(), magnification, materialPathOrNullptr );
		}
		~EffectWrapper()
		{
			ES_SAFE_RELEASE( pFx );
		}
	public:
		bool		IsValid() const
		{
			return ( pFx );
		}
		Fx::Effect	*GetEffectOrNullptr() const
		{
			return ( IsValid() ) ? pFx : nullptr;
		}
	};

	struct EffectAdmin::Impl
	{
	private:
		static constexpr int		maxInstanceCount	= 4096;
		static constexpr int32_t	maxSpriteCount		= 8192;
	private:
		Fx::Manager					*pManager			= nullptr;
		FxRenderer::Renderer		*pRenderer			= nullptr;
		bool						wasInitialized		= false;
	private:
		std::unordered_map<std::basic_string<EFK_CHAR>, std::shared_ptr<EffectWrapper>> fxMap;
	public:
		bool Init( ID3D11Device *pDevice, ID3D11DeviceContext *pContext )
		{
			if ( wasInitialized ) { return true; }
			// else

			if ( !pDevice || !pContext )
			{
				_ASSERT_EXPR( 0, L"Error: D3D device is null!" );
				return false;
			}
			// else

			// Create a renderer and manager
			{
				pRenderer = FxRenderer::Renderer::Create( pDevice, pContext, maxSpriteCount );
				pManager  = Fx::Manager::Create( maxInstanceCount );
			}
			if ( !pRenderer || !pManager )
			{
				_ASSERT_EXPR( 0, L"Failed: Effect initialize is failed." );
				return false;
			}
			// else

			// Specidy coordinate system
			pManager->SetCoordinateSystem( Fx::CoordinateSystem::LH );

			// Sprcify rendering modules
			{
				pManager->SetSpriteRenderer	( pRenderer->CreateSpriteRenderer()	);
				pManager->SetRibbonRenderer	( pRenderer->CreateRibbonRenderer()	);
				pManager->SetRingRenderer	( pRenderer->CreateRingRenderer()	);
				pManager->SetTrackRenderer	( pRenderer->CreateTrackRenderer()	);
				pManager->SetModelRenderer	( pRenderer->CreateModelRenderer()	);
			}

			// Specify a texture, model and material loader
			{
				pManager->SetTextureLoader	( pRenderer->CreateTextureLoader()	);
				pManager->SetModelLoader	( pRenderer->CreateModelLoader()	);
				pManager->SetMaterialLoader	( pRenderer->CreateMaterialLoader()	);
			}

			// Set default camera matrix and projection matrix
			{
				constexpr float defaultFOV	= ToRadian( 90.0f );
				constexpr float defaultNear	= 0.1f;
				constexpr float defaultFar	= 1000.0f;
				constexpr float aspect		= Common::ScreenWidthF() / Common::ScreenHeightF();

				const bool oldInitializedState = wasInitialized;
				wasInitialized = true;

				// These method requires "wasInitialized == true", so set true temporary.
				SetProjectionMatrix
				(
					Donya::Vector4x4::MakePerspectiveFovLH
					(
						defaultFOV,
						aspect,
						defaultNear,
						defaultFar
					)
				);
				SetViewMatrix
				(
					Donya::Vector4x4::MakeLookAtLH
					(
						-Donya::Vector3::Front(),
						Donya::Vector3::Zero(),
						Donya::Vector3::Up()
					)
				);

				wasInitialized = oldInitializedState;
			}

			fxMap.clear();
			wasInitialized = true;

			ParamEffect::Get().Init();
			return true;
		}
		void Uninit()
		{
			fxMap.clear();

			pManager->Destroy();
			pRenderer->Destroy();

			wasInitialized = false;
		}

		void Update( float updateSpeedMagnification )
		{
		#if USE_IMGUI
			ParamEffect::Get().UseImGui();
		#endif // USE_IMGUI

			assert( wasInitialized );
			pManager->Update( updateSpeedMagnification );
		}
	
		void Draw()
		{
			assert( wasInitialized );
			pRenderer->BeginRendering();
			pManager->Draw();
			pRenderer->EndRendering();
		}
	public:
		void SetViewMatrix( const Donya::Vector4x4 &m )
		{
			assert( wasInitialized );
			pRenderer->SetCameraMatrix( ToFxMatrix( m ) );
		}
		void SetProjectionMatrix( const Donya::Vector4x4 &m )
		{
			assert( wasInitialized );
			pRenderer->SetProjectionMatrix( ToFxMatrix( m ) );
		}
	public:
		Fx::Manager *GetManagerOrNullptr() const
		{
			assert( wasInitialized );
			return pManager;
		}
	public:
		bool LoadEffect( EffectAttribute attr )
		{
			assert( wasInitialized );
			const std::basic_string<EFK_CHAR> filePath = GetEffectPath( attr );

			// Has already loaded?
			const auto find = fxMap.find( filePath );
			if ( find != fxMap.end() ) { return true; }
			// else

			auto pointer = std::make_shared<EffectWrapper>( pManager, filePath );
			auto result  = fxMap.insert( std::make_pair( filePath, std::move( pointer ) ) );
			return result.second;
		}
		void UnloadEffect( EffectAttribute attr )
		{
			assert( wasInitialized );
			fxMap.erase( GetEffectPath( attr ) );
		}
		void UnloadEffectAll()
		{
			assert( wasInitialized );
			fxMap.clear();
		}
	public:
		bool IsOutOfRange( EffectAttribute attr )
		{
			return scast<size_t>( EffectAttribute::AttributeCount ) < scast<size_t>( attr );
		}
		float GetEffectScale( EffectAttribute attr )
		{
			if ( IsOutOfRange( attr ) ) { return 0.0f; }
			// else
			const auto scales = ParamEffect::Get().Data().effectScales;
			return scales[scast<size_t>( attr )];
		}
		Effekseer::Effect *GetEffectOrNullptr( EffectAttribute attr )
		{
			assert( wasInitialized );
		
			const auto find = fxMap.find( GetEffectPath( attr ) );
			if ( find == fxMap.end() ) { return nullptr; }
			// else

			return find->second->GetEffectOrNullptr();
		}
	};


	EffectAdmin::EffectAdmin()  : Singleton(), pImpl( std::make_unique<EffectAdmin::Impl>() ) {}
	EffectAdmin::~EffectAdmin() = default;

	bool EffectAdmin::Init( ID3D11Device *pDevice, ID3D11DeviceContext *pImmediateContext )
								{ return pImpl->Init( pDevice, pImmediateContext );	}
	void EffectAdmin::Uninit()
								{ pImpl->Uninit();									}
	void EffectAdmin::Update( float updateSpeedMagni )
								{ pImpl->Update( updateSpeedMagni );				}
	void EffectAdmin::Draw()
								{ pImpl->Draw();									}
	void EffectAdmin::SetViewMatrix( const Donya::Vector4x4 &m )
								{ pImpl->SetViewMatrix( m );						}
	void EffectAdmin::SetProjectionMatrix( const Donya::Vector4x4 &m )
								{ pImpl->SetProjectionMatrix( m );					}
	Fx::Manager *EffectAdmin::GetManagerOrNullptr() const
								{ return pImpl->GetManagerOrNullptr();				}
	bool EffectAdmin::LoadEffect( EffectAttribute attr )
								{ return pImpl->LoadEffect( attr );					}
	void EffectAdmin::UnloadEffect( EffectAttribute attr )
								{ pImpl->UnloadEffect( attr );						}
	void EffectAdmin::UnloadEffectAll()
								{ pImpl->UnloadEffectAll();							}
	float EffectAdmin:: GetEffectScale( EffectAttribute attr )
								{ return pImpl->GetEffectScale( attr ); }
	Effekseer::Effect *EffectAdmin::GetEffectOrNullptr( EffectAttribute attr )
								{ return pImpl->GetEffectOrNullptr( attr );			}

}
