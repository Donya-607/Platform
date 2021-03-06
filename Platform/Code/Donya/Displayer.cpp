#include "Displayer.h"

#include "Constant.h"		// Use scast<>()
#include "Direct3DUtil.h"	// Use CreateVertexBuffer()
#include "Donya.h"			// Use GetDevice(), GetImmediateContext()
#include "ScreenShake.h"
#include "Sprite.h"			// Use GetDrawDepth()

#undef max
#undef min

namespace Donya
{
	bool Displayer::Init( ID3D11Device *pDevice )
	{
		if ( wasInitialized ) { return true; }
		// else

		if ( !pDevice ) { pDevice = ::Donya::GetDevice(); }

		constexpr Donya::Vector4 defaultColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		constexpr std::array<Displayer::Vertex, 4> initialVertices
		{
			Displayer::Vertex{ Donya::Vector3{ 0.0f, 1.0f, 0.0f }, defaultColor, Donya::Vector2{ 0.0f, 1.0f } },
			Displayer::Vertex{ Donya::Vector3{ 1.0f, 1.0f, 0.0f }, defaultColor, Donya::Vector2{ 1.0f, 1.0f } },
			Displayer::Vertex{ Donya::Vector3{ 0.0f, 0.0f, 0.0f }, defaultColor, Donya::Vector2{ 0.0f, 0.0f } },
			Displayer::Vertex{ Donya::Vector3{ 1.0f, 0.0f, 0.0f }, defaultColor, Donya::Vector2{ 1.0f, 0.0f } }
		};

		const HRESULT hr = Donya::CreateVertexBuffer<Displayer::Vertex>
		(
			pDevice, initialVertices,
			D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE,
			pVertexBuffer.GetAddressOf()
		);
		if ( FAILED( hr ) )
		{
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create vertex-buffer." );
			return false;
		}
		// else

		wasInitialized = true;
		return true;
	}

#pragma region Normal
	bool Displayer::Draw			( const Donya::Vector2 &originalTexSize, const Donya::Vector2 &ssPos, float degree, float alpha, const Donya::Vector2 &origin ) const
	{
		return DrawExt
		(
			originalTexSize,
			ssPos,
			defaultConfig.scale, degree,
			defaultConfig.MakeColor( alpha ), origin
		);
	}
	bool Displayer::DrawExt			( const Donya::Vector2 &originalTexSize, const Donya::Vector2 &ssPos, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
	{
		return DrawGeneralExt
		(
			originalTexSize,
			ssPos, originalTexSize,
			Donya::Vector2::Zero(), originalTexSize,
			scale, degree,
			color, origin
		);
	}
#pragma endregion

#pragma region Stretched
	bool Displayer::DrawStretched	( const Donya::Vector2 &originalTexSize, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, float degree, float alpha, const Donya::Vector2 &origin ) const
	{
		return DrawStretchedExt
		(
			originalTexSize,
			ssPos, ssSize,
			defaultConfig.scale, degree,
			defaultConfig.MakeColor( alpha ), origin
		);
	}
	bool Displayer::DrawStretchedExt( const Donya::Vector2 &originalTexSize, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
	{
		return DrawGeneralExt
		(
			originalTexSize,
			ssPos, ssSize,
			Donya::Vector2::Zero(), originalTexSize,
			scale, degree,
			color, origin
		);
	}
#pragma endregion

#pragma region Part
	bool Displayer::DrawPart		( const Donya::Vector2 &originalTexSize, const Donya::Vector2 &ssPos, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, float degree, float alpha, const Donya::Vector2 &origin ) const
	{
		return DrawPartExt
		(
			originalTexSize,
			ssPos,
			texPos, texSize,
			defaultConfig.scale, degree,
			defaultConfig.MakeColor( alpha ), origin
		);
	}
	bool Displayer::DrawPartExt		( const Donya::Vector2 &originalTexSize, const Donya::Vector2 &ssPos, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
	{
		return DrawGeneralExt
		(
			originalTexSize,
			ssPos,  originalTexSize,
			texPos, texSize,
			scale,  degree,
			color,  origin
		);
	}
#pragma endregion

#pragma region General
	bool Displayer::DrawGeneral		( const Donya::Vector2 &originalTexSize, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, float degree, float alpha, const Donya::Vector2 &origin ) const
	{
		return DrawGeneralExt
		(
			originalTexSize,
			ssPos, ssSize,
			texPos, texSize,
			defaultConfig.scale, degree,
			defaultConfig.MakeColor( alpha ), origin
		);
	}
	bool Displayer::DrawGeneralExt	( const Donya::Vector2 &originalTexSize, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
	{
		if ( !wasInitialized )
		{
			_ASSERT_EXPR( 0, L"Error : The display does not initialized!" );
			return false;
		}
		// else

		ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();

		const Donya::Vector2 scrSize{ ssSize.x * scale.x, ssSize.y * scale.y };
		Donya::Vector2 scrPos = ssPos;

		// Adjust the origin of rotation
		scrPos.x -= scrSize.x * origin.x;
		scrPos.y -= scrSize.y * origin.y;

		if ( Donya::ScreenShake::GetEnableState() )
		{
			scrPos.x -= Donya::ScreenShake::GetX();
			scrPos.y -= Donya::ScreenShake::GetY();
		}

		const auto NDCVertices = MakeNDCVertices
		(
			originalTexSize,
			scrPos, scrSize,
			texPos, texSize,
			degree, color, origin
		);

		if ( !MapVertices( NDCVertices, pImmediateContext ) ) { return false; }
		// else
		
		constexpr UINT stride = sizeof( Displayer::Vertex );
		constexpr UINT offset = 0;
		pImmediateContext->IASetVertexBuffers( 0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset );
		pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
		
		pImmediateContext->Draw( NDCVertices.size(), 0 );

		return true;
	}
#pragma endregion

	std::array<Displayer::Vertex, 4U> Displayer::MakeNDCVertices( const Donya::Vector2 &originalTexSize, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
	{
		constexpr size_t vertexCount = 4U;

		const float drawDepth = Sprite::GetDrawDepth();
		std::array<Displayer::Vertex, vertexCount> vertices
		{
			/* LT */ Displayer::Vertex{ Donya::Vector3{ ssPos.x,			ssPos.y,			drawDepth }, color, Donya::Vector2{ texPos.x,				texPos.y				} },
			/* RT */ Displayer::Vertex{ Donya::Vector3{ ssPos.x + ssSize.x,	ssPos.y,			drawDepth }, color, Donya::Vector2{ texPos.x + texSize.x,	texPos.y				} },
			/* LB */ Displayer::Vertex{ Donya::Vector3{ ssPos.x,			ssPos.y + ssSize.y,	drawDepth }, color, Donya::Vector2{ texPos.x,				texPos.y + texSize.y	} },
			/* RB */ Displayer::Vertex{ Donya::Vector3{ ssPos.x + ssSize.x,	ssPos.y + ssSize.y,	drawDepth }, color, Donya::Vector2{ texPos.x + texSize.x,	texPos.y + texSize.y	} },
		};

		// Rotate vertices at origin
		{
			auto RotateZAxis = []( Donya::Vector3 *pVector, float cos, float sin )
			{
				const Donya::Vector2 relative{ pVector->x, pVector->y };

				pVector->x = ( relative.x * cos ) - ( relative.y * sin );
				pVector->y = ( relative.y * cos ) + ( relative.x * sin );
			};

			// Translate sprite's centre to origin ( rotate centre )
			const Donya::Vector2 distFromOrigin
			{
				ssPos.x + ( ssSize.x * origin.x ),
				ssPos.y + ( ssSize.y * origin.y ),
			};

			for ( size_t i = 0; i < vertexCount; ++i )
			{
				vertices[i].pos.x -= distFromOrigin.x;
				vertices[i].pos.y -= distFromOrigin.y;
			}

			const float radian = ToRadian( degree );
			const float cos = cosf( radian );
			const float sin = sinf( radian );
			for ( size_t i = 0; i < vertexCount; ++i )
			{
				RotateZAxis( &vertices[i].pos, cos, sin );
			}

			for ( size_t i = 0; i < vertexCount; ++i )
			{
				vertices[i].pos.x += distFromOrigin.x;
				vertices[i].pos.y += distFromOrigin.y;
			}
		}

		// Convert to NDC space
		{
			/*
			2/xMax-xMin     0               0            -((xMax+xMin)/(xMax-xMin))
			0               2/yMax-yMin     0            -((yMax+yMin)/(yMax-yMin))
			0               0               1/zMax-zMin  -(    zMin   /(zMax-zMin))
			0               0               0            1
			*/
			const Donya::Vector2 screenSize
			{
				Donya::Private::RegisteredScreenWidthF(),
				Donya::Private::RegisteredScreenHeightF()
			};

			for ( size_t i = 0; i < vertexCount; ++i )
			{
				vertices[i].pos.x =        ( ( 2.0f * vertices[i].pos.x ) / ( screenSize.x ) ) - 1.0f;
				vertices[i].pos.y = 1.0f - ( ( 2.0f * vertices[i].pos.y ) / ( screenSize.y ) );

				vertices[i].texCoord.x /= originalTexSize.x;
				vertices[i].texCoord.y /= originalTexSize.y;
			}
		}

		return vertices;
	}

	bool Displayer::MapVertices( const std::array<Displayer::Vertex, 4U> &NDCVertices, ID3D11DeviceContext *pImmediateContext ) const
	{
		if ( !pImmediateContext ) { pImmediateContext = Donya::GetImmediateContext(); }

		D3D11_MAPPED_SUBRESOURCE msr{};

		const HRESULT hr = pImmediateContext->Map( pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr );
		if ( FAILED( hr ) )
		{
			_ASSERT_EXPR( 0, L"Failed: Displayer::Map" );
			return false;
		}
		// else

		const size_t destSize = std::min( sizeof( Displayer::Vertex ) * NDCVertices.size(), msr.RowPitch ); // Usually I expect these size is same. Using smaller one is a fail-safe.
		memcpy_s( msr.pData, destSize, NDCVertices.data(), destSize );

		pImmediateContext->Unmap( pVertexBuffer.Get(), 0 );

		return true;
	}

}
