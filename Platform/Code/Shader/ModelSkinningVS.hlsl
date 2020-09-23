#include "Model.hlsli"
#include "Techniques.hlsli"

struct VS_IN
{
	float4	pos			: POSITION;
	float4	normal		: NORMAL;
	float4	tangent		: TANGENT;
	float2	texCoord	: TEXCOORD0;
	float4	weights		: WEIGHTS;
	uint4	bones		: BONES;
};

static const uint MAX_BONE_COUNT = 64U;
cbuffer CBPerMesh : register( b2 )
{
	row_major
	float4x4	cbAdjustMatrix;
	row_major
	float4x4	cbBoneTransforms[MAX_BONE_COUNT];
};

void ApplyBoneMatrices( float4 boneWeights, uint4 boneIndices, inout float4 inoutPosition, inout float4 inoutNormal, inout float4 inoutTangent )
{
	const float4 inPosition	= float4( inoutPosition.xyz, 1.0f );
	const float3 inNormal	= inoutNormal.xyz;
	const float3 inTangent	= inoutTangent.xyz;
	float3 resultPos		= { 0, 0, 0 };
	float3 resultNormal		= { 0, 0, 0 };
	float3 resultTangent	= { 0, 0, 0 };
	float  weight			= 0;
	row_major float4x4 transform4D = 0;
	row_major float3x3 transform3D = 0;
	for ( int i = 0; i < 4/* float4 */; ++i )
	{
		weight			= boneWeights[i];
		transform4D		= cbBoneTransforms[boneIndices[i]];
		transform3D		= ( float3x3 )( transform4D );
		resultPos		+= ( weight * mul( inPosition,	transform4D ) ).xyz;
		resultNormal	+= ( weight * mul( inNormal,	transform3D ) );
		resultTangent	+= ( weight * mul( inTangent,	transform3D ) );
	}
	inoutPosition	= float4( resultPos,		1.0f );
	inoutNormal		= float4( resultNormal,		0.0f );
	inoutTangent	= float4( resultTangent,	0.0f );
}

VS_OUT main( VS_IN vin )
{
	vin.pos.w			=  1.0f;
	vin.normal.w		=  0.0f;
	vin.tangent.w		=  0.0f;
	ApplyBoneMatrices( vin.weights, vin.bones, vin.pos, vin.normal, vin.tangent );

	float4x4 W			=  mul( cbAdjustMatrix, cbWorld );
	float4x4 WV			=  mul( W, cbView		);
	float4x4 WVP		=  mul( W, cbViewProj	);
	float4x4 WLP		=  mul( W, cbLightProj	);
	float3   vsNormal	=  normalize( mul( vin.normal,  WV ).xyz );
	float3   vsTangent	=  normalize( mul( vin.tangent, WV ).xyz );
	float4x4 VT			=  mul( cbView, MakeMatrixToTangentSpace( vsTangent, vsNormal ) );

	VS_OUT vout			=  ( VS_OUT )( 0 );
	vout.wsPos			=  mul( vin.pos, W );
	vout.svPos			=  mul( vin.pos, WVP );
	vout.lssPosNDC		=  mul( vin.pos, WLP );
	vout.lssPosNDC		/= vout.lssPosNDC.w; // Clip space to NDC
	vout.tsLightVec		=  normalize( mul( -cbDirLight.direction, VT ) );
	vout.tsEyeVec		=  normalize( mul( cbEyePosition - vout.wsPos, VT ) );
	vout.texCoord		=  vin.texCoord;
	vout.shadowMapUV	=  NDCToTexCoord( vout.lssPosNDC.xy );
	return vout;
}
