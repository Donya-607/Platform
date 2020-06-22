#include "RenderingStates.h"

#include <d3d11.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include <wrl.h>

#include "Donya.h"			// For fetch the library's device and immediate-context.
#include "Constant.h"		// Use scast macro.


using namespace Microsoft::WRL;

namespace
{
	void SetDefaultDeviceIfNull( ID3D11Device **ppDevice )
	{
		if ( *ppDevice != nullptr ) { return; }
		// else

		*ppDevice = Donya::GetDevice();
	}
	void SetDefaultImmediateContextIfNull( ID3D11DeviceContext **ppContext )
	{
		if ( *ppContext != nullptr ) { return; }
		// else

		*ppContext = Donya::GetImmediateContext();
	}

	int  FindUsableIdentifierImpl( const std::function<bool( int )> &IsAlreadyExists, bool findPositiveDirection )
	{
		int found = 0;
		auto AssignIfUsable = [&]( int i )->bool
		{
			if ( IsAlreadyExists( i ) ) { return false; }
			// else
			found = i;
			return true;
		};

		if ( findPositiveDirection )
		{
			for ( int i = 1; i < INT_MAX; ++i )
			{
				if ( AssignIfUsable( i ) ) { break; }
			}
		}
		else
		{
			for ( int i = -1; -INT_MAX < i; --i )
			{
				if ( AssignIfUsable( i ) ) { break; }
			}
		}

		return found;
	}
}


namespace Donya
{
	// The Blend-State is there at "Blend.h".

	namespace DepthStencil
	{
		static std::unordered_map<int, ComPtr<ID3D11DepthStencilState>> mapDepthStencil{};
		static ComPtr<ID3D11DepthStencilState> oldState{}; // Use for Deactivate().
		
		bool CreateState( int id, const D3D11_DEPTH_STENCIL_DESC &desc, ID3D11Device *pDevice )
		{
			if ( IsAlreadyExists( id ) ) { return true; }
			// else

			SetDefaultDeviceIfNull( &pDevice );

			HRESULT hr = S_OK;
			ComPtr<ID3D11DepthStencilState> tmpStateObject{};

			hr = pDevice->CreateDepthStencilState( &desc, tmpStateObject.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create internal depth stencil state." );
				return false;
			}
			// else

			mapDepthStencil.insert
			(
				std::make_pair
				(
					id,
					tmpStateObject
				)
			);

			return true;
		}
		bool CreateDefinedStates( ID3D11Device *pDevice )
		{
			bool succeeded	= true;
			bool result		= true;

			D3D11_DEPTH_STENCIL_DESC desc{};
			desc.DepthEnable	= TRUE;
			desc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ALL;
			desc.StencilEnable	= FALSE;


			desc.DepthFunc		= D3D11_COMPARISON_LESS;
			result = CreateState( scast<int>( Defined::Write_PassLess ), desc, pDevice );
			if ( !result ) { succeeded = false; }

			desc.DepthFunc		= D3D11_COMPARISON_LESS_EQUAL;
			result = CreateState( scast<int>( Defined::Write_PassLessEq ), desc, pDevice );
			if ( !result ) { succeeded = false; }
			
			desc.DepthFunc		= D3D11_COMPARISON_GREATER;
			result = CreateState( scast<int>( Defined::Write_PassLess ), desc, pDevice );
			if ( !result ) { succeeded = false; }

			desc.DepthFunc		= D3D11_COMPARISON_GREATER_EQUAL;
			result = CreateState( scast<int>( Defined::Write_PassLessEq ), desc, pDevice );
			if ( !result ) { succeeded = false; }


			return succeeded;
		}

		bool IsAlreadyExists( int id )
		{
			const auto found =  mapDepthStencil.find( id );
			return   ( found != mapDepthStencil.end() );
		}
		int  FindUsableIdentifier( bool positive )
		{
			return FindUsableIdentifierImpl( IsAlreadyExists, positive );
		}

		constexpr unsigned int STENCIL_REF = 0xFFFFFFFF;
		bool Activate( int id, ID3D11DeviceContext *pContext )
		{
			auto found =  mapDepthStencil.find( id );
			if ( found == mapDepthStencil.end() ) { return false; }
			// else

			SetDefaultImmediateContextIfNull( &pContext );

			pContext->OMGetDepthStencilState( oldState.ReleaseAndGetAddressOf(), NULL );
			pContext->OMSetDepthStencilState( found->second.Get(), STENCIL_REF );

			return true;
		}
		void Deactivate( ID3D11DeviceContext *pContext )
		{
			SetDefaultImmediateContextIfNull( &pContext );
			pContext->OMSetDepthStencilState( oldState.Get(), STENCIL_REF );
		}

		void ReleaseAllCachedStates()
		{
			mapDepthStencil.clear();
		}
	}

