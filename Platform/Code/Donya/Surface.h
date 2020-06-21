#pragma once

#include <d3d11.h>
#include <wrl.h>

#include "Color.h"		// Use to clear-color.
#include "UseImGui.h"
#include "Vector.h"		// Use to clear-color, and specify the left-top position of viewport.

namespace Donya
{
	/// <summary>
	/// The Surface class behave renderable texture. What like a wrapper of render-target.
	/// </summary>
	class Surface
	{
	private:
		bool wasCreated{ false };
		Donya::Int2 wholeSize{ -1, -1 };

		mutable unsigned int	slotVS{};	// Store a specified slot of VS. Use for reset the shader-resource.
		mutable unsigned int	slotPS{};	// Store a specified slot of PS. Use for reset the shader-resource.
		mutable unsigned int	slotGS{};	// Store a specified slot of GS. Use for reset the shader-resource.
		mutable D3D11_VIEWPORT	viewport{};

		template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
		mutable ComPtr<ID3D11RenderTargetView>		pRTV{};
		mutable ComPtr<ID3D11DepthStencilView>		pDSV{};
		mutable ComPtr<ID3D11ShaderResourceView>	pSRV{};
	public:
		/// <summary>
		/// Returns true if the initialization was succeeded, or already initialized.<para></para>
		/// If set nullptr to pDevice, a default device will be used.
		/// </summary>
		bool Init( int wholeWidth, int wholeHeight, DXGI_FORMAT renderTargetFormat, ID3D11Device *pDevice = nullptr );
		/// <summary>
		/// Returns true if already initialized.
		/// </summary>
		bool IsEnable() const;
	public:
		/// <summary>
		/// Returns whole-size of surface. That is same as passed size when creating.
		/// </summary>
		Donya::Int2		GetSurfaceSize() const;
		/// <summary>
		/// Returns whole-size of surface. That is same as passed size when creating(the type-conversion by static_cast).
		/// </summary>
		Donya::Vector2	GetSurfaceSizeF() const;
		/// <summary>
		/// Returns shader-resource view with the com-pointer.
		/// </summary>
		ComPtr<ID3D11ShaderResourceView> GetShaderResourceView() const;
	public:
		/// <summary>
		/// Set render-target to me.<para></para>
		/// You can specify the setting position by use "ssLTPos"(screen-space, left-top).<para></para>
		/// If set nullptr to pImmediateContext, a default device will be used.
		/// </summary>
		void SetTarget( const Donya::Vector2 &ssLTPos = { 0.0f, 0.0f }, ID3D11DeviceContext * pImmediateContext = nullptr ) const;
		/// <summary>
		/// Set a default render-target. This is same as call Donya::SetDefaultRenderTargets() fuction(at Donya.h).
		/// </summary>
		void ResetTarget() const;
		/// <summary>
		/// Clear render-target view, depth-stencil view.<para></para>
		/// If set nullptr to pImmediateContext, a default device will be used.
		/// </summary>
		void Clear( const Donya::Vector4 clearColor, ID3D11DeviceContext *pImmediateContext = nullptr ) const;
		/// <summary>
		/// Clear render-target view, depth-stencil view.<para></para>
		/// If set nullptr to pImmediateContext, a default device will be used.
		/// </summary>
		void Clear( Donya::Color::Code clearColor, float alpha = 1.0f, ID3D11DeviceContext *pImmediateContext = nullptr ) const;
	public:
		/// <summary>
		/// Set my shader-resource view to vertex-shader.<para></para>
		/// If set nullptr to pImmediateContext, a default device will be used.
		/// </summary>
		void SetShaderResourceVS( unsigned int setSlot, ID3D11DeviceContext *pImmediateContext = nullptr ) const;
		/// <summary>
		/// Set the NULL shader-resource view to vertex-shader.<para></para>
		/// If set nullptr to pImmediateContext, a default device will be used.
		/// </summary>
		void ResetShaderResourceVS( ID3D11DeviceContext *pImmediateContext = nullptr ) const;

		/// <summary>
		/// Set my shader-resource view to pixel-shader.<para></para>
		/// If set nullptr to pImmediateContext, a default device will be used.
		/// </summary>
		void SetShaderResourcePS( unsigned int setSlot, ID3D11DeviceContext *pImmediateContext = nullptr ) const;
		/// <summary>
		/// Set the NULL shader-resource view to pixel-shader.<para></para>
		/// If set nullptr to pImmediateContext, a default device will be used.
		/// </summary>
		void ResetShaderResourcePS( ID3D11DeviceContext *pImmediateContext = nullptr ) const;

		/// <summary>
		/// Set my shader-resource view to geometry-shader.<para></para>
		/// If set nullptr to pImmediateContext, a default device will be used.
		/// </summary>
		void SetShaderResourceGS( unsigned int setSlot, ID3D11DeviceContext *pImmediateContext = nullptr ) const;
		/// <summary>
		/// Set the NULL shader-resource view to geometry-shader.<para></para>
		/// If set nullptr to pImmediateContext, a default device will be used.
		/// </summary>
		void ResetShaderResourceGS( ID3D11DeviceContext *pImmediateContext = nullptr ) const;
	public:
	#if USE_IMGUI

		/// <summary>
		/// Only call ImGui::Image(), so you should call this between ImGui::Begin() and ImGui::End().
		/// </summary>
		void RenderToImGui( const Donya::Vector2 drawSize ) const;

	#endif // USE_IMGUI
	};
}
