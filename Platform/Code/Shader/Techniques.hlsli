#include "Struct.hlsli"

static const float PI		= 3.14159265359f;
static const float EPSILON	= 0.0001f;

// See https://tech.cygames.co.jp/archives/2339/
float4 SRGBToLinear( float4 colorSRGB )
{
	return pow( abs( colorSRGB ), 2.2f );
}
// See https://tech.cygames.co.jp/archives/2339/
float4 LinearToSRGB( float4 colorLinear )
{
	return pow( abs( colorLinear ), 1.0f / 2.2f );
}

// Using way is: https://en.wikipedia.org/wiki/Relative_luminance
float CalcLuminance( float3 linearRGB )
{
	return
	linearRGB.r * 0.2126f +
	linearRGB.g * 0.7152f +
	linearRGB.b * 0.0722f
	;
}

// We consider as the origin of texCoord is left-top.
float2 NDCToTexCoord( float2 NDCPos )
{
	/*
	-1.0f * 0.5f + 0.5f = 0.0f
	 0.0f * 0.5f + 0.5f = 0.5f
	 1.0f * 0.5f + 0.5f = 1.0f
	*/
	
	return float2
	(
		(  NDCPos.x * 0.5f ) + 0.5f,
		( -NDCPos.y * 0.5f ) + 0.5f // Y-plus direction to be down
	);
}

// Calculate diffuse reflection.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Returns : Diffuse factor of normalized in 0.0f ~ 1.0f.
float Lambert( float3 nwsNormal, float3 nwsToLightVec )
{
	return max( 0.0f, dot( nwsNormal, nwsToLightVec ) );
}
// Calculate half-lambert.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Returns : Diffuse factor. 0.5f ~ 1.0f.
float HalfLambert( float3 nwsNormal, float3 nwsToLightVec )
{
	float lambert = dot( nwsNormal, nwsToLightVec );
	return ( lambert * 0.5f ) + 0.5f;
}
// see http://www.project-asura.com/program/d3d11/d3d11_004.html
// Argument.diffuseColor : The diffuse color.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Returns : Diffuse color of according to the energy conservation law.
float3 NormalizedLambert( float3 diffuseColor, float3 nwsNormal, float3 nwsToLightVec )
{
	return diffuseColor * Lambert( nwsNormal, nwsToLightVec ) * ( 1.0f / PI );
}
// NormalizedLambert() of using HalfLambert(). see http://www.project-asura.com/program/d3d11/d3d11_004.html
// Argument.diffuseColor : The diffuse color.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Returns : Diffuse color of according to the energy conservation law.
float3 NormalizedHalfLambert( float3 diffuseColor, float3 nwsNormal, float3 nwsToLightVec )
{
	return diffuseColor * HalfLambert( nwsNormal, nwsToLightVec ) * ( 1.0f / PI );
}

// Calculate specular factor.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Argument.nwsToEyeVec : The eye vector of normalized world space. this vector is "position -> eye".
// Argument.shininess : The specular power. Must be greater than zero.
// Argument.intensity : The specular's intensity. Must be greater than zero.
// Returns : Specular factor.
float Phong( float3 nwsNormal, float3 nwsToLightVec, float3 nwsToEyeVec, float shininess, float intensity )
{
#if 1 // USE_REFLECT
	float3 nwsReflection	= normalize( reflect( -nwsToLightVec, nwsNormal ) );
	float  specularFactor	= max( 0.0f, dot( nwsReflection, nwsToEyeVec ) );
#else
	float3 nwsProjection	= nwsNormal * dot( nwsNormal, nwsToLightVec );
	float3 nwsReflection	= ( nwsProjection * 2.0f ) - nwsToLightVec;
	float  specularFactor	= max( 0.0f, dot( nwsReflection, nwsToEyeVec ) );
#endif // USE_REFLECT
	return max( 0.0f, pow( specularFactor, shininess ) * intensity );
}
// Calculate specular factor by half vector.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Argument.nwsToEyeVec : The eye vector of normalized world space. this vector is "position -> eye".
// Argument.shininess : The specular power. Must be greater than zero.
// Argument.intensity : The specular's intensity. Must be greater than zero.
// Returns : Specular factor.
float BlinnPhong( float3 nwsNormal, float3 nwsToLightVec, float3 nwsToEyeVec, float shininess, float intensity )
{
	float3 nwsHalfVec		= normalize( nwsToEyeVec + nwsToLightVec );
	float  specularFactor	= max( 0.0f, dot( nwsHalfVec, nwsNormal ) );
	return max( 0.0f, pow( specularFactor, shininess ) * intensity );
}

