#include "EffectUtil.h"

#include <iterator>				// Use std::begin()/end()

#include "../Donya/Template.h"	// Use Clamp()
#include "../Donya/Useful.h"	// Use characer convert function

namespace Effect
{
	namespace
	{
		constexpr int maxStringBufferCount = 512;

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
		EFK_CHAR dest[maxStringBufferCount];
		
		Effekseer::ConvertUtf8ToUtf16( dest, sizeof( dest ), str.c_str() );

		return dest;
	}
	stdEfkString ToEfkString( const std::wstring &str )
	{
		return ToEfkString( Donya::WideToUTF8( str ) );
	}
	std::string  ToString( const stdEfkString &str )
	{
		char dest[maxStringBufferCount];
		
		Effekseer::ConvertUtf16ToUtf8
		(
			reinterpret_cast<int8_t *>( dest ), sizeof( dest ),
			reinterpret_cast<const int16_t*>( str.c_str() )
		);

		return dest;
	}
	std::wstring ToWString( const stdEfkString &str )
	{
		return Donya::UTF8ToWide( ToString( str ) );
	}
}
