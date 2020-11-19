#include "Model.hlsli"
#include "Techniques.hlsli"

cbuffer CBPerSubset : register( b3 )
{
	float4		cbAmbient;
	float4		cbDiffuse;
	float4		cbSpecular;	// W is Shininess
	float4		cbEmissive;
};
	
cbuffer CBForPointLight : register( b5 )
{
	PointLight	cbPointLights[MAX_POINT_LIGHT_COUNT];
	uint		cbPointLightCount;
	uint3		cb_paddings_b5;
};

cbuffer CBForVoxelize : register( b6 )
{
	float		cbVoxelSize		= 1.0f;
	float		cbEdgeSize		= 0.0f;	// 0.0f ~ 1.0f
	float		cbEdgeDarkness	= 1.0f;	// 0.0f ~ 1.0f
	float		cb_paddings_b6;
};

float CalcVoxelEdgeBrightness( in float3 pos )
{
	const float3 axes[3] =
	{
		float3( 1.0f, 0.0f, 0.0f ),
		float3( 0.0f, 1.0f, 0.0f ),
		float3( 0.0f, 0.0f, 1.0f )
	};

	float	edgeBrightness = 1.0f;
	
	float3	projPos		= float3( 0.0f, 0.0f, 0.0f );
	float	t			= 0.0f;
	float	near		= 0.0;
	float	far			= 0.0;
	
	float	insideCount	= 0;
	[unroll]
	for ( uint i = 0; i < 3; ++i )
	{
		projPos	=  abs( axes[i] * dot( pos, axes[i] ) );
		t		=  fmod( length( projPos ), cbVoxelSize );
		t		/= cbVoxelSize; // Normalize
		
		near	=  step( t, cbEdgeSize );
		far		=  step( 1.0f - cbEdgeSize, t );
		
		// if ( !near && !far ) { insideCount++; }
		insideCount += step( near + far, 0.1f );
	}

	// return ( 2 <= insideCount ) ? 1.0f : cbEdgeDarkness;
	return lerp( cbEdgeDarkness, 1.0f, step( 2, insideCount ) );
}

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
	float	diffuseAlpha	= diffuseMapColor.a * cbDrawColor.a;
	clip(	diffuseAlpha - 0.001f ); // Also discard the 0.0f


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

	float3	resultColor		=  diffuseMapColor.rgb * totalLight * cbDrawColor.rgb;
			resultColor		*= CalcVoxelEdgeBrightness( pin.msPos );
	
	float	pixelDepth		=  pin.lssPosNDC.z - cbShadowBias;
	float	shadowMapDepth	=  shadowMap.Sample( shadowMapSampler, pin.shadowMapUV ).r;
	float	shadowFactor	=  CalcShadowFactor( pixelDepth, shadowMapDepth );
	
			resultColor		=  lerp( cbShadowColor.rgb, resultColor, shadowFactor );
	
	float4	outputColor		=  float4( resultColor, diffuseAlpha );
			outputColor.rgb	+= cbEmissive.rgb * cbEmissive.a;

	return	LinearToSRGB( outputColor );
}
