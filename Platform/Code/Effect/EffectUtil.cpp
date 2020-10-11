#include "EffectUtil.h"

#include <iterator>				// Use std::begin()/end()

#include "../Donya/Template.h"	// Use Clamp()

namespace Effect
{
	namespace
	{
		void Convert( float( *output )[4][4], const float( &source )[4][4] )
		{
			for ( int row = 0; row < 4; ++row )
			{
				for ( int column = 0; column < 4; ++column )
				{
					( *output )[row][column] = source[row][column];
				}
			}
		}

		template<typename FromChar, typename ToChar>
		std::basic_string<ToChar> Convert( const std::basic_string<FromChar> &str )
		{
			// See https://dbj.org/c17-codecvt-deprecated-panic/

			if ( str.empty() ) { return {}; }
			// else
		
			return { std::begin( str ), std::end( str ) };
		}
	}

	Effekseer::Matrix44	ToFxMatrix( const Donya::Vector4x4 &m )
	{
		Effekseer::Matrix44 fx;
		Convert( &fx.Values, m.m );
		return fx;
	}
	Donya::Vector4x4	ToMatrix( const Effekseer::Matrix44 &fx )
	{
		Donya::Vector4x4 m{};
		Convert( &m.m, fx.Values );
		return m;
	}

	Effekseer::Color	ToFxColor( const Donya::Vector4 &rgba )
	{
		Effekseer::Color fx;
		using EfkColorT = decltype( fx.R );

		constexpr float normalizer = 255.0f;
		fx.R = static_cast<EfkColorT>( Donya::Clamp( rgba.x, 0.0f, 1.0f ) * normalizer );
		fx.G = static_cast<EfkColorT>( Donya::Clamp( rgba.y, 0.0f, 1.0f ) * normalizer );
		fx.B = static_cast<EfkColorT>( Donya::Clamp( rgba.z, 0.0f, 1.0f ) * normalizer );
		fx.A = static_cast<EfkColorT>( Donya::Clamp( rgba.w, 0.0f, 1.0f ) * normalizer );

		return fx;
	}
	Donya::Vector4		ToColor( const Effekseer::Color &fx )
	{
		Donya::Vector4 c;
		using DonyaColorT = decltype( c.x );

		constexpr float normalizer = 1.0f / 255.0f;
		c.x = static_cast<float>( fx.R ) * normalizer;
		c.y = static_cast<float>( fx.G ) * normalizer;
		c.z = static_cast<float>( fx.B ) * normalizer;
		c.w = static_cast<float>( fx.A ) * normalizer;

		return c;
	}

	stdEfkString ToEfkString( const std::string &str )
	{
		return Convert<char, EFK_CHAR>( str );
	}
	stdEfkString ToEfkString( const std::wstring &str )
	{
		return Convert<wchar_t, EFK_CHAR>( str );
	}
	std::string  ToString( const stdEfkString &str )
	{
		return Convert<EFK_CHAR, char>( str );
	}
	std::wstring ToWString( const stdEfkString &str )
	{
		return Convert<EFK_CHAR, wchar_t>( str );
	}
}
