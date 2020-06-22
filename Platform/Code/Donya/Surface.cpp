#include "Surface.h"

#include "Donya.h" // Use for getting a default device and immediate-context, call a setting default render-targets function.

#undef max
#undef min

namespace Donya
{
	void SetDefaultIfNull( ID3D11Device **ppDevice )
	{
		if ( *ppDevice != nullptr ) { return; }
		// else
		*ppDevice = Donya::GetDevice();
	}
	void SetDefaultIfNull( ID3D11DeviceContext **ppImmediateContext )
	{
		if ( *ppImmediateContext != nullptr ) { return; }
		// else
		*ppImmediateContext = Donya::GetImmediateContext();
	}

	bool Surface::Init( int wholeWidth, int wholeHeight, DXGI_FORMAT rtFormat, bool needRTSRV, DXGI_FORMAT dsFormat, bool needDSSRV, ID3D11Device *pDevice )
	{
		if ( wasCreated ) { return true; }
		// else

		SetDefaultIfNull( &pDevice );

		HRESULT hr = S_OK;

		if ( rtFormat != DXGI_FORMAT_UNKNOWN )
		{
			ComPtr<ID3D11Texture2D> pRTBuffer{};
			D3D11_TEXTURE2D_DESC descRTTexture{};
			descRTTexture.Width					= static_cast<UINT>( wholeWidth  );
			descRTTexture.Height				= static_cast<UINT>( wholeHeight );
			descRTTexture.MipLevels				= 1;
			descRTTexture.ArraySize				= 1;
			descRTTexture.Format				= rtFormat;
			descRTTexture.SampleDesc.Count		= 1;
			descRTTexture.SampleDesc.Quality	= 0;
			descRTTexture.Usage					= D3D11_USAGE_DEFAULT;
			descRTTexture.BindFlags				= D3D11_BIND_RENDER_TARGET;
			if ( needRTSRV )
			{ descRTTexture.BindFlags			|= D3D11_BIND_SHADER_RESOURCE; }
			descRTTexture.CPUAccessFlags		= 0;
			hr = pDevice->CreateTexture2D( &descRTTexture, NULL, pRTBuffer.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create texture at Surface." );
				return false;
			}
			// else

			D3D11_RENDER_TARGET_VIEW_DESC descRTView{};
			descRTView.Format					= descRTTexture.Format;
			descRTView.ViewDimension			= D3D11_RTV_DIMENSION_TEXTURE2D;
			hr = pDevice->CreateRenderTargetView( pRTBuffer.Get(), &descRTView, pRTV.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create render-target view at Surface." );
				return false;
			}
			// else

			if ( needRTSRV )
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC descSRView{};
				descSRView.Format						= descRTView.Format;
				descSRView.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
				descSRView.Texture2D.MipLevels			= 1;
				descSRView.Texture2D.MostDetailedMip	= 0;
				hr = pDevice->CreateShaderResourceView( pRTBuffer.Get(), &descSRView, pRTSRV.GetAddressOf() );
				if ( FAILED( hr ) )
				{
					_ASSERT_EXPR( 0, L"Failed : Create shader-resource view at Surface." );
					return false;
				}
			}
		}

