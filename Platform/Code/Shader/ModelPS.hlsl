#include "Model.hlsli"
#include "Techniques.hlsli"

cbuffer CBPerSubset : register( b3 )
{
	float4		cbAmbient;
	float4		cbDiffuse;
	float4		cbSpecular;		// W is Shininess
};
	
cbuffer CBForPointLiht : register( b5 )
{
	PointLight	cbPointLights[MAX_POINT_LIGHT_COUNT];
	uint		cbPointLightCount;
	uint3		cb_paddings;
};

Texture2D		diffuseMap			: register( t0 );
SamplerState	diffuseMapSampler	: register( s0 );
Texture2D		shadowMap			: register( t1 );
SamplerState	shadowMapSampler	: register( s1 );

float4 main( VS_OUT pin ) : SV_TARGET
{
			pin.normal		= normalize( pin.normal );
			
	float3	nLightVec		= normalize( -cbDirLight.direction.xyz );	// Vector from position.
	float4	nEyeVector		= normalize( cbEyePosition - pin.wsPos );	// Vector from position.

	float4	diffuseMapColor	= diffuseMap.Sample( diffuseMapSampler, pin.texCoord );
			diffuseMapColor	= SRGBToLinear( diffuseMapColor );
	float	diffuseMapAlpha	= diffuseMapColor.a;

	float3	totalLight		= CalcLightInfluence
							(
								cbDirLight.light, nLightVec,
								pin.normal.xyz, nEyeVector.xyz,
								cbDiffuse.rgb, cbSpecular.rgb, cbSpecular.w
							);
	for ( uint i = 0; i < cbPointLightCount; ++i )
	{
			totalLight += CalcPointLightInfl
			(
				cbPointLights[i],
				pin.wsPos.xyz, pin.normal.xyz, nEyeVector.xyz,
				cbDiffuse.rgb, cbSpecular.rgb, cbSpecular.w
			);
	}
			totalLight		+= cbAmbient.rgb * cbAmbient.w;

	float3	resultColor		= diffuseMapColor.rgb * totalLight * cbDrawColor.rgb;
	
	float	pixelDepth		= pin.lssPosNDC.z - cbShadowBias;
	float	shadowMapDepth	= shadowMap.Sample( shadowMapSampler, pin.shadowMapUV ).r;
	float	shadowFactor	= CalcShadowFactor( pixelDepth, shadowMapDepth );
	
			resultColor		= lerp( cbShadowColor.rgb, resultColor, shadowFactor );
	
	float4	outputColor		= float4( resultColor, diffuseMapAlpha * cbDrawColor.a );

	return	LinearToSRGB( outputColor );
}
