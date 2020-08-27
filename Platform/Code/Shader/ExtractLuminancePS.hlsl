#include "DisplayQuad.hlsli"
#include "Techniques.hlsli"

cbuffer CBuffer : register( b0 )
{
	float	cbLuminanceThreshold;
	float3	cbPaddings;
};

Texture2D		texScene		: register( t0 );
SamplerState	spriteSampler	: register( s0 );
float4 main( VS_OUT pin ) : SV_TARGET
{
	float4	pixelColor		= texScene.Sample( spriteSampler, pin.texCoord );
	float	luminance		= CalcLuminance( pixelColor.rgb );
	float	shouldOutput	= step( cbLuminanceThreshold, luminance );
	float3	result			= pixelColor.rgb * shouldOutput;
	return	float4( result.r, result.g, result.b, 1.0f );
}
