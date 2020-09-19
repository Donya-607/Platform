struct VS_IN
{
	float4		pos			: POSITION;		// Local space
};
struct VS_OUT
{
	float4		svPos		: SV_POSITION;
	float3		texCoord	: TEXCOORD;
};

cbuffer CBuffer : register( b0 )
{
	float4		cbDrawColor;	// RGBA
	row_major
	float4x4	cbWVP;			// World-View-Projection matrix
};
