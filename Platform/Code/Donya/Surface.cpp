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

	bool Surface::Init( int wholeWidth, int wholeHeight, DXGI_FORMAT format, ID3D11Device *pDevice )
	{
		if ( wasCreated ) { return true; }
		// else

		SetDefaultIfNull( &pDevice );

		HRESULT hr = S_OK;

	#pragma region RenderTargetView

		ComPtr<ID3D11Texture2D> pRTBuffer{};
		D3D11_TEXTURE2D_DESC descRTTexture{};
		descRTTexture.Width					= static_cast<UINT>( wholeWidth  );
		descRTTexture.Height				= static_cast<UINT>( wholeHeight );
		descRTTexture.MipLevels				= 1;
		descRTTexture.ArraySize				= 1;
		descRTTexture.Format				= format;
		descRTTexture.SampleDesc.Count		= 1;
		descRTTexture.SampleDesc.Quality	= 0;
		descRTTexture.Usage					= D3D11_USAGE_DEFAULT;
		descRTTexture.BindFlags				= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
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

	// region RenderTargetView
	#pragma endregion

	#pragma region ShaderResourceView

		D3D11_SHADER_RESOURCE_VIEW_DESC descSRView{};
		descSRView.Format						= descRTView.Format;
		descSRView.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		descSRView.Texture2D.MipLevels			= 1;
		descSRView.Texture2D.MostDetailedMip	= 0;
		hr = pDevice->CreateShaderResourceView( pRTBuffer.Get(), &descSRView, pSRV.GetAddressOf() );
		if ( FAILED( hr ) )
		{
			_ASSERT_EXPR( 0, L"Failed : Create shader-resource view at Surface." );
			return false;
		}
		// else

	// region ShaderResourceView
	#pragma endregion

	#pragma region DepthStencilView

		ComPtr<ID3D11Texture2D> pDSBuffer{};
		D3D11_TEXTURE2D_DESC descDSTexture = descRTTexture;
		descDSTexture.Format		= DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDSTexture.BindFlags		= D3D11_BIND_DEPTH_STENCIL;
		hr = pDevice->CreateTexture2D( &descDSTexture, NULL, pDSBuffer.GetAddressOf() );
		if ( FAILED( hr ) )
		{
			_ASSERT_EXPR( 0, L"Failed : Create texture at Surface." );
			return false;
		}
		// else

		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV{};
		descDSV.Format				= descDSTexture.Format;
		descDSV.ViewDimension		= D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Flags				= 0;
		descDSV.Texture2D.MipSlice	= 0;
		hr = pDevice->CreateDepthStencilView( pDSBuffer.Get(), &descDSV, pDSV.GetAddressOf() );
		if ( FAILED( hr ) )
		{
			_ASSERT_EXPR( 0, L"Failed : Create depth-stencil view at Surface." );
			return false;
		}
		// else

	// region DepthStencilView
	#pragma endregion

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
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Surface::GetShaderResourceView() const
	{
		return pSRV;
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
		pImmediateContext->ClearRenderTargetView( pRTV.Get(), colors );
		pImmediateContext->ClearDepthStencilView( pDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
	}
	void Surface::Clear( Donya::Color::Code   clearColor, float alpha, ID3D11DeviceContext *pImmediateContext ) const
	{
		Clear
		(
			Donya::Vector4{ Donya::Color::MakeColor( clearColor ), alpha },
			pImmediateContext
		);
	}

	void Surface::SetShaderResourceVS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		slotVS = slot;
		pImmediateContext->VSSetShaderResources( slotVS, 1U, pSRV.GetAddressOf() );
	}
	void Surface::ResetShaderResourceVS( ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		ID3D11ShaderResourceView *pNullSRV = nullptr;
		pImmediateContext->VSSetShaderResources( slotVS, 1U, &pNullSRV );
	}

	void Surface::SetShaderResourcePS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		slotPS = slot;
		pImmediateContext->PSSetShaderResources( slotPS, 1U, pSRV.GetAddressOf() );
	}
	void Surface::ResetShaderResourcePS( ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		ID3D11ShaderResourceView *pNullSRV = nullptr;
		pImmediateContext->PSSetShaderResources( slotPS, 1U, &pNullSRV );
	}

	void Surface::SetShaderResourceGS( unsigned int slot, ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		slotGS = slot;
		pImmediateContext->GSSetShaderResources( slotGS, 1U, pSRV.GetAddressOf() );
	}
	void Surface::ResetShaderResourceGS( ID3D11DeviceContext *pImmediateContext ) const
	{
		SetDefaultIfNull( &pImmediateContext );

		ID3D11ShaderResourceView *pNullSRV = nullptr;
		pImmediateContext->GSSetShaderResources( slotGS, 1U, &pNullSRV );
	}

#if USE_IMGUI

	void Surface::RenderToImGui( const Donya::Vector2 drawSize ) const
	{
		ImGui::Image( static_cast<void *>( pSRV.Get() ), ImVec2{ drawSize.x, drawSize.y } );
	}

#endif // USE_IMGUI

}
