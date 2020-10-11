#pragma once

#include <string>

#include "Effekseer.h"

#include "../Donya/Vector.h"

#include "EffectKind.h"

namespace Effect
{
	static	Effekseer::Vector2D	ToFxVector( const Donya::Vector2   &v ) { return Effekseer::Vector2D{ v.x, v.y }; }
	static	Effekseer::Vector3D	ToFxVector( const Donya::Vector3   &v ) { return Effekseer::Vector3D{ v.x, v.y, v.z }; }
			Effekseer::Matrix44	ToFxMatrix( const Donya::Vector4x4 &m );

	static	Donya::Vector2		ToVector( const Effekseer::Vector2D &fx ) { return Donya::Vector2{ fx.X, fx.Y }; }
	static	Donya::Vector3		ToVector( const Effekseer::Vector3D &fx ) { return Donya::Vector3{ fx.X, fx.Y, fx.Z }; }
			Donya::Vector4x4	ToMatrix( const Effekseer::Matrix44 &fx );

	/// <summary>
	/// RGBA values will be clamped into [0.0f ~ 1.0f].
	/// </summary>
	Effekseer::Color	ToFxColor	( const Donya::Vector4   &unitRGBA );
	Donya::Vector4		ToColor		( const Effekseer::Color &fxColor  );

	using stdEfkString = std::basic_string<EFK_CHAR>;

	stdEfkString ToEfkString( const std::string  &str );
	stdEfkString ToEfkString( const std::wstring &str );
	std::string  ToString	( const stdEfkString &str );
	std::wstring ToWString	( const stdEfkString &str );

	stdEfkString GetEffectPath( Effect::Kind kind );
}
