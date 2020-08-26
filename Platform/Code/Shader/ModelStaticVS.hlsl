#include "Model.hlsli"
#include "Techniques.hlsli"

struct VS_IN
{
	float4 pos		: POSITION;
	float4 normal	: NORMAL;
	float2 texCoord	: TEXCOORD0;
};

cbuffer CBPerMesh : register( b2 )
{
	row_major
	float4x4	cbAdjustMatrix;
};

VS_OUT main( VS_IN vin )
{
	vin.pos.w			=  1.0f;
	vin.normal.w		=  0.0f;

	float4x4 W			=  mul( cbAdjustMatrix, cbWorld );
	float4x4 WVP		=  mul( W, cbViewProj  );
	float4x4 WLP		=  mul( W, cbLightProj );

	VS_OUT vout			=  ( VS_OUT )( 0 );
	vout.wsPos			=  mul( vin.pos, W );
	vout.svPos			=  mul( vin.pos, WVP );
	vout.lssPosNDC		=  mul( vin.pos, WLP );
	vout.lssPosNDC		/= vout.lssPosNDC.w; // Clip space to NDC
	vout.normal			=  normalize( mul( vin.normal, W ) );
	vout.texCoord		=  vin.texCoord;
	vout.shadowMapUV	=  NDCToTexCoord( vout.lssPosNDC.xy );
	return vout;
}
