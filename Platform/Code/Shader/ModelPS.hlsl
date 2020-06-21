#include "Model.hlsli"
#include "Techniques.hlsli"

cbuffer CBPerSubset : register( b3 )
{
	float4	cbAmbient;
	float4	cbDiffuse;
	float4	cbSpecular;
};

cbuffer CBForTransparency : register( b4 )
{
	float cbTransNear;
	float cbTransFar;
	float cbTransLowerAlpha;
	float cbTransHeightThreshold;
};

float CalcTransparency( float3 pixelPos )
{
	float	distance		= length( pixelPos - cbEyePosition.xyz );
	float	percent			= saturate( ( distance - cbTransNear ) / ( cbTransFar - cbTransNear ) );
			clip( percent );
			
	float	biasedPercent	= cbTransLowerAlpha + ( percent * ( 1.0f - cbTransLowerAlpha ) );
	
	// I don't wanna transparentize if the pixel under the threshold.
	float	isUnderThreshold= step( pixelPos.y, cbTransHeightThreshold );
	
	return	min( 1.0f, biasedPercent + isUnderThreshold );
}

float3 CalcLightInfluence( float4 lightColor, float3 nwsPixelToLightVec, float3 nwsPixelNormal, float3 nwsEyeVector )
{
	float3	ambientColor	= cbAmbient.rgb;
	float	diffuseFactor	= HalfLambert( nwsPixelNormal, nwsPixelToLightVec );
	//		diffuseFactor	= pow( diffuseFactor, 2.0f );
	float3	diffuseColor	= cbDiffuse.rgb * diffuseFactor;
	float	specularFactor	= Phong( nwsPixelNormal, nwsPixelToLightVec, nwsEyeVector );
	float3	specularColor	= cbSpecular.rgb * specularFactor * cbSpecular.w;

	float3	Ka				= ambientColor;
	float3	Kd				= diffuseColor;
	float3	Ks				= specularColor;
	float3	light			= lightColor.rgb * lightColor.w;
	return	Ka + ( ( Kd + Ks ) * light );
}

Texture2D		diffuseMap			: register( t0 );
SamplerState	diffuseMapSampler	: register( s0 );

float4 main( VS_OUT pin ) : SV_TARGET
{
			pin.normal		= normalize( pin.normal );
			
	float3	nLightVec		= normalize( -cbDirLight.direction.rgb );	// Vector from position.
	float4	nEyeVector		= cbEyePosition - pin.wsPos;				// Vector from position.

	float4	diffuseMapColor	= diffuseMap.Sample( diffuseMapSampler, pin.texCoord );
			diffuseMapColor	= SRGBToLinear( diffuseMapColor );
	float	diffuseMapAlpha	= diffuseMapColor.a;

	float3	totalLight		= CalcLightInfluence( cbDirLight.color, nLightVec, pin.normal.rgb, nEyeVector.rgb );

	float3	resultColor		= diffuseMapColor.rgb * totalLight;
	float4	outputColor		= float4( resultColor, diffuseMapAlpha );
			outputColor		= outputColor * cbDrawColor;
			outputColor.a	= outputColor.a * CalcTransparency( pin.wsPos.xyz );

	return	LinearToSRGB( outputColor );
}
