#include "FontHelper.h"

#include <unordered_map>

#include "Donya/Constant.h" // Use DEBUG_MODE macro

namespace FontHelper
{
	namespace
	{
		static std::unordered_map<FontAttribute, std::shared_ptr<Donya::Font::Renderer>>
			rendererMap;
	}

	bool Init()
	{
		auto Load = [&]( FontAttribute attr )
		{
			const auto found = rendererMap.find( attr );
			if ( found != rendererMap.end() ) { return true; }
			// else

			bool succeeded = true;
			auto pFontLoader = std::make_unique<Donya::Font::Holder>();

		#if DEBUG_MODE
			const auto binPath = MakeFontPathBinary( attr );
			if ( Donya::IsExistFile( binPath ) )
			{
				if ( !pFontLoader->LoadByCereal( binPath ) ) { succeeded = false; }
			}
			else
			{
				if ( !pFontLoader->LoadFntFile( MakeFontPathFnt( attr ) ) ) { succeeded = false; }
				pFontLoader->SaveByCereal( binPath );
			}
		#else
			if ( !pFontLoader->LoadByCereal( MakeFontPathBinary( attr ) ) ) { succeeded = false; }
		#endif // DEBUG_MODE

			auto pTmp = std::make_shared<Donya::Font::Renderer>();
			if ( pTmp->Init( *pFontLoader ) )
			{
				rendererMap.insert( std::make_pair( attr, pTmp ) );
			}
			else
			{
				succeeded = false;
			}

			pFontLoader.reset();
			return succeeded;
		};
		
		bool succeeded = true;

		if ( !Load( FontAttribute::Main ) ) { succeeded = false; }

		return succeeded;
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
