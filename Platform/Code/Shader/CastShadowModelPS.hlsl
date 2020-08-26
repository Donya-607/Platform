#include "CastShadowModel.hlsli"

cbuffer UNUSED_CBPerSubset : register( b3 )
{
	float4	cbAmbient;
	float4	cbDiffuse;
	float4	cbSpecular;
};

float4 main( VS_OUT pin ) : SV_TARGET
{
	const  float   depth = saturate( pin.lssPosNDC.z );
	return float4( depth, depth, depth, 1.0f );
}