// Calculate color that influenced by light.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToEyeVec : The eye vector of normalized world space. this vector is "position -> eye".
// Argument.diffuse : The diffuse color.
// Argument.specular : The specular color.
// Argument.shininess : The specular power. Must be greater than zero.
// Returns : Color that influenced by light.
float3 CalcLightInfluence( Light light, float3 nwsToLightVec, float3 nwsNormal, float3 nwsEyeVec, float3 diffuse, float3 specular, float shininess )
{
	float	diffuseFactor	= HalfLambert( nwsNormal, nwsToLightVec );
	//		diffuseFactor	= pow( diffuseFactor, 2.0f );
	float3	diffuseColor	= diffuse.rgb * diffuseFactor;
	
	float	specularFactor	= Phong( nwsNormal, nwsToLightVec, nwsEyeVec, shininess, light.specularColor.w );
	float3	specularColor	= specular.rgb * specularFactor;

	float3	Kd				= diffuseColor;
	float3	Id				= light.diffuseColor.rgb * light.diffuseColor.w;
	float3	Ks				= specularColor;
	float3	Is				= light.specularColor.rgb;
	return	( Kd * Id ) + ( Ks * Is );
}
// Calculate color that influenced by point light.
// Argument.wsPixelPos : The pixel position of world space.
// Argument.nwsNormal : Normalized World-space vector of pixel's normal.
// Argument.nwsEyeVec : Normalized World-space vector that is (camera.pos - pixel.pos).
// Argument.diffuse : The diffuse color.
// Argument.specular : The specular color.
// Argument.shininess : The specular power. Must be greater than zero.
// Returns : Color that influenced by light.
float3 CalcPointLightInfl( PointLight plight, const float3 wsPixelPos, const float3 nwsNormal, const float3 nwsEyeVec, float3 diffuse, float3 specular, float shininess )
{	
	// See http://ogldev.atspace.co.uk/www/tutorial20/tutorial20.html
	
	float3	lightVec		= plight.wsPos.xyz - wsPixelPos;
	float	distance		= length( lightVec );
	float3	nLightVec		= normalize( lightVec );
	
	float3	lightColor		= CalcLightInfluence( plight.light, nLightVec, nwsNormal, nwsEyeVec, diffuse, specular, shininess );
	
	float	influence		= pow( 1.0f - saturate( ( plight.range / ( distance + EPSILON ) ) ), 2.0f );
	float	attenuation		= plight.attenuation.x +
							  plight.attenuation.y * distance +
							  plight.attenuation.z * distance * distance;
	
	return	lightColor * influence / ( attenuation + EPSILON );
	// return	lightColor / ( attenuation + EPSILON );
}

// Calculate shadowing percent.
// Argument.lsPixelDepth : Light-source space depth of current pixel.
// Argument.lsShadowMapDepth : Light-source space nearest depth of stored.
// Returns : Shadow factor. 0.0f ~ 1.0f.
float CalcShadowFactor( float lsPixelDepth, float lsShadowMapDepth )
{
	float	shadowColor		= max ( lsPixelDepth, lsShadowMapDepth );
	float	ignoreShadow	= step( lsPixelDepth, lsShadowMapDepth );
	return	saturate( shadowColor + ignoreShadow );
}

// Calculate fog effect.
// Argument.affectColor : The color.
// Argument.wsEyePos : The position of eye(like camera) in world space.
// Argument.wsPos : The position of including "affectColor".
// Argument.wsNear : The near distance in world space. if the distance is less-equal than the near, the fog does not affecting.
// Argument.wsFar : The far distance in world space. if the distance is greater-equal than the far, the returns color will affected completely.
// Argument.fogColor : The color of fog. the affect color.
// Returns : Affected color by fog.
float3 AffectFog( float3 affectColor, float3 wsEyePos, float3 wsPos, float wsNear, float wsFar, float3 fogColor = { 1.0f, 1.0f, 1.0f } )
{
	float distance	= length( wsPos - wsEyePos );
	float alpha		= saturate( ( distance - wsNear ) / ( wsFar - wsNear ) );
	return ( affectColor * ( 1.0f - alpha ) ) + ( fogColor * alpha );
}
