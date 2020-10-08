#pragma once

#include <string>

#include "Effekseer.h"

#include "../Donya/Vector.h"

#include "EffectKind.h"

namespace Effect
{
	static Effekseer::Vector2D	ToFxVector( const Donya::Vector2 &v ) { return Effekseer::Vector2D{ v.x, v.y }; }
	static Effekseer::Vector3D	ToFxVector( const Donya::Vector3 &v ) { return Effekseer::Vector3D{ v.x, v.y, v.z }; }
	static Effekseer::Matrix44	ToFxMatrix( const Donya::Vector4x4 &m )
	{
		Effekseer::Matrix44 fx{};

		for ( int row = 0; row < 4; ++row )
		{
			for ( int column = 0; column < 4; ++column )
			{
				fx.Values[row][column] = m.m[row][column];
			}
		}

		return fx;
	}

	static Donya::Vector2		ToVector( const Effekseer::Vector2D &fx ) { return Donya::Vector2{ fx.X, fx.Y }; }
	static Donya::Vector3		ToVector( const Effekseer::Vector3D &fx ) { return Donya::Vector3{ fx.X, fx.Y, fx.Z }; }
	static Donya::Vector4x4		ToMatrix( const Effekseer::Matrix44 &fx )
	{
		Donya::Vector4x4 m{};

		for ( int row = 0; row < 4; ++row )
		{
			for ( int column = 0; column < 4; ++column )
			{
				m.m[row][column] = fx.Values[row][column];
			}
		}

		return m;
	}

	std::basic_string<EFK_CHAR> GetEffectPath( Effect::Kind kind );

}
