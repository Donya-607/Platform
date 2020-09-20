#include "SkyMap.hlsli"

VS_OUT main( VS_IN vin )
{
	VS_OUT vout		= ( VS_OUT )( 0 );
	vout.svPos		= mul( vin.pos, cbWVP ).xyww; // Specify 1.0f to the Z component for the sky position always places farest from camera.
	vout.texCoord	= normalize( vin.pos.xyz );
	return vout;
}