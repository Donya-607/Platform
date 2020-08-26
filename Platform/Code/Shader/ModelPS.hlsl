#include "Model.hlsli"
#include "Techniques.hlsli"

cbuffer CBPerSubset : register( b3 )
{
	float4		cbAmbient;
	float4		cbDiffuse;
	float4		cbSpecular;		// W is Shininess
};

cbuffer CBForShadow : register( b4 )
{
	row_major
	float4x4	cbLightProj;	// View-Projection of light-source
	float3		cbShadowColor;	// RGB
	float		cbShadowBias;
};

float3 CalcLightInfluence( Light light, float3 nwsPixelToLightVec, float3 nwsPixelNormal, float3 nwsEyeVector )
{
	float	diffuseFactor	= HalfLambert( nwsPixelNormal, nwsPixelToLightVec );
	//		diffuseFactor	= pow( diffuseFactor, 2.0f );
	float3	diffuseColor	= cbDiffuse.rgb * diffuseFactor;
	
	float	specularFactor	= Phong( nwsPixelNormal, nwsPixelToLightVec, nwsEyeVector, cbSpecular.w, light.specularColor.w );
	float3	specularColor	= cbSpecular.rgb * specularFactor;

	float3	Kd				= diffuseColor;
	float3	Id				= light.diffuseColor.rgb * light.diffuseColor.w;
	float3	Ks				= specularColor;
	float3	Is				= light.specularColor.rgb;
	return	( Kd * Id ) + ( Ks * Is );
}

Texture2D		diffuseMap			: register( t0 );
SamplerState	diffuseMapSampler	: register( s0 );
Texture2D		shadowMap			: register( t1 );
SamplerState	shadowMapSampler	: register( s1 );

float4 main( VS_OUT pin ) : SV_TARGET
{
			pin.normal		= normalize( pin.normal );
			
	float3	nLightVec		= normalize( -cbDirLight.direction.rgb );	// Vector from position.
	float4	nEyeVector		= cbEyePosition - pin.wsPos;				// Vector from position.

	float4	diffuseMapColor	= diffuseMap.Sample( diffuseMapSampler, pin.texCoord );
			diffuseMapColor	= SRGBToLinear( diffuseMapColor );
	float	diffuseMapAlpha	= diffuseMapColor.a;

	float3	totalLight		= CalcLightInfluence( cbDirLight.light, nLightVec, pin.normal.rgb, nEyeVector.rgb );
			totalLight		+= cbAmbient.rgb * cbAmbient.w;

	float3	resultColor		= diffuseMapColor.rgb * totalLight * cbDrawColor.rgb;
	
	float	pixelDepth		= pin.lssPosNDC.z - cbShadowBias;
	float	shadowMapDepth	= shadowMap.Sample( shadowMapSampler, pin.shadowMapUV ).r;
	float	shadowFactor	= CalcShadowFactor( pixelDepth, shadowMapDepth );
	
			resultColor		= lerp( cbShadowColor.rgb, resultColor, shadowFactor );
	
	float4	outputColor		= float4( resultColor, diffuseMapAlpha * cbDrawColor.a );

	return	LinearToSRGB( outputColor );
}
