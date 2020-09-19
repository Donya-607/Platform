#include "SkyMap.hlsli"

VS_OUT main( VS_IN vin )
{
	VS_OUT vout		= ( VS_OUT )( 0 );
	vout.svPos		= mul( vin.pos, cbWVP );
	vout.texCoord	= normalize( vin.pos.xyz );
	return vout;
}