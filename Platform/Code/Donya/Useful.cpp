#include "Useful.h"

#include <cmath>
#include <crtdbg.h>
#include <d3d11.h>
#include <direct.h>		// Use _mkdir(), _wmkdir().
#include <float.h>
#include <fstream>
#include <iomanip>		// Use std::setprecesion(), etc
#include <locale>
#include <mutex>
#include <Shlwapi.h>	// Use PathRemoveFileSpecA(), PathAddBackslashA(), In AcquireDirectoryFromFullPath().
#include <sstream>		// Use ostringstream at ToString()
#include <vector>
#include <Windows.h>

#include "Constant.h"
#include "Donya.h"

#pragma comment( lib, "shlwapi.lib" ) // Use PathRemoveFileSpecA(), PathAddBackslashA(), in AcquireDirectoryFromFullPath().

namespace Donya
{
	float NormalizeRadian( float radian )
	{
		// http://hima-tubusi.blogspot.com/2016/12/blog-post_12.html
		return atan2f( sinf( radian ), cosf( radian ) );
	}
	float NormalizeDegree( float degree )
	{
		return ToDegree( NormalizeRadian( ToRadian( degree ) ) );
	}

	bool Equal( float L, float R, float maxRelativeDiff )
	{
	#if		0 // see https://marycore.jp/prog/c-lang/compare-floating-point-number/

		return ( fabsf( L - R ) <= maxRelativeDiff * fmaxf( 1.0f, fmaxf( fabsf( L ), fabsf( R ) ) ) ) ? true : false;

	#elif	0 // see http://berobemin2.hatenablog.com/entry/2016/02/27/231856

		float diff = fabsf( L - R );
		L = fabsf( L );
		R = fabsf( R );

		float &largest = ( L > R ) ? L : R;
		return ( diff <= largest * maxRelativeDiff ) ? true : false;

	#else // using std::isgreaterequal() and std::islessequal()

		return ( std::isgreaterequal<float>( L, R ) && std::islessequal<float>( L, R ) ) ? true : false;

	#endif
	}

	void OutputDebugStr( const char *string )
	{
	#if DEBUG_MODE
		OutputDebugStringA( string );
	#endif // DEBUG_MODE
	}
	void OutputDebugStr( const wchar_t *string )
	{
	#if DEBUG_MODE
		OutputDebugStringW( string );
	#endif // DEBUG_MODE
	}

	int ShowMessageBox( const char *text, const std::string &caption, unsigned int message )
	{
		return ShowMessageBox
		(
			Donya::UTF8ToWide( text    ),
			Donya::UTF8ToWide( caption ), 
			message
		);
	}
	int ShowMessageBox( const std::string &text, const std::string &caption, unsigned int message )
	{
		return ShowMessageBox( text.c_str(), caption, message );
	}
	int ShowMessageBox( const wchar_t *text, const std::wstring &caption, unsigned int message )
	{
		return MessageBox
		(
			Donya::GetHWnd(),
			text,
			caption.c_str(),
			message
		);
	}
	int ShowMessageBox( const std::wstring &text, const std::wstring &caption, unsigned int message )
	{
		return ShowMessageBox( text.c_str(), caption, message );
	}

	bool IsExistFile( const std::string &wholePath )
	{
		std::ifstream ifs( wholePath );
		return ifs.is_open();
	}
	bool IsExistFile( const std::wstring &wholePath )
	{
		std::wifstream ifs( wholePath );
		return ifs.is_open();
	}

	std::vector<unsigned int> SeparateDigits( unsigned int value, int storeDigits )
	{
		const int MAX_DIGIT = ( storeDigits <= 0 ) ? INT_MAX : storeDigits;

		std::vector<unsigned int> digits{};
		if ( 0 < storeDigits )
		{
			digits.reserve( storeDigits );
		}

		int i = 0;
		while ( 0 < value && i < MAX_DIGIT )
		{
			digits.emplace_back( value % 10 );
			value /= 10;

			i++;
		}

		while ( i < storeDigits )
		{
			digits.emplace_back( value % 10 );
			i++;
		}

		return digits;
	}

	template<typename DestCharType, typename FileOpenMethod, typename PathCharType>
	long ReadByteCodeImpl( std::unique_ptr<DestCharType[]> *pDestination, FileOpenMethod FileOpen, const PathCharType *filePath, const PathCharType *openMode )
	{
		if ( !pDestination						) { return -1; }
		if ( !Donya::IsExistFile( filePath )	) { return -1; }
		// else

		FILE *fp = nullptr;

		FileOpen( &fp, filePath, openMode );
		if ( !fp ) { return -1; }
		// else

		fseek( fp, 0, SEEK_END );
		long codeLength = ftell( fp );
		fseek( fp, 0, SEEK_SET );

		*pDestination = std::make_unique<DestCharType[]>( codeLength );
		fread( pDestination->get(), codeLength, 1, fp );
		
		fclose( fp );

		return codeLength;
	}
	long ReadByteCode( std::unique_ptr<char[]> *pDestination, const std::string &filePath, const char *openMode )
	{
		return ReadByteCodeImpl( pDestination, fopen_s, filePath.c_str(), openMode );
	}
	long ReadByteCode( std::unique_ptr<unsigned char[]> *pDestination, const std::string &filePath, const char *openMode )
	{
		return ReadByteCodeImpl( pDestination, fopen_s, filePath.c_str(), openMode );
	}
	long ReadByteCode( std::unique_ptr<char[]> *pDestination, const std::wstring &filePath, const wchar_t *openMode )
	{
		return ReadByteCodeImpl( pDestination, _wfopen_s, filePath.c_str(), openMode );
	}
	long ReadByteCode( std::unique_ptr<unsigned char[]> *pDestination, const std::wstring &filePath, const wchar_t *openMode )
	{
		return ReadByteCodeImpl( pDestination, _wfopen_s, filePath.c_str(), openMode );
	}