	namespace Rasterizer
	{
		static std::unordered_map<int, ComPtr<ID3D11RasterizerState>> mapRasterizer{};
		static ComPtr<ID3D11RasterizerState> oldState{};

		bool CreateState( int id, const D3D11_RASTERIZER_DESC &desc, ID3D11Device *pDevice )
		{
			if ( IsAlreadyExists( id ) ) { return true; }
			// else

			SetDefaultDeviceIfNull( &pDevice );

			HRESULT hr = S_OK;
			ComPtr<ID3D11RasterizerState> tmpStateObject{};

			hr = pDevice->CreateRasterizerState( &desc, tmpStateObject.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create internal rasterizer state." );
				return false;
			}
			// else

			mapRasterizer.insert
			(
				std::make_pair
				(
					id,
					tmpStateObject
				)
			);

			return true;
		}
		bool CreateDefinedStates( ID3D11Device *pDevice )
		{
			bool succeeded	= true;
			bool result		= true;

			D3D11_RASTERIZER_DESC desc{};
			desc.DepthBias				= 0;
			desc.DepthBiasClamp			= 0.0f;
			desc.SlopeScaledDepthBias	= 0.0f;
			desc.DepthClipEnable		= TRUE;
			desc.ScissorEnable			= FALSE;
			desc.MultisampleEnable		= FALSE;
			desc.AntialiasedLineEnable	= FALSE;


			desc.FillMode				= D3D11_FILL_SOLID;
			desc.CullMode				= D3D11_CULL_BACK;
			desc.FrontCounterClockwise	= TRUE;
			result = CreateState( scast<int>( Defined::Solid_CullBack_CCW ), desc, pDevice );
			if ( !result ) { succeeded = false; }

			desc.FrontCounterClockwise	= FALSE;
			result = CreateState( scast<int>( Defined::Solid_CullBack_CW ), desc, pDevice );
			if ( !result ) { succeeded = false; }

			desc.CullMode				= D3D11_CULL_NONE;
			result = CreateState( scast<int>( Defined::Solid_CullNone ), desc, pDevice );
			if ( !result ) { succeeded = false; }
			
			desc.DepthClipEnable		= FALSE;
			result = CreateState( scast<int>( Defined::Solid_CullNone_NoClip ), desc, pDevice );
			if ( !result ) { succeeded = false; }
			

			desc.FillMode				= D3D11_FILL_WIREFRAME;
			desc.CullMode				= D3D11_CULL_BACK;
			desc.FrontCounterClockwise	= TRUE;
			result = CreateState( scast<int>( Defined::Wired_CullBack_CCW ), desc, pDevice );
			if ( !result ) { succeeded = false; }

			desc.FrontCounterClockwise	= FALSE;
			result = CreateState( scast<int>( Defined::Wired_CullBack_CW ), desc, pDevice );
			if ( !result ) { succeeded = false; }

			desc.CullMode				= D3D11_CULL_NONE;
			result = CreateState( scast<int>( Defined::Wired_CullNone ), desc, pDevice );
			if ( !result ) { succeeded = false; }
			
			desc.DepthClipEnable		= FALSE;
			result = CreateState( scast<int>( Defined::Wired_CullNone_NoClip ), desc, pDevice );
			if ( !result ) { succeeded = false; }


			return succeeded;
		}

		bool IsAlreadyExists( int id )
		{
			const auto found =  mapRasterizer.find( id );
			return   ( found != mapRasterizer.end() );
		}
		int  FindUsableIdentifier( bool positive )
		{
			return FindUsableIdentifierImpl( IsAlreadyExists, positive );
		}

		bool Activate( int id, ID3D11DeviceContext *pContext )
		{
			auto found =  mapRasterizer.find( id );
			if ( found == mapRasterizer.end() ) { return false; }
			// else

			SetDefaultImmediateContextIfNull( &pContext );

			pContext->RSGetState( oldState.ReleaseAndGetAddressOf() );
			pContext->RSSetState( found->second.Get() );

			return true;
		}
		void Deactivate( ID3D11DeviceContext *pContext )
		{
			SetDefaultImmediateContextIfNull( &pContext );
			pContext->RSSetState( oldState.Get() );
		}

		void ReleaseAllCachedStates()
		{
			mapRasterizer.clear();
		}
	}

	namespace Sampler
	{
		static std::unordered_map<int, ComPtr<ID3D11SamplerState>> mapSampler{};
		
