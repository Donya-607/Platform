#include "DisplayQuad.hlsli"

Texture2D		blurTextures[4]	: register( t0 );
SamplerState	spriteSampler	: register( s0 );
float4 main( VS_OUT pin ) : SV_TARGET
{
	float3	sum =  ( float3 )( 0 );
			sum += blurTextures[0].Sample( spriteSampler, pin.texCoord ).rgb;
			sum += blurTextures[1].Sample( spriteSampler, pin.texCoord ).rgb;
			sum += blurTextures[2].Sample( spriteSampler, pin.texCoord ).rgb;
			sum += blurTextures[3].Sample( spriteSampler, pin.texCoord ).rgb;
			sum *= 0.25f; // Take average. sum /= 4.0f
	return	float4( sum, 1.0f );
}
