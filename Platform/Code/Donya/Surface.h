#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <vector>

#include "Color.h"		// Use for Clear()
#include "UseImGui.h"
#include "Vector.h"

namespace Donya
{
	/// <summary>
	/// The Surface class behave renderable texture. A frame-buffer. A render-target.
	/// </summary>
	class Surface
	{
	public:
		/// <summary>
		/// Set render-target views.You should call together a SetViewport().<para></para>
		/// The viewport size and depth-stencil view will using first surface's.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		static void SetRenderTargets( const std::vector<Surface> &surfaces, ID3D11DeviceContext *pContext = nullptr );
		/// <summary>
		/// Reset to null the render targets from zero slot to max slot.
		/// </summary>
		static void ResetRenderTargets( size_t resetCount, ID3D11DeviceContext *pContext = nullptr );
		/// <summary>
		/// Set a default render-target. This is same as call Donya::SetDefaultRenderTargets() fuction(at Donya.h).
		/// </summary>
		static void ResetRenderTarget();
	private:
		bool wasCreated{ false };
		Donya::Int2 wholeSize{ -1, -1 };

		mutable D3D11_VIEWPORT viewport{};

		template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
		mutable ComPtr<ID3D11RenderTargetView>		pRTV{};
		mutable ComPtr<ID3D11ShaderResourceView>	pRTSRV{};
		mutable ComPtr<ID3D11DepthStencilView>		pDSV{};
		mutable ComPtr<ID3D11ShaderResourceView>	pDSSRV{};
	public:
		/// <summary>
		/// Returns true if the initialization was succeeded, or already initialized.<para></para>
		/// If you set DXGI_UNKNOWN to format, that parameter is not create.<para></para>
		/// If set nullptr to pDevice, a default device will be used.
		/// </summary>
		bool Init( int wholeWidth, int wholeHeight, DXGI_FORMAT renderTargetTexture2DFormat, bool needRenderTargetSRV = true, DXGI_FORMAT depthStencilTexture2DFormat = DXGI_FORMAT_R24G8_TYPELESS, bool needDepthStencilSRV = true, ID3D11Device *pDevice = nullptr );
		/// <summary>
		/// Returns true if already initialized.
		/// </summary>
		bool IsInitialized() const;
	public:
		/// <summary>
		/// Returns whole-size of surface. That is same as passed size when creating.
		/// </summary>
		Donya::Int2		GetSurfaceSize() const;
		/// <summary>
		/// Returns whole-size of surface. That is same as passed size when creating(the interactionType-conversion by static_cast).
		/// </summary>
		Donya::Vector2	GetSurfaceSizeF() const;
		ComPtr<ID3D11RenderTargetView> GetRenderTargetView() const;
		ComPtr<ID3D11DepthStencilView> GetDepthStencilView() const;
		/// <summary>
		/// Returns render-target's shader-resource view with the com-pointer.
		/// </summary>
		ComPtr<ID3D11ShaderResourceView> GetRenderTargetShaderResource() const;
		/// <summary>
		/// Returns depth-stencil's shader-resource view with the com-pointer.
		/// </summary>
		ComPtr<ID3D11ShaderResourceView> GetDepthStencilShaderResource() const;
	public:
		/// <summary>
		/// Set render-target to me.You should call together the SetViewport().<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void SetRenderTarget( ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Clear render-target view and depth-stencil view.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void Clear( const Donya::Vector4 clearColor, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Clear render-target view and depth-stencil view.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void Clear( Donya::Color::Code clearColor, float alpha = 1.0f, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Clear render-target view.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void ClearRenderTarget( const Donya::Vector4 clearColor, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Clear render-target view.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void ClearRenderTarget( Donya::Color::Code clearColor, float alpha = 1.0f, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Clear depth-stencil view.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void ClearDepthStencil( ID3D11DeviceContext *pContext = nullptr ) const;
	public:
		/// <summary>
		/// Set render-target's shader-resource view to vertex-shader.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void SetRenderTargetShaderResourceVS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Set depth-stencil's shader-resource view to vertex-shader.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void SetDepthStencilShaderResourceVS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Set render-target's shader-resource view to geometry-shader.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void SetRenderTargetShaderResourceGS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Set depth-stencil's shader-resource view to geometry-shader.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void SetDepthStencilShaderResourceGS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Set render-target's shader-resource view to pixel-shader.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void SetRenderTargetShaderResourcePS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Set depth-stencil's shader-resource view to pixel-shader.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void SetDepthStencilShaderResourcePS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Set the NULL shader-resource view to vertex-shader.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void ResetShaderResourceVS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Set the NULL shader-resource view to pixel-shader.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void ResetShaderResourcePS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Set the NULL shader-resource view to geometry-shader.<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void ResetShaderResourceGS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr ) const;
		/// <summary>
		/// Set only the viewport. You should call this together when call SetRenderTarget().<para></para>
		/// You can specify the setting position by use "ssLTPos"(screen-space, left-top).<para></para>
		/// If set nullptr to pContext, a default device will be used.
		/// </summary>
		void SetViewport( const Donya::Vector2 &ssLTPos = { 0.0f, 0.0f }, ID3D11DeviceContext *pContext = nullptr ) const;
	public:
	#if USE_IMGUI
		/// <summary>
		/// Only call ImGui::Image(), so you should call this between ImGui::Begin() and ImGui::End().
		/// </summary>
		void DrawRenderTargetToImGui( const Donya::Vector2 drawSize ) const;
		/// <summary>
		/// Only call ImGui::Image(), so you should call this between ImGui::Begin() and ImGui::End().
		/// </summary>
		void DrawDepthStencilToImGui( const Donya::Vector2 drawSize ) const;
	#endif // USE_IMGUI
	};
}
