#include "EffectUtil.h"

#include <iterator>		// Use std::begin()/end()

namespace Effect
{
	template<typename FromChar, typename ToChar>
	std::basic_string<ToChar> Convert( const std::basic_string<FromChar> &str )
	{
		// See https://dbj.org/c17-codecvt-deprecated-panic/

		if ( str.empty() ) { return {}; }
		// else
		
		return { std::begin( str ), std::end( str ) };
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
