#pragma once

#include <vector>

namespace Math
{
	namespace Impl
	{
		template<typename T>
		constexpr T CalcInteriorPoint( const T &lhs, const T &rhs, float t )
		{
			return lhs + ( t * ( rhs - lhs ) );
		}
	}
	/// <summary>
	/// The points count must be greater-eq than 2
	/// </summary>
	template<typename T>
	T CalcBezierCurve( const std::vector<T> points, float t )
	{
		const int pointCount = static_cast<int>( points.size() );
		
		if ( pointCount < 2 )
		{
			assert( !"Error: Can not calc Bezier-Curve!" );
			return T{};
		}
		// else
		if ( pointCount == 2 )
		{
			return Impl::CalcInteriorPoint( points[0], points[1], t );
		}
		// else

		std::vector<T> separated;
		separated.resize( pointCount - 1 );

		for ( int i = 0; i < pointCount - 1; ++i )
		{
			separated[i] = Impl::CalcInteriorPoint( points[i], points[i + 1], t );
		}

		return CalcBezierCurve( separated, t );
	}
}
