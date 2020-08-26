#pragma once

#include <array>
#include <d3d11.h>
#include <memory>
#include <wrl.h>

#include "Vector.h"

namespace Donya
{
	/// <summary>
	/// Displayer only doing call a draw with quad polygon.
	/// It does not set rendering states nor any shader.
	/// </summary>
	class Displayer
	{
	public:
		struct Vertex
		{
			Donya::Vector3 pos;		// NDC
			Donya::Vector4 color;	// RGBA
			Donya::Vector2 texCoord;// Origin is left-top
		public:
			static constexpr const auto GenerateInputElements( UINT inputSlot = 0 )
			{
				return std::array<D3D11_INPUT_ELEMENT_DESC, 3>
				{
					D3D11_INPUT_ELEMENT_DESC{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		inputSlot, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	inputSlot, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,		inputSlot, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
				};
			}
		};
	private:
		bool wasInitialized = false;
			
		// This mutable modifier is for modify to const the draw methods
		mutable Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer;
	public:
		/// <summary>
		/// Returns false if the initialization is failed.<para></para>
		/// If set nullptr to "pDevice", I will use library's device.
		/// </summary>
		bool Init( ID3D11Device *pDevice = nullptr );
	public:
	#pragma region Normal
		/// <summary>
		/// Drawing size is sprite size.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.
		/// </summary>
		bool Draw
		(
			ID3D11ShaderResourceView *pSRV,
			const Donya::Vector2 &screenSpacePosition,
			float degree = 0.0f,
			float alpha  = 1.0f,
			const Donya::Vector2 &unitOriginCoord = { 0.0f, 0.0f }
		) const;
		/// <summary>
		/// Drawing size is sprite size.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.
		/// </summary>
		bool DrawExt
		(
			ID3D11ShaderResourceView *pSRV,
			const Donya::Vector2 &screenSpacePosition,
			const Donya::Vector2 &screenSpaceDrawScale,
			float degree = 0.0f,
			const Donya::Vector4 &colorRGBA = { 1.0f, 1.0f, 1.0f, 1.0f },
			const Donya::Vector2 &unitOriginCoord = { 0.0f, 0.0f }
		) const;
	#pragma endregion

	#pragma region Stretched
		/// <summary>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.
		/// </summary>
		bool DrawStretched
		(
			ID3D11ShaderResourceView *pSRV,
			const Donya::Vector2 &screenSpacePosition,
			const Donya::Vector2 &screenSpaceWholeSize,
			float degree = 0.0f,
			float alpha  = 1.0f,
			const Donya::Vector2 &unitOriginCoord = { 0.0f, 0.0f }
		) const;
		/// <summary>
		/// Texture origin is left-top(0, 0), using whole size.
		/// </summary>
		bool DrawStretchedExt
		(
			ID3D11ShaderResourceView *pSRV,
			const Donya::Vector2 &screenSpacePosition,
			const Donya::Vector2 &screenSpaceWholeSize,
			const Donya::Vector2 &screenSpaceDrawScale,
			float degree = 0.0f,
			const Donya::Vector4 &colorRGBA = { 1.0f, 1.0f, 1.0f, 1.0f },
			const Donya::Vector2 &unitOriginCoord = { 0.0f, 0.0f }
		) const;
	#pragma endregion

	#pragma region Part
		/// <summary>
		/// Drawing size is specified texture size.<para></para>
		/// Colors are 1.0f.
		/// </summary>
		bool DrawPart
		(
			ID3D11ShaderResourceView *pSRV,
			const Donya::Vector2 &screenSpacePosition,
			const Donya::Vector2 &textureOriginLeftTop,
			const Donya::Vector2 &textureWholeSize,
			float degree = 0.0f,
			float alpha  = 1.0f,
			const Donya::Vector2 &unitOriginCoord = { 0.0f, 0.0f }
		) const;
		/// <summary>
		/// Drawing size is specified texture size.
		/// </summary>
		bool DrawPartExt
		(
			ID3D11ShaderResourceView *pSRV,
			const Donya::Vector2 &screenSpacePosition,
			const Donya::Vector2 &textureOriginLeftTop,
			const Donya::Vector2 &textureWholeSize,
			const Donya::Vector2 &screenSpaceDrawScale,
			float degree = 0.0f,
			const Donya::Vector4 &colorRGBA = { 1.0f, 1.0f, 1.0f, 1.0f },
			const Donya::Vector2 &unitOriginCoord = { 0.0f, 0.0f }
		) const;
	#pragma endregion

	#pragma region General
		/// <summary>
		/// Colors are 1.0f.
		/// </summary>
		bool DrawGeneral
		(
			ID3D11ShaderResourceView *pSRV,
			const Donya::Vector2 &screenSpacePosition,
			const Donya::Vector2 &screenSpaceWholeSize,
			const Donya::Vector2 &textureOriginLeftTop,
			const Donya::Vector2 &textureWholeSize,
			float degree = 0.0f,
			float alpha  = 1.0f,
			const Donya::Vector2 &unitOriginCoord = { 0.0f, 0.0f }
		) const;
		bool DrawGeneralExt
		(
			ID3D11ShaderResourceView *pSRV,
			const Donya::Vector2 &screenSpacePosition,
			const Donya::Vector2 &screenSpaceWholeSize,
			const Donya::Vector2 &textureOriginLeftTop,
			const Donya::Vector2 &textureWholeSize,
			const Donya::Vector2 &screenSpaceDrawScale,
			float degree = 0.0f,
			const Donya::Vector4 &colorRGBA = { 1.0f, 1.0f, 1.0f, 1.0f },
			const Donya::Vector2 &unitOriginCoord = { 0.0f, 0.0f }
		) const;
	#pragma endregion
	private:
		/// <summary>
		/// Use for constant value at draw methods.
		/// </summary>
		struct Default
		{
			Donya::Vector2 scale{ 1.0f, 1.0f };
			Donya::Vector3 color{ 1.0f, 1.0f, 1.0f };
		public:
			constexpr Default() = default;
		public:
			constexpr Donya::Vector4 MakeColor( float alpha ) const
			{
				return Donya::Vector4{ color, alpha };
			}
		};
		static constexpr Default defaultConfig{};
	private:
		/// <summary>
		/// Returns whole size.
		/// </summary>
		Donya::Int2		FetchTextureSize ( ID3D11ShaderResourceView *pSRV ) const;
		/// <summary>
		/// Returns whole size.
		/// </summary>
		Donya::Vector2	FetchTextureSizeF( ID3D11ShaderResourceView *pSRV ) const;

		std::array<Displayer::Vertex, 4U> MakeNDCVertices
		(
			const Donya::Vector2 &originalTextureWholeSize,
			const Donya::Vector2 &screenSpacePosition,
			const Donya::Vector2 &screenSpaceWholeSize,
			const Donya::Vector2 &textureOriginLeftTop,
			const Donya::Vector2 &textureWholeSize,
			float degree,
			const Donya::Vector4 &colorRGBA,
			const Donya::Vector2 &unitOriginCoord
		) const;
		/// <summary>
		/// If the mapping is failed, returns false.
		/// </summary>
		bool MapVertices( const std::array<Displayer::Vertex, 4U> &NDCTransformedVertices, ID3D11DeviceContext *pImmediateContext = nullptr ) const;
	};
}
