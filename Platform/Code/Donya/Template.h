#pragma once

#include <algorithm>	// Use std::max, min
#include <memory>

#undef max
#undef min

namespace Donya
{
	/// <summary>
	/// The Get() returns reference of T.<para></para>
	/// Please register to friend.
	/// </summary>
	template<class T>
	class Singleton
	{
	protected:
		Singleton() = default;
		Singleton( const Singleton &  ) = delete;
		Singleton(	     Singleton && ) = delete;
		Singleton &operator = ( const Singleton &  ) = delete;
		Singleton &operator = (	      Singleton && ) = delete;
	public:
		~Singleton() = default;
	public:
		static T &Get()
		{
			static T instance{};
			return instance;
		}
	};

	/// <summary>
	/// This class can show the T's type.<para></para>
	/// This class don't have definition, so a compiler output the error if you use this class.<para></para>
	/// You can know the T's type by that error-message.
	/// </summary>
	template<typename T> class TypeDetective;

	/// <summary>
	/// Returns std::make_unique( source.get() ) if the source pointer is valid.
	/// </summary>
	template<typename T>
	constexpr std::unique_ptr<T> Clone( const std::unique_ptr<T> &source )
	{
		return	( !source )
				? std::make_unique<T>()
				: std::make_unique<T>( *source );
	}

	template<typename T>
	T Clamp( const T &v, const T &min, const T &max )
	{
		return std::max( min, std::min( max, v ) );
	}
}
