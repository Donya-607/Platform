#include "DisplayQuad.hlsli"
#include "Techniques.hlsli"

static const uint maxBlurParamCount = 16;

struct BlurParam
{
	float2		uvOffset;
	float		distribution;
	float		_paddings;
};
cbuffer CBuffer : register( b0 )
{
	BlurParam	cbBlurParams[maxBlurParamCount];
	uint		cbSampleCount;
	uint3		cb_paddings;
};

Texture2D		sourceTexture	: register( t0 );
SamplerState	spriteSampler	: register( s0 );
float4 main( VS_OUT pin ) : SV_TARGET
{
	const uint sampleCount = min( cbSampleCount, maxBlurParamCount );
	
	float4 result = ( float4 )( 0 );
	
	[unroll]
	for ( uint i = 0; i < sampleCount; ++i )
	{
		result += cbBlurParams[i].distribution * sourceTexture.Sample
		(
			spriteSampler,
			pin.texCoord + cbBlurParams[i].uvOffset
		);
	}
	result.w = 1.0f;
	
	return result;
}
