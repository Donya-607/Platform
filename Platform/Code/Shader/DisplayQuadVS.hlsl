#include "DisplayQuad.hlsli"

VS_OUT main( VS_IN vin )
{
	VS_OUT vout		= ( VS_OUT )( 0 );
	vout.pos		= vin.pos;
	vout.color		= vin.color;
	vout.texCoord	= vin.texCoord;
	return vout;
}
