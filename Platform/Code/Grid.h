#pragma once

#include <string>
#include <memory>

#include "Donya/Vector.h"
#include "Donya/UseImGui.h"

namespace Donya
{
	namespace Geometric
	{
		class Line;
	}
}

class GridLine
{
private:
	static constexpr unsigned int MAX_LINE_COUNT = 512U;
private:
	Donya::Vector2	lineLength;		// Half length.
	Donya::Vector2	drawInterval;
	Donya::Vector3	drawOrigin;
	std::shared_ptr<Donya::Geometric::Line>	pLine = nullptr;
public:
	~GridLine();
public:
	bool Init();
	void Uninit();

	void Draw( const Donya::Vector4x4 &VP ) const;
public:
	void SetDrawLength	( const Donya::Vector2 &halfDrawLength );
	void SetDrawInterval( const Donya::Vector2 &drawInterval );
	void SetDrawOrigin	( const Donya::Vector3 &wsDrawOrigin );

	Donya::Vector2 GetDrawLength() const;
	Donya::Vector2 GetDrawInterval() const;
	Donya::Vector3 GetDrawOrigin() const;
private:
	Donya::Int2 CalcDrawCount() const;
#if USE_IMGUI
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
