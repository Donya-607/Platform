#include "FontHelper.h"

#include <unordered_map>

#include "Donya/Constant.h" // Use DEBUG_MODE macro

namespace FontHelper
{
	namespace
	{
		static std::unordered_map<FontAttribute, std::shared_ptr<Donya::Font::Renderer>>
			rendererMap;

		bool Load( FontAttribute attr )
		{
			// Already loaded?
			const auto found = rendererMap.find( attr );
			if ( found != rendererMap.end() ) { return true; }
			// else


			bool succeeded = true;
			auto pFontLoader = std::make_unique<Donya::Font::Holder>();


			// Load the specified font
		#if DEBUG_MODE
			const auto binPath = MakeFontPathBinary( attr );
			if ( Donya::IsExistFile( binPath ) )
			{
				// Serialized font file is there
				if ( !pFontLoader->LoadByCereal( binPath ) ) { succeeded = false; }
			}
			else
			{
				// Load .fnt file and serialize it
				if ( !pFontLoader->LoadFntFile( MakeFontPathFnt( attr ) ) ) { succeeded = false; }
				pFontLoader->SaveByCereal( binPath );
			}
		#else
			if ( !pFontLoader->LoadByCereal( MakeFontPathBinary( attr ) ) ) { succeeded = false; }
		#endif // DEBUG_MODE


			// Insert into the map if the initialization is succeeded
			auto pTmp = std::make_shared<Donya::Font::Renderer>();
			if ( pTmp->Init( *pFontLoader ) )
			{
				rendererMap.insert( std::make_pair( attr, pTmp ) );
			}
			else
			{
				succeeded = false;
			}


			// Clear the loader and return the result
			pFontLoader.reset();
			return succeeded;
		}
	}

	bool Init()
	{
		bool succeeded = true;

		if ( !Load( FontAttribute::Main ) ) { succeeded = false; }

		return succeeded;
	}
	void Uninit()
	{
		rendererMap.clear();
	}

	std::shared_ptr<Donya::Font::Renderer> GetRendererOrNullptr( FontAttribute attr )
	{
		const auto found = rendererMap.find( attr );
		if ( found != rendererMap.end() )
		{
			return found->second;
		}
		// else
		return nullptr;
	}
}