	template<typename T>
	std::string MakeArraySuffixImpl( T index )
	{
		return std::string{ "[" + std::to_string( index ) + "]" };
	}
	std::string MakeArraySuffix( int index )
	{
		return MakeArraySuffixImpl( index );
	}
	std::string MakeArraySuffix( size_t index )
	{
		return MakeArraySuffixImpl( index );
	}

	std::string ToString( float value, size_t width, size_t decimalCount, char fill )
	{
		std::ostringstream oss{};
		oss
			<< std::fixed
			<< std::setw( width )
			<< std::setprecision( decimalCount )
			<< std::setfill( fill )
			<< value
			;
		return oss.str();
	}

#pragma region Convert Character Functions

#define USE_WIN_API ( true )
#define IS_SETTING_LOCALE_NOW ( false )

	// these convert function referenced to https://nekko1119.hatenablog.com/entry/2017/01/02/054629

	std::wstring	MultiToWide( const std::string &source, int codePage )
	{
		// MultiByteToWideChar() : http://www.t-net.ne.jp/~cyfis/win_api/sdk/MultiByteToWideChar.html
		const int destSize = MultiByteToWideChar( codePage, 0U, source.data(), -1, nullptr, NULL );
		std::vector<wchar_t> dest( destSize, L'\0' );

		if ( MultiByteToWideChar( codePage, 0U, source.data(), -1, dest.data(), dest.size() ) == 0 )
		{
			throw std::system_error{ scast<int>( GetLastError() ), std::system_category() };
		}
		// else

		dest.resize( std::char_traits<wchar_t>::length( dest.data() ) );
		dest.shrink_to_fit();

		return std::wstring( dest.begin(), dest.end() );
	}
	std::string		WideToMulti( const std::wstring &source, int codePage )
	{
		// WideCharToMultiByte() : http://www.t-net.ne.jp/~cyfis/win_api/sdk/WideCharToMultiByte.html
		const int destSize = WideCharToMultiByte( codePage, 0U, source.data(), -1, nullptr, NULL, NULL, NULL );
		std::vector<char> dest( destSize, '\0' );

		if ( WideCharToMultiByte( codePage, 0U, source.data(), -1, dest.data(), dest.size(), NULL, NULL ) == 0 )
		{
			throw std::system_error{ scast<int>( ::GetLastError() ), std::system_category() };
		}
		// else

		dest.resize( std::char_traits<char>::length( dest.data() ) );
		dest.shrink_to_fit();

		return std::string( dest.begin(), dest.end() );
	}

	const wchar_t	*MultiToWide( const char *source )
	{
		return MultiToWide( std::string( source ) ).c_str();
	}
	const char		*WideToMulti( const wchar_t *source )
	{
		return WideToMulti( std::wstring( source ) ).data();
	}
	std::wstring	MultiToWide( const std::string &source )
	{
	#if USE_WIN_API

		return MultiToWide( source, CP_ACP );

	#else

		size_t resultLength = 0;
		std::vector<WCHAR> dest( source.size() + 1/*terminate*/, L'\0' );

	#if IS_SETTING_LOCALE_NOW
		errno_t err = _mbstowcs_s_l	// multi byte str to wide char str.
		(
			&resultLength,
			dest.data(),
			dest.size(),
			source.data(),
			_TRUNCATE,
			_create_locale( LC_ALL, "JPN" )
		);
	#else
		errno_t err = mbstowcs_s	// multi byte str to wide char str.
		(
			&resultLength,
			dest.data(),
			dest.size(),
			source.data(),
			_TRUNCATE
		);
	#endif // IS_SETTING_LOCALE_NOW

		if ( err != 0 ) { _ASSERT_EXPR( 0, L"Failed : MultiToWide()" ); };

		dest.resize( std::char_traits<wchar_t>::length( dest.data() ) );
		dest.shrink_to_fit();

		return std::wstring{ dest.begin(), dest.end() };

	#endif // USE_WIN_API
	}
	std::string		WideToMulti( const std::wstring &source )
	{
	#if USE_WIN_API

		return WideToMulti( source, CP_ACP );

	#else

		size_t resultLength = 0;
		std::vector<char> dest( source.size() * sizeof( wchar_t ) + 1, '\0' );
	#if IS_SETTING_LOCALE_NOW
		errno_t err = _wcstombs_s_l	// wide char str to multi byte str.
		(
			&resultLength,
			dest.data(),
			dest.size(),
			source.data(),
			_TRUNCATE,
			_create_locale( LC_ALL, "JPN" )
		);
	#else
		errno_t err = wcstombs_s	// wide char str to multi byte str.
		(
			&resultLength,
			dest.data(),
			dest.size(),
			source.data(),
			_TRUNCATE
		);
	#endif // IS_SETTING_LOCALE_NOW
		if ( err != 0 ) { _ASSERT_EXPR( 0, L"Failed : WideToMulti()" ); };

		dest.resize( std::char_traits<char>::length( dest.data() ) );
		dest.shrink_to_fit();

		return std::string{ dest.begin(), dest.end() };

	#endif // USE_WIN_API
	}

