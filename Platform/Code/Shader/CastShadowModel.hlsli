#include "Struct.hlsli"

struct VS_OUT
{
	float4		svPos		: SV_POSITION;
	float4		lssPosNDC	: POSITION;		// Light-source space, NDC
};

cbuffer CBPerScene : register( b0 )
{
	DirectionalLight cbDirLight;
	float4		cbEyePosition;
	row_major
	float4x4	cbView;
	row_major
	float4x4	cbViewProj;
};

cbuffer CBPerModel : register( b1 )
{
	float4		cbDrawColor;
	row_major
	float4x4	cbWorld;
	float2		cbUVOrigin;
	float2		_padding_cbpermodel;
};

// cbuffer( b2 ) is there at VS
// cbuffer( b3 ) is there at PS
// cbuffer( b4 ) is there at PS
