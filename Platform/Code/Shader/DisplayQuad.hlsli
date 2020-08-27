struct VS_IN
{
	float4	pos			: POSITION;
	float4	color		: COLOR;
	float2	texCoord	: TEXCOORD;
};
struct VS_OUT
{
	float4	pos			: SV_POSITION;
	float4	color		: COLOR;
	float2	texCoord	: TEXCOORD;
};
