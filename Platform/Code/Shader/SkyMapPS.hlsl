#include "SkyMap.hlsli"

TextureCube		skyMap		: register( t0 );
SamplerState	skySampler	: register( s0 );

float4 main( VS_OUT pin ) : SV_TARGET
{
	const  float3  skyColor = skyMap.Sample( skySampler, pin.texCoord ).rgb;
	return float4( skyColor * cbDrawColor.rgb, cbDrawColor.a );
}