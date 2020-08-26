struct VS_OUT
{
	float4		svPos		: SV_POSITION;
	float4		lssPosNDC	: POSITION;		// Light-source space, NDC
};

struct Light
{
	float4		diffuseColor;	// W is Intensity
	float4		specularColor;	// W is Intensity
};
struct DirectionalLight
{
	Light		light;
	float4		direction;
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
// cbuffer( b4 ) is there at PS