		if ( dsFormat != DXGI_FORMAT_UNKNOWN )
		{
			struct DSFormat
			{
				DXGI_FORMAT texture = DXGI_FORMAT_UNKNOWN;
				DXGI_FORMAT dsView  = DXGI_FORMAT_UNKNOWN;
				DXGI_FORMAT srView  = DXGI_FORMAT_UNKNOWN;
			public:
				constexpr DSFormat( DXGI_FORMAT tex, DXGI_FORMAT view, DXGI_FORMAT srv )
					: texture( tex ), dsView( view ), srView( srv ) {}
			};

			constexpr DSFormat dsFormats[3]
			{
				DSFormat{ DXGI_FORMAT_R24G8_TYPELESS,	DXGI_FORMAT_D24_UNORM_S8_UINT,	DXGI_FORMAT_R24_UNORM_X8_TYPELESS	},
				DSFormat{ DXGI_FORMAT_R32_TYPELESS,		DXGI_FORMAT_D32_FLOAT,			DXGI_FORMAT_R32_FLOAT				},
				DSFormat{ DXGI_FORMAT_R16_TYPELESS,		DXGI_FORMAT_D16_UNORM,			DXGI_FORMAT_R16_UNORM				},
			};
			int useFormatIndex = 0;
			if ( dsFormat == DXGI_FORMAT_R24G8_TYPELESS	) { useFormatIndex = 0; }
			else
			if ( dsFormat == DXGI_FORMAT_R32_TYPELESS	) { useFormatIndex = 1; }
			else
			if ( dsFormat == DXGI_FORMAT_R16_TYPELESS	) { useFormatIndex = 2; }
			else
			{
				_ASSERT_EXPR( 0, L"Error: Not supported depth-stencil format!" );
				return false;
			}
			// else

			ComPtr<ID3D11Texture2D> pDSBuffer{};
			D3D11_TEXTURE2D_DESC descDSTexture{};
			descDSTexture.Width					= static_cast<UINT>( wholeWidth  );
			descDSTexture.Height				= static_cast<UINT>( wholeHeight );
			descDSTexture.MipLevels				= 1;
			descDSTexture.ArraySize				= 1;
			descDSTexture.Format				= dsFormats[useFormatIndex].texture;
			descDSTexture.SampleDesc.Count		= 1;
			descDSTexture.SampleDesc.Quality	= 0;
			descDSTexture.Usage					= D3D11_USAGE_DEFAULT;
			descDSTexture.BindFlags				= D3D11_BIND_DEPTH_STENCIL;
			if ( needDSSRV )
			{ descDSTexture.BindFlags			|= D3D11_BIND_SHADER_RESOURCE; }
			descDSTexture.CPUAccessFlags		= 0;
			hr = pDevice->CreateTexture2D( &descDSTexture, NULL, pDSBuffer.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create texture at Surface." );
				return false;
			}
			// else

			D3D11_DEPTH_STENCIL_VIEW_DESC descDSView{};
			descDSView.Format				= dsFormats[useFormatIndex].dsView;
			descDSView.ViewDimension		= D3D11_DSV_DIMENSION_TEXTURE2D;
			descDSView.Flags				= 0;
			descDSView.Texture2D.MipSlice	= 0;
			hr = pDevice->CreateDepthStencilView( pDSBuffer.Get(), &descDSView, pDSV.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create depth-stencil view at Surface." );
				return false;
			}
			// else

			if ( needDSSRV )
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC descSRView{};
				descSRView.Format						= dsFormats[useFormatIndex].srView;
				descSRView.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
				descSRView.Texture2D.MipLevels			= 1;
				descSRView.Texture2D.MostDetailedMip	= 0;
				hr = pDevice->CreateShaderResourceView( pDSBuffer.Get(), &descSRView, pDSSRV.GetAddressOf() );
				if ( FAILED( hr ) )
				{
					_ASSERT_EXPR( 0, L"Failed : Create shader-resource view at Surface." );
					return false;
				}
			}
		}

	#pragma region Viewport

		viewport.TopLeftX	= 0.0f;
		viewport.TopLeftY	= 0.0f;
		viewport.Width		= static_cast<FLOAT>( wholeWidth  );
		viewport.Height		= static_cast<FLOAT>( wholeHeight );
		viewport.MinDepth	= D3D11_MIN_DEPTH;
		viewport.MaxDepth	= D3D11_MAX_DEPTH;

	// region Viewport
	#pragma endregion

		wholeSize.x = wholeWidth;
		wholeSize.y = wholeHeight;

		wasCreated = true;
		return true;
	}

	bool Surface::IsEnable() const
	{
		return wasCreated;
	}

	Donya::Int2		Surface::GetSurfaceSize() const
	{
		return wholeSize;
	}
	Donya::Vector2	Surface::GetSurfaceSizeF() const
	{
		return GetSurfaceSize().Float();
	}
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Surface::GetRenderTargetShaderResource() const
	{
		return pRTSRV;
	}
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Surface::GetDepthStencilShaderResource() const
	{
		return pDSSRV;
	}

	void Surface::SetTarget( const Donya::Vector2 &ssLTPos, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		viewport.TopLeftX = ssLTPos.x;
		viewport.TopLeftY = ssLTPos.y;

		pImmediateContext->RSSetViewports( 1U, &viewport );
		pImmediateContext->OMSetRenderTargets( 1U, pRTV.GetAddressOf(), pDSV.Get() );
	}
	void Surface::ResetTarget() const
	{
		Donya::SetDefaultRenderTargets();
	}

	void Surface::Clear( const Donya::Vector4 clearColor, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		const FLOAT colors[4]{ clearColor.x, clearColor.y, clearColor.z, clearColor.w };
		if ( pRTV ) { pImmediateContext->ClearRenderTargetView( pRTV.Get(), colors ); }
		if ( pDSV ) { pImmediateContext->ClearDepthStencilView( pDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 ); }
	}
	void Surface::Clear( Donya::Color::Code   clearColor, float alpha, ID3D11DeviceContext *pImmediateContext ) const
	{
		Clear
		(
			Donya::Vector4{ Donya::Color::MakeColor( clearColor ), alpha },
			pImmediateContext
		);
	}

	void Surface::SetRenderTargetShaderResourceVS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );
		pImmediateContext->VSSetShaderResources( slot, 1U, pRTSRV.GetAddressOf() );
	}
	void Surface::SetDepthStencilShaderResourceVS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );
		pImmediateContext->VSSetShaderResources( slot, 1U, pDSSRV.GetAddressOf() );
	}
	void Surface::SetRenderTargetShaderResourceGS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );
		pImmediateContext->GSSetShaderResources( slot, 1U, pRTSRV.GetAddressOf() );
	}
	void Surface::SetDepthStencilShaderResourceGS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );
		pImmediateContext->GSSetShaderResources( slot, 1U, pDSSRV.GetAddressOf() );
	}
	void Surface::SetRenderTargetShaderResourcePS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );
		pImmediateContext->PSSetShaderResources( slot, 1U, pRTSRV.GetAddressOf() );
	}
	void Surface::SetDepthStencilShaderResourcePS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );
		pImmediateContext->PSSetShaderResources( slot, 1U, pDSSRV.GetAddressOf() );
	}
	
	void Surface::ResetShaderResourceVS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		ID3D11ShaderResourceView *pNullSRV = nullptr;
		pImmediateContext->VSSetShaderResources( slot, 1U, &pNullSRV );
	}
	void Surface::ResetShaderResourceGS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		ID3D11ShaderResourceView *pNullSRV = nullptr;
		pImmediateContext->GSSetShaderResources( slot, 1U, &pNullSRV );
	}
	void Surface::ResetShaderResourcePS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		ID3D11ShaderResourceView *pNullSRV = nullptr;
		pImmediateContext->PSSetShaderResources( slot, 1U, &pNullSRV );
	}

#if USE_IMGUI
	void Surface::DrawRenderTargetToImGui( const Donya::Vector2 drawSize ) const
	{
		ImGui::Image( static_cast<void *>( pRTSRV.Get() ), ImVec2{ drawSize.x, drawSize.y } );
	}
	void Surface::DrawDepthStencilToImGui( const Donya::Vector2 drawSize ) const
	{
		ImGui::Image( static_cast<void *>( pDSSRV.Get() ), ImVec2{ drawSize.x, drawSize.y } );
	}
#endif // USE_IMGUI

}
