struct VS_OUT
{
	float4		svPos		: SV_POSITION;
	float4		wsPos		: POSITION;
	float4		normal		: NORMAL;
	float2		texCoord	: TEXCOORD0;
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
