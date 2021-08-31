#pragma once

#include <memory>

#include "Donya/Font.h"

#include "FilePath.h" // Use FontAttribute

namespace FontHelper
{
	bool Init();
	void Uninit();

	std::shared_ptr<Donya::Font::Renderer> GetRendererOrNullptr( FontAttribute font );
}