		bool CreateState( int id, const D3D11_SAMPLER_DESC &desc, ID3D11Device *pDevice )
		{
			if ( IsAlreadyExists( id ) ) { return true; }
			// else

			SetDefaultDeviceIfNull( &pDevice );

			HRESULT hr = S_OK;
			ComPtr<ID3D11SamplerState> tmpStateObject{};

			hr = pDevice->CreateSamplerState( &desc, tmpStateObject.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create internal rasterizer state." );
				return false;
			}
			// else

			mapSampler.insert
			(
				std::make_pair
				(
					id,
					tmpStateObject
				)
			);

			return true;
		}
		bool CreateDefinedStates( ID3D11Device *pDevice )
		{
			bool succeeded	= true;
			bool result		= true;

			D3D11_SAMPLER_DESC desc{};
			desc.MipLODBias		= 0.0f;
			desc.MaxAnisotropy	= 1U;
			desc.ComparisonFunc	= D3D11_COMPARISON_NEVER;
			desc.MinLOD			= 0;
			desc.MaxLOD			= D3D11_FLOAT32_MAX;
			
			
			desc.Filter			= D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU		= D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV		= D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW		= D3D11_TEXTURE_ADDRESS_WRAP;
			result = CreateState( scast<int>( Defined::Point_Wrap ), desc, pDevice );
			if ( !result ) { succeeded = false; }

			desc.Filter			= D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			result = CreateState( scast<int>( Defined::Linear_Wrap ), desc, pDevice );
			if ( !result ) { succeeded = false; }
			
			desc.Filter			= D3D11_FILTER_ANISOTROPIC;
			result = CreateState( scast<int>( Defined::Aniso_Wrap ), desc, pDevice );
			if ( !result ) { succeeded = false; }


			desc.Filter			= D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU		= D3D11_TEXTURE_ADDRESS_BORDER;
			desc.AddressV		= D3D11_TEXTURE_ADDRESS_BORDER;
			desc.AddressW		= D3D11_TEXTURE_ADDRESS_BORDER;
			desc.BorderColor[0]	= 0.0f;
			desc.BorderColor[1]	= 0.0f;
			desc.BorderColor[2]	= 0.0f;
			desc.BorderColor[3]	= 1.0f;
			result = CreateState( scast<int>( Defined::Point_Border_Black ), desc, pDevice );
			if ( !result ) { succeeded = false; }
			
			desc.BorderColor[3]	= 0.0f;
			result = CreateState( scast<int>( Defined::Point_Border_Clear ), desc, pDevice );
			if ( !result ) { succeeded = false; }
			
			desc.BorderColor[0] = 1.0f;
			desc.BorderColor[1] = 1.0f;
			desc.BorderColor[2] = 1.0f;
			desc.BorderColor[3]	= 1.0f;
			result = CreateState( scast<int>( Defined::Point_Border_White ), desc, pDevice );
			if ( !result ) { succeeded = false; }

			return succeeded;
		}

		bool IsAlreadyExists( int id )
		{
			const auto found =  mapSampler.find( id );
			return   ( found != mapSampler.end() );
		}
		int  FindUsableIdentifier( bool positive )
		{
			return FindUsableIdentifierImpl( IsAlreadyExists, positive );
		}

		bool SetVS( int id, unsigned int setSlot, ID3D11DeviceContext *pContext )
		{
			auto found =  mapSampler.find( id );
			if ( found == mapSampler.end() ) { return false; }
			// else

			SetDefaultImmediateContextIfNull( &pContext );

			pContext->VSSetSamplers( setSlot, 1U, found->second.GetAddressOf() );
			
			return true;
		}
		bool SetGS( int id, unsigned int setSlot, ID3D11DeviceContext *pContext )
		{
			auto found =  mapSampler.find( id );
			if ( found == mapSampler.end() ) { return false; }
			// else

			SetDefaultImmediateContextIfNull( &pContext );

			pContext->GSSetSamplers( setSlot, 1U, found->second.GetAddressOf() );
			
			return true;
		}
		bool SetPS( int id, unsigned int setSlot, ID3D11DeviceContext *pContext )
		{
			auto found =  mapSampler.find( id );
			if ( found == mapSampler.end() ) { return false; }
			// else

			SetDefaultImmediateContextIfNull( &pContext );

			pContext->PSSetSamplers( setSlot, 1U, found->second.GetAddressOf() );
			
			return true;
		}
		void ResetVS( unsigned int setSlot, ID3D11DeviceContext *pContext )
		{
			SetDefaultImmediateContextIfNull( &pContext );
			constexpr ID3D11SamplerState *empty = nullptr;
			pContext->VSSetSamplers( setSlot, 1U, &empty );
		}
		void ResetGS( unsigned int setSlot, ID3D11DeviceContext *pContext )
		{
			SetDefaultImmediateContextIfNull( &pContext );
			constexpr ID3D11SamplerState *empty = nullptr;
			pContext->GSSetSamplers( setSlot, 1U, &empty );
		}
		void ResetPS( unsigned int setSlot, ID3D11DeviceContext *pContext )
		{
			SetDefaultImmediateContextIfNull( &pContext );
			constexpr ID3D11SamplerState *empty = nullptr;
			pContext->PSSetSamplers( setSlot, 1U, &empty );
		}
		
		void ReleaseAllCachedStates()
		{
			mapSampler.clear();
		}
	}
}

