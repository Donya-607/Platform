#include "Grid.h"

#include "Donya/GeometricPrimitive.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"

namespace
{
	static constexpr int MAX_INSTANCE_COUNT = 4096;
}

GridLine::~GridLine()
{
	if ( pLine ) { pLine->Uninit(); }
	pLine.reset();
}

bool GridLine::Init()
{
	pLine = std::make_shared<Donya::Geometric::Line>( MAX_INSTANCE_COUNT );
	if ( !pLine->Init() )
	{
		_ASSERT_EXPR( 0, L"Failed: The line initialization was failed." );
		return false;
	}
	// else

	return true;
}
void GridLine::Uninit()
{
	if ( pLine ) { pLine->Uninit(); }
	pLine.reset();
}

void GridLine::Draw( const Donya::Vector4x4 &matVP ) const
{
	if ( !pLine ) { return; }
	// else

	const Donya::Vector3 origin	{ drawOrigin.x - lineLength.x,	drawOrigin.y - lineLength.y,	drawOrigin.z		};
	const Donya::Vector3 endX	{ lineLength.x * 2.0f,			0.0f,							0.0f				};
	const Donya::Vector3 endY	{ 0.0f,							lineLength.y * 2.0f,			0.0f				};
	const Donya::Vector3 offsetX{ drawInterval.x,				0.0f,							0.0f				};
	const Donya::Vector3 offsetY{ 0.0f,							drawInterval.y,					0.0f				};
	
	const Donya::Int2 loopCount = CalcDrawCount();

	for ( int y = 0; y < loopCount.y; ++y )
	{
		// Left -> Right
		pLine->Reserve
		(
			origin +		( offsetY * scast<float>( y ) ),
			origin + endX +	( offsetY * scast<float>( y ) )
		);
	}
	for ( int x = 0; x < loopCount.x; ++x )
	{
		// Bottom -> Top
		pLine->Reserve
		(
			origin +		( offsetX * scast<float>( x ) ),
			origin + endY +	( offsetX * scast<float>( x ) )
		);
	}
	
	pLine->Flush( matVP );
}

void GridLine::SetDrawLength	( const Donya::Vector2 &halfLength	)	{ lineLength	= halfLength;	}
void GridLine::SetDrawInterval	( const Donya::Vector2 &interval	)	{ drawInterval	= interval;		}
void GridLine::SetDrawOrigin	( const Donya::Vector3 &origin		)	{ drawOrigin	= origin;		}

Donya::Vector2 GridLine::GetDrawLength() const						{ return lineLength;			}
Donya::Vector2 GridLine::GetDrawInterval() const					{ return drawInterval;			}
Donya::Vector3 GridLine::GetDrawOrigin() const						{ return drawOrigin;			}

Donya::Int2 GridLine::CalcDrawCount() const
{
	const int xCount = ( IsZero( drawInterval.x ) ) ? 0 : scast<int>( ( lineLength.x * 2.0f ) / drawInterval.x );
	const int yCount = ( IsZero( drawInterval.y ) ) ? 0 : scast<int>( ( lineLength.y * 2.0f ) / drawInterval.y );
	return Donya::Int2
	{
		xCount + 1/* Edge */,
		yCount + 1/* Edge */
	};
}

#if USE_IMGUI
void GridLine::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat3( u8"中心位置",				&drawOrigin.x,		0.05f );
	ImGui::DragFloat2( u8"線の長さXY（半径）",	&lineLength.x,		0.05f );
	ImGui::DragFloat2( u8"線の間隔XY（直径）",	&drawInterval.x,	0.05f );

	const Donya::Int2 drawingCount = CalcDrawCount();
	ImGui::Text( u8"本数：[Ｘ：%d][Ｙ：%d][計：%d]", drawingCount.x - 1, drawingCount.y - 1, drawingCount.x + drawingCount.y - 2 );
	ImGui::Text( u8"最大本数：[%d]", MAX_LINE_COUNT );

	ImGui::TreePop();
}
#endif // USE_IMGUI
