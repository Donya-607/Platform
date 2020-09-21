#pragma once

#include <array>
#include <wrl.h>

#include "Donya/CBuffer.h"
#include "Donya/Shader.h"
#include "Donya/Vector.h"

/// <summary>
/// Draw a sky by cube map
/// </summary>
class SkyMap
{
public:
	struct Vertex
	{
		Donya::Vector3		pos;
	public:
		static constexpr const auto GenerateInputElements( UINT inputSlot )
		{
			return std::array<D3D11_INPUT_ELEMENT_DESC, 1>
			{
				D3D11_INPUT_ELEMENT_DESC{ "POSITION" , 0, DXGI_FORMAT_R32G32B32_FLOAT, inputSlot, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
		}
	};
	struct Constant
	{
		Donya::Vector4		drawColor;
		Donya::Vector4x4	matWVP;
	};
private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	pSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				pBufferPos;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				pBufferIdx;
	Donya::CBuffer<Constant>							cbuffer;
	Donya::VertexShader									VS;
	Donya::PixelShader									PS;
	size_t												indexCount  = 0;
	bool												initialized = false;
public:
	bool Init();
	void UpdateConstant( const Constant &data );
	void UpdateConstant( const Donya::Vector4x4 &matWVP, const Donya::Vector4 &drawColor );
	void Draw() const;
private:
	bool CreateCubeMap();
	bool CreateBuffers();
	bool CreateShaders();
};
