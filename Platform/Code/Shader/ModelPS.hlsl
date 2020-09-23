#include "Model.hlsli"
#include "Techniques.hlsli"

cbuffer CBPerSubset : register( b3 )
{
	float4		cbAmbient;
	float4		cbDiffuse;
	float4		cbSpecular;	// W is Shininess
	float4		cbEmissive;
};
	
cbuffer CBForPointLiht : register( b5 )
{
	PointLight	cbPointLights[MAX_POINT_LIGHT_COUNT];
	uint		cbPointLightCount;
	uint3		cb_paddings;
};

Texture2D		diffuseMap			: register( t0 );
SamplerState	diffuseMapSampler	: register( s0 );
Texture2D		normalMap			: register( t1 );
SamplerState	normalMapSampler	: register( s1 );
Texture2D		shadowMap			: register( t2 );
SamplerState	shadowMapSampler	: register( s2 );

float4 main( VS_OUT pin ) : SV_TARGET
{
			pin.tsLightVec	= normalize( pin.tsLightVec	);
			pin.tsEyeVec	= normalize( pin.tsEyeVec	);

	float4	normalMapColor	= normalMap.Sample( normalMapSampler, pin.texCoord );
			normalMapColor	= SRGBToLinear( normalMapColor );
	float3	tsNormal		= normalize( SampledToNormal( normalMapColor.xyz ) );
	
	float4	diffuseMapColor	= diffuseMap.Sample( diffuseMapSampler, pin.texCoord );
			diffuseMapColor	= SRGBToLinear( diffuseMapColor );
	float	diffuseMapAlpha	= diffuseMapColor.a;

	float3	totalLight		= CalcLightInfluence
							(
								cbDirLight.light, pin.tsLightVec.xyz,
								tsNormal, pin.tsEyeVec.xyz,
								cbAmbient, cbDiffuse.rgb, cbSpecular.rgb, cbSpecular.w
							);
	for ( uint i = 0; i < cbPointLightCount; ++i )
	{
		// TODO: Align the space between the cbPointLights and the tsNormal, tsEyeVec
			totalLight += CalcPointLightInfl
			(
				cbPointLights[i],
				pin.wsPos.xyz, tsNormal.xyz, pin.tsEyeVec.xyz,
				cbAmbient, cbDiffuse.rgb, cbSpecular.rgb, cbSpecular.w
			);
	}
			totalLight		+= cbAmbient.rgb * cbAmbient.w;

	float3	resultColor		= diffuseMapColor.rgb * totalLight * cbDrawColor.rgb;
	
	float	pixelDepth		= pin.lssPosNDC.z - cbShadowBias;
	float	shadowMapDepth	= shadowMap.Sample( shadowMapSampler, pin.shadowMapUV ).r;
	float	shadowFactor	= CalcShadowFactor( pixelDepth, shadowMapDepth );
	
			resultColor		= lerp( cbShadowColor.rgb, resultColor, shadowFactor );
	
	float4	outputColor		= float4( resultColor, diffuseMapAlpha * cbDrawColor.a );
			outputColor.rgb	+= cbEmissive.rgb * cbEmissive.a;

	return	LinearToSRGB( outputColor );
}
