#ifndef INCLUDED_DONYA_STRUCT_HLSL_
#define INCLUDED_DONYA_STRUCT_HLSL_

struct Light
{
	float4		diffuseColor;	// W is Intensity
	float4		specularColor;	// W is Intensity
};
struct DirectionalLight
{
	Light		light;
	float4		direction;
};
static const uint MAX_POINT_LIGHT_COUNT = 8;
struct PointLight // See http://ogldev.atspace.co.uk/www/tutorial20/tutorial20.html
{
	Light	light;
	float4	wsPos;
	float3	attenuation;	// [X:Constant][Y:Linear][Z:Exponential]
	float	range;
};

#endif // INCLUDED_DONYA_STRUCT_HLSL_
