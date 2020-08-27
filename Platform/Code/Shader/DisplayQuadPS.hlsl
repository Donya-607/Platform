#include "DisplayQuad.hlsli"

Texture2D		texResource	: register( t0 );
SamplerState	texSampler	: register( s0 );

float4 main( VS_OUT pin ) : SV_TARGET
{
	return texResource.Sample( texSampler, pin.texCoord ) * pin.color;
}
