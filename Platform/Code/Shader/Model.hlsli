#include "Struct.hlsli"

struct VS_OUT
{
	float4		svPos		: SV_POSITION;
	float4		wsPos		: POSITION0;	// World space
	float4		lssPosNDC	: POSITION1;	// Light-source space, NDC
	float4		normal		: NORMAL;
	float2		texCoord	: TEXCOORD0;
	float2		shadowMapUV	: TEXCOORD1;
};

cbuffer CBPerScene : register( b0 )
{
	DirectionalLight cbDirLight;
	float4		cbEyePosition;
	row_major
	float4x4	cbViewProj;
};

cbuffer CBPerModel : register( b1 )
{
	float4		cbDrawColor;
	row_major
	float4x4	cbWorld;
};

// cbuffer( b2 ) is there at VS
// cbuffer( b3 ) is there at PS

cbuffer CBForShadow : register( b4 )
{
	row_major
	float4x4	cbLightProj;	// View-Projection of light-source
	float3		cbShadowColor;	// RGB
	float		cbShadowBias;
};

// cbuffer( b5 ) is there at PS