	std::wstring	UTF8ToWide( const std::string &source )
	{
		return MultiToWide( source, CP_UTF8 );
	}
	std::string		WideToUTF8( const std::wstring &source )
	{
		// std::wstring_convert is deprecated in C++17.

		return WideToMulti( source, CP_UTF8 );
	}

	std::string		MultiToUTF8( const std::string &source )
	{
		return WideToUTF8( MultiToWide( source ) );
	}
	std::string		UTF8ToMulti( const std::string &source )
	{
		return WideToMulti( UTF8ToWide( source ) );
	}

#undef IS_SETTING_LOCALE_NOW
#undef USE_WIN_API

#pragma endregion

	static std::mutex mutexFullPathName{};

	template<typename CharType, typename StringType, typename Method>
	StringType   ToFullPathImpl( const StringType &filePath, Method GetFullPathNameMethod )
	{
		// GetFullPathName() is thread unsafe.
		// Because a current directory always have possibility will be changed by another thread.
		// See https://stackoverflow.com/questions/54814130/is-there-a-thread-safe-version-of-getfullpathname

		std::lock_guard<std::mutex> enterCS( mutexFullPathName );

		const auto bufferSize = GetFullPathNameMethod( filePath.c_str(), NULL, NULL, nullptr );
		std::unique_ptr<CharType[]> buffer = std::make_unique<CharType[]>( bufferSize + sizeof( CharType )/* Null-Terminate */ );

		/* auto result = */ GetFullPathNameMethod( filePath.c_str(), bufferSize, buffer.get(), nullptr );

		return StringType{ buffer.get() };
	}
	std::string  ToFullPath( const std::string &filePath )
	{
		return ToFullPathImpl<char>( filePath, GetFullPathNameA );
	}
	std::wstring ToFullPath( const std::wstring &filePath )
	{
		return ToFullPathImpl<wchar_t>( filePath, GetFullPathNameW );
	}

	template<typename CharType, typename StringType, typename RemoveFileSpecMethod, typename AddBackslashMethod>
	StringType   ExtractFileDirectoryFromFullPathImpl( const StringType &fullPath, RemoveFileSpecMethod PathRemoveFileSpecMethod, AddBackslashMethod PathAddBackslashMethod )
	{
		const size_t pathLength = fullPath.size();
		if ( !pathLength ) { return StringType{}; }
		// else

		std::unique_ptr<CharType[]> directory = std::make_unique<CharType[]>( pathLength + sizeof( CharType )/* Null-Terminate */ );
		for ( size_t i = 0; i < pathLength; ++i )
		{
			directory[i] = fullPath[i];
		}

		PathRemoveFileSpecMethod( directory.get() );
		PathAddBackslashMethod	( directory.get() );

		return StringType{ directory.get() };
	}
	std::string  ExtractFileDirectoryFromFullPath( const std::string &fullPath )
	{
		return	ExtractFileDirectoryFromFullPathImpl<char>
				(
					fullPath,
					PathRemoveFileSpecA,
					PathAddBackslashA
				);
	}
	std::wstring ExtractFileDirectoryFromFullPath( const std::wstring &fullPath )
	{
		return	ExtractFileDirectoryFromFullPathImpl<wchar_t>
				(
					fullPath,
					PathRemoveFileSpecW,
					PathAddBackslashW
				);
	}

	std::string  ExtractFileNameFromFullPath( const std::string &fullPath )
	{
		const	std::string fileDirectory = ExtractFileDirectoryFromFullPath( fullPath );
		return	( fileDirectory.empty() )
				? ""
				: fullPath.substr( fileDirectory.size() );
	}
	std::wstring ExtractFileNameFromFullPath( const std::wstring &fullPath )
	{
		const	std::wstring fileDirectory = ExtractFileDirectoryFromFullPath( fullPath );
		return	( fileDirectory.empty() )
				? L""
				: fullPath.substr( fileDirectory.size() );
	}

	bool MakeDirectory( const std::string  &dirPath )
	{
		const int result = _mkdir( dirPath.c_str() );
		return  ( result == 0 ) ? true : false;
	}
	bool MakeDirectory( const std::wstring &dirPath )
	{
		const int result = _wmkdir( dirPath.c_str() );
		return  ( result == 0 ) ? true : false;
	}
}