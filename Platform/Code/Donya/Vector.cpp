#include "Vector.h"

#include "Quaternion.h"
#include "Useful.h"		// Use ToDegree(), Equal().

using namespace DirectX;

namespace Donya
{
#pragma region Vector2

	Vector2 Vector2::Normalize()
	{
		float length = Length();

		if ( ::IsZero( length ) ) { return *this; }
		// else

		x /= length;
		y /= length;

		return *this;
	}
	Vector2 Vector2::Unit() const
	{
		Vector2 normalized = *this;
		normalized.Normalize();
		return normalized;
	}
	float Vector2::Radian() const
	{
		return atan2f( y, x );
	}
	float Vector2::Degree() const
	{
		return ToDegree( Radian() );
	}
	bool Vector2::IsZero() const
	{
		return ( ::IsZero( LengthSq() ) ) ? true : false;
	}
	Vector2 Vector2::Rotate( float radian )
	{
		const float cos = cosf( radian );
		const float sin = sinf( radian );
		return Vector2
		{
			x * cos - y * sin,
			x * sin + y * cos
		};
	}
	bool operator == ( const Vector2 &L, const Vector2 &R )
	{
		return Vector2{ L - R }.IsZero();
	}

#pragma endregion

#pragma region Vector3

	Vector3 Vector3::Normalize()
	{
		float length = Length();

		if ( ::IsZero( length ) ) { return *this; }
		// else

		x /= length;
		y /= length;
		z /= length;

		return *this;
	}
	Vector3 Vector3::Unit() const
	{
		Vector3 normalized = *this;
		normalized.Normalize();
		return normalized;
	}
	bool Vector3::IsZero() const
	{
		return ( ::IsZero( LengthSq() ) ) ? true : false;
	}
	bool operator == ( const Vector3 &L, const Vector3 &R )
	{
		if ( !IsZero( L.x - R.x ) ) { return false; }
		if ( !IsZero( L.y - R.y ) ) { return false; }
		if ( !IsZero( L.z - R.z ) ) { return false; }
		// else
		return true;
	}

#pragma endregion

#pragma region Vector4

	Vector4 &Vector4::AssignXMVector( const XMVECTOR &V )
	{
		XMStoreFloat4( this, V );
		return *this;
	}

	Vector4 Vector4::FromXMVector( const XMVECTOR &V )
	{
		XMFLOAT4 vector{};
		XMStoreFloat4( &vector, V );
		return Vector4{ vector };
	}

	bool operator == ( const Vector4 &L, const Vector4 &R )
	{
		float diffX = fabsf( L.x - R.x );
		float diffY = fabsf( L.y - R.y );
		float diffZ = fabsf( L.z - R.z );
		float diffW = fabsf( L.w - R.w );

		if ( FLT_EPSILON <= diffX ) { return false; }
		if ( FLT_EPSILON <= diffY ) { return false; }
		if ( FLT_EPSILON <= diffZ ) { return false; }
		if ( FLT_EPSILON <= diffW ) { return false; }
		// else
		return true;
	}

#pragma endregion

#pragma region Vector4x4

	/// <summary>
	/// Returns X:Row, Y:Column.
	/// </summary>
	constexpr Int2 CalcRowColumnFromIndex( unsigned int index )
	{
		constexpr unsigned int ROW_COUNT = 4U;
		return Donya::Int2
		{
			static_cast<int>( index % ROW_COUNT ),
			static_cast<int>( index / ROW_COUNT )
		};
	}
	const float &Vector4x4::operator [] ( unsigned int index ) const &
	{
		Int2 indices = CalcRowColumnFromIndex( index );
		return m[indices.x][indices.y];
	}
	float &Vector4x4::operator [] ( unsigned int index ) &
	{
		Int2 indices = CalcRowColumnFromIndex( index );
		return m[indices.x][indices.y];
	}
	float Vector4x4::operator [] ( unsigned int index ) const &&
	{
		Int2 indices = CalcRowColumnFromIndex( index );
		return m[indices.x][indices.y];
	}
	float &Vector4x4::operator () ( unsigned int row, unsigned int column )
	{
		constexpr unsigned int ROW_COUNT	= 4U;
		constexpr unsigned int COLUMN_COUNT	= 4U;
		if ( ROW_COUNT <= row || COLUMN_COUNT <= column )
		{
			_ASSERT_EXPR( 0, L"Error : out of range at Vector4x4::operator() access." );
			return operator[]( 0 );
		}
		// else

		return m[row][column];
	}

	Vector4x4 &Vector4x4::AssignMatrix( const XMMATRIX &M )
	{
		XMStoreFloat4x4( this, M );
		return *this;
	}
	XMMATRIX  Vector4x4::ToMatrix() const
	{
		return XMLoadFloat4x4( this );
	}

#pragma region Arithmetic
	Vector4x4 Vector4x4::Mul( const Vector4x4 &R ) const
	{
		XMMATRIX LHS = ToMatrix();
		XMMATRIX RHS = R.ToMatrix();
		
		return FromMatrix( LHS * RHS );
	}
	Vector4   Vector4x4::Mul( const Vector4 &V ) const
	{
		return Vector4::FromXMVector
		(
			XMVector4Transform
			(
				V.ToXMVector(),
				ToMatrix()
			)
		);
	}
	Vector4   Vector4x4::Mul( const Vector3 &V, float fourthParam ) const
	{
		return Mul( Vector4{ V.x,V.y,V.z, fourthParam } );
	}

	Vector4x4 &Vector4x4::operator *= ( const Vector4x4 &R )
	{
		*this = Mul( R );
		return *this;
	}
#pragma endregion

	Vector4x4 Vector4x4::Inverse() const
	{
		return FromMatrix
		(
			XMMatrixInverse( nullptr, ToMatrix() )
		);
	}
	Vector4x4 Vector4x4::Transpose() const
	{
		return FromMatrix
		(
			XMMatrixTranspose( ToMatrix() )
		);
	}
	
	Vector4x4 Vector4x4::FromMatrix( const XMMATRIX &M )
	{
		XMFLOAT4X4 matrix{};
		XMStoreFloat4x4( &matrix, M );
		return Vector4x4{ matrix };
	}

	Vector4x4 Vector4x4::MakeScaling( const Vector3 &scale )
	{
		return FromMatrix
		(
			XMMatrixScaling
			(
				scale.x,
				scale.y,
				scale.z
			)
		);
	}
	Vector4x4 Vector4x4::MakeRotationEuler( const Vector3 &eulerRadian )
	{
		return FromMatrix
		(
			XMMatrixRotationRollPitchYaw
			(
				eulerRadian.x,
				eulerRadian.y,
				eulerRadian.z
			)
		);
	}
	Vector4x4 Vector4x4::MakeRotationAxis( const Vector3 &axis, float angleRadian )
	{
		return FromMatrix
		(
			XMMatrixRotationAxis
			(
				axis.ToXMVector( 0.0f ),
				angleRadian
			)
		);
	}
	Vector4x4 Vector4x4::MakeRotationNormalAxis( const Vector3 &nAxis, float angleRadian )
	{
		return FromMatrix
		(
			XMMatrixRotationNormal
			(
				nAxis.ToXMVector( 0.0f ),
				angleRadian
			)
		);
	}
	Vector4x4 Vector4x4::MakeTranslation( const Vector3 &offset )
	{
		return FromMatrix
		(
			XMMatrixTranslation( offset.x, offset.y, offset.z )
		);
	}

	Vector4x4 Vector4x4::MakeTransformation( const Vector3 &scaling, const Quaternion &rotation, const Vector3 translation )
	{
		Vector4x4 M;
		M._11 = scaling.x;
		M._22 = scaling.y;
		M._33 = scaling.z;
		M *= rotation.MakeRotationMatrix();
		M._41 = translation.x;
		M._42 = translation.y;
		M._43 = translation.z;
		return M;
	}

	Vector4x4 Vector4x4::MakeLookAtLH( const Vector3 &eye, const Vector3 &focus, const Vector3 &up )
	{
		const Vector3 zAxis = Vector3{ focus - eye }.Unit();
		const Vector3 xAxis = Vector3{ Cross( up, zAxis ) }.Unit();
		const Vector3 yAxis = Cross( zAxis, xAxis ).Unit();

		Vector4x4 m = MakeRotationOrthogonalAxis( xAxis, yAxis, zAxis );
		m._41 = -Dot( xAxis, eye );
		m._42 = -Dot( yAxis, eye );
		m._43 = -Dot( zAxis, eye );
		m._43 = 1.0f;
		return m;
	}

	Vector4x4 Vector4x4::MakeOrthographicLH( const Vector2 &view, float zNear, float zFar )
	{
		return FromMatrix
		(
			XMMatrixOrthographicLH( view.x, view.y, zNear, zFar )
		);
	}
	Vector4x4 Vector4x4::MakePerspectiveFovLH( float FOV, float aspect, float zNear, float zFar )
	{
		return FromMatrix
		(
			XMMatrixPerspectiveFovLH( FOV, aspect, zNear, zFar )
		);
	}

	bool operator == ( const Vector4x4 &L, const Vector4x4 &R )
	{
		constexpr unsigned int ELEMENT_COUNT = 15U;
		for ( unsigned int i = 0; i < ELEMENT_COUNT; ++i )
		{
			if ( !IsZero( L[i] - R[i] ) )
			{
				return false;
			}
		}
		return true;
	}

#pragma endregion
}