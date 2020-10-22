#include "SkyMap.h"

#include "Donya/Direct3DUtil.h"
#include "Donya/ModelPrimitive.h"	// Use CreateCube()
#include "Donya/RenderingStates.h"
#include "Donya/Resource.h"
#include "Donya/Useful.h"			// Use OutputDebugStr()

namespace
{
	constexpr const wchar_t	*skyMapPath = L"./Data/Images/BG/SkyMap/SkyMap.dds";
	constexpr const char	*VSFilePath = "./Data/Shaders/SkyMapVS.cso";
	constexpr const char	*PSFilePath = "./Data/Shaders/SkyMapPS.cso";
}

bool SkyMap::Init()
{
	if ( initialized ) { return true; }
	// else

	bool succeeded = true;
	if ( !CreateCubeMap() ) { succeeded = false; }
	if ( !CreateBuffers() ) { succeeded = false; }
	if ( !CreateShaders() ) { succeeded = false; }

	if ( !succeeded ) { return false; }
	// else

	initialized = true;
	return true;
}

void SkyMap::UpdateConstant( const Constant &data )
{
	cbuffer.data = data;
}
void SkyMap::UpdateConstant( const Donya::Vector4x4 &matWorld, const Donya::Vector4 &drawColor )
{
	cbuffer.data.drawColor	= drawColor;
	cbuffer.data.matWVP		= matWorld;
}

void SkyMap::Draw() const
{
	if ( !initialized )
	{
		_ASSERT_EXPR( 0, L"Error: [SkyMap] was not initialized!" );
		return;
	}
	// else

	ID3D11DeviceContext *pContext = Donya::GetImmediateContext();

	constexpr UINT stride = sizeof( Vertex );
	constexpr UINT offset = 0;
	pContext->IASetVertexBuffers
	(
		0U, 1U,
		pBufferPos.GetAddressOf(),
		&stride,
		&offset
	);
	pContext->IASetIndexBuffer( pBufferIdx.Get(), DXGI_FORMAT_R32_UINT, 0 );
	pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	Donya::DepthStencil::Activate( Donya::DepthStencil::Defined::NoTest_NoWrite, pContext );
	Donya::Rasterizer::Activate( Donya::Rasterizer::Defined::Solid_CullNone_NoClip, pContext );
	Donya::Sampler::SetPS( Donya::Sampler::Defined::Aniso_Clamp, 0U, pContext );

	VS.Activate( pContext );
	PS.Activate( pContext );
	cbuffer.Activate( 0U, /* setVS = */ true, /* setVS = */ true, pContext );

	pContext->PSSetShaderResources( 0U, 1U, pSRV.GetAddressOf() );

	pContext->DrawIndexed( indexCount, 0U, 0U );

	ID3D11ShaderResourceView *pNullSRV = nullptr;
	pContext->PSSetShaderResources( 0U, 1U, &pNullSRV );

	cbuffer.Deactivate( pContext );
	PS.Deactivate( pContext );
	VS.Deactivate( pContext );

	Donya::Sampler::ResetPS( 0U, pContext );
	Donya::Rasterizer::Deactivate( pContext );
	Donya::DepthStencil::Deactivate( pContext );
}

bool SkyMap::CreateCubeMap()
{
	const bool result = Donya::Resource::CreateTextureFromFile
	(
		Donya::GetDevice(),
		skyMapPath,
		pSRV.GetAddressOf()
	);

	return result;
}
bool SkyMap::CreateBuffers()
{
	if ( !cbuffer.Create() ) { return false; }
	// else

	HRESULT	hr			= S_OK;
	bool	succeeded	= true;

	constexpr size_t sliceH = 12U;
	constexpr size_t sliceV = 6U;
	constexpr float  radius = 1.0f;
	// const auto geometry = Donya::Model::Geometry::CreateSphere( sliceH, sliceV, radius );
	const auto geometry = Donya::Model::Geometry::CreateCube( radius );

	// The create functions requires a resource as std::vector
	const size_t posCount = geometry.vertices.size();
	std::vector<size_t> indices { geometry.indices.begin(), geometry.indices.end() };
	std::vector<Vertex> vertices{ posCount }; // Extract positions only
	for ( size_t i = 0; i < posCount; ++i )
	{
		vertices[i].pos = geometry.vertices[i].position;
	}

	hr = Donya::CreateVertexBuffer<Vertex>( Donya::GetDevice(), vertices, D3D11_USAGE_IMMUTABLE, NULL, pBufferPos.GetAddressOf() );
	if ( FAILED( hr ) )
	{
		constexpr const wchar_t *errMsg = L"Failed: Create Vertex buffer in SkyMap";
		Donya::OutputDebugStr( errMsg ); _ASSERT_EXPR( 0, errMsg );
		succeeded = false;
	}
	
	hr = Donya::CreateIndexBuffer( Donya::GetDevice(), indices, pBufferIdx.GetAddressOf() );
	if ( FAILED( hr ) )
	{
		constexpr const wchar_t *errMsg = L"Failed: Create Index buffer in SkyMap";
		Donya::OutputDebugStr( errMsg ); _ASSERT_EXPR( 0, errMsg );
		succeeded = false;
	}

	indexCount = indices.size();
	return succeeded;
}
bool SkyMap::CreateShaders()
{
	constexpr auto descs = Vertex::GenerateInputElements( 0 );
	const std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescs{ descs.begin(), descs.end() };

	bool succeeded = true;
	if ( !VS.CreateByCSO( VSFilePath, IEDescs	) ) { succeeded = false; }
	if ( !PS.CreateByCSO( PSFilePath			) ) { succeeded = false; }

	return succeeded;
}
