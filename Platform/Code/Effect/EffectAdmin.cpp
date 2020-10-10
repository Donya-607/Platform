#include "EffectAdmin.h"

#include "EffekseerRendererDX11.h"

#include "../Donya/Useful.h"	// Use OutputDebugStr()

#include "../Common.h"
#include "EffectParam.h"
#include "EffectUtil.h"
#include "../Parameter.h"

namespace Fx			= Effekseer;
namespace FxRenderer	= EffekseerRendererDX11;

namespace
{
	static constexpr size_t kindCount = scast<size_t>( Effect::Kind::KindCount );

	constexpr bool IsOutOfRange( Effect::Kind kind )
	{
		return scast<size_t>( Effect::Kind::KindCount ) < scast<size_t>( kind );
	}

	static ParamOperator<Effect::Param> effectParam{ "Effect" };
	const Effect::Param &FetchParameter()
	{
		return effectParam.Get();
	}

}

namespace Effect
{
#if USE_IMGUI
	void Param::ShowImGuiNode()
	{
		if ( effectScales.size() != kindCount )
		{
			constexpr float defaultScale = 1.0f;
			effectScales.resize( kindCount, defaultScale );
		}

		if ( ImGui::TreeNode( u8"スケール調整" ) )
		{
			ImGui::Text( u8"生成時に適用されます" );
			
			std::string caption{};
			for ( size_t i = 0; i < kindCount; ++i )
			{
				caption = GetEffectName( scast<Effect::Kind>( i ) );
				ImGui::DragFloat( caption.c_str(), &effectScales[i], 0.01f );
			}

			ImGui::TreePop();
		}
	}
#endif // USE_IMGUI

	Admin::Instance::Instance( Effekseer::Manager *pManager, const stdEfkString &filePath, float scale, const stdEfkString &mtlPath )
	{
		const EFK_CHAR *mtlPathOrNullptr = ( mtlPath.empty() ) ? nullptr : mtlPath.c_str();
		pHandle = Fx::Effect::Create( pManager, filePath.c_str(), scale, mtlPathOrNullptr );

		if ( !IsValid() )
		{
		#if DEBUG_MODE
			std::wstring errMsg = L"Failed: Loading an effect file: ";
			errMsg += Effect::ToWString( filePath );
			errMsg += L"\n";
			Donya::OutputDebugStr( errMsg.c_str() );
			_ASSERT_EXPR( 0, errMsg );
		#endif // DEBUG_MODE
		}
	}
	Admin::Instance::~Instance()
	{
		ES_SAFE_RELEASE( pHandle );
	}
	bool Admin::Instance::IsValid() const
	{
		return ( pHandle );
	}
	Fx::Effect *Admin::Instance::GetEffectOrNullptr()
	{
		return ( IsValid() ) ? pHandle : nullptr;
	}

	bool Admin::Init( ID3D11Device *pDevice, ID3D11DeviceContext *pContext )
	{
		instances.clear();
		LoadParameter();

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

		wasInitialized = true;
		return true;
	}
	void Admin::Uninit()
	{
		instances.clear();

		pManager->Destroy();
		pRenderer->Destroy();

		wasInitialized = false;
	}

	void Admin::Update( float updateSpeedMagnification )
	{
		assert( wasInitialized );
		pManager->Update( updateSpeedMagnification );
	}
	
	void Admin::Draw()
	{
		assert( wasInitialized );
		pRenderer->BeginRendering();
		pManager->Draw();
		pRenderer->EndRendering();
	}

	void Admin::SetViewMatrix( const Donya::Vector4x4 &m )
	{
		assert( wasInitialized );
		pRenderer->SetCameraMatrix( ToFxMatrix( m ) );
	}
	void Admin::SetProjectionMatrix( const Donya::Vector4x4 &m )
	{
		assert( wasInitialized );
		pRenderer->SetProjectionMatrix( ToFxMatrix( m ) );
	}

	Fx::Manager *Admin::GetManagerOrNullptr() const
	{
		assert( wasInitialized );
		return pManager;
	}

	void Admin::LoadParameter()
	{
		effectParam.LoadParameter();
	}
	bool Admin::LoadEffect( Effect::Kind attr )
	{
		assert( wasInitialized );
		const stdEfkString filePath = GetEffectPath( attr );

		// Has already loaded?
		const auto find = instances.find( filePath );
		if ( find != instances.end() ) { return true; }
		// else

		auto pointer = std::make_shared<Instance>( pManager, filePath );
		auto result  = instances.insert( std::make_pair( filePath, std::move( pointer ) ) );
		return result.second;
	}
	void Admin::UnloadEffect( Effect::Kind attr )
	{
		assert( wasInitialized );
		instances.erase( GetEffectPath( attr ) );
	}
	void Admin::UnloadEffectAll()
	{
		assert( wasInitialized );
		instances.clear();
	}

	float Admin::GetEffectScale( Effect::Kind attr )
	{
		if ( IsOutOfRange( attr ) ) { return 1.0f; }
		// else
		const auto scales = FetchParameter().effectScales;
		return ( scales.empty() ) ? 1.0f : scales[scast<size_t>( attr )];
	}
	Effekseer::Effect *Admin::GetEffectOrNullptr( Effect::Kind attr )
	{
		assert( wasInitialized );
		
		const auto find = instances.find( GetEffectPath( attr ) );
		if ( find == instances.end() ) { return nullptr; }
		// else

		return find->second->GetEffectOrNullptr();
	}

#if USE_IMGUI
	void Admin::ShowImGuiNode( const std::string &nodeCaption )
	{
		effectParam.ShowImGuiNode( nodeCaption );
	}
#endif // USE_IMGUI

}
