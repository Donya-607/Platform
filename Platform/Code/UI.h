#pragma once

#include <string>

#include "Donya/Serializer.h"
#include "Donya/Vector.h"
#include "Donya/UseImgui.h"

/// <summary>
/// Provides basic drawing a 2D sprite.
/// </summary>
struct UIObject
{
private:
	size_t			sprite = 0;
public:
	float			degree = 0.0f;
	float			alpha  = 1.0f;
	Donya::Vector2	pos;					// Screen space
	Donya::Vector2	origin{ 0.5f, 0.5f };	// 0.0f ~ 1.0f
	Donya::Vector2	texPos;					// Texture space
	Donya::Vector2	texSize;				// Whole size, Texture space
	Donya::Vector2	scale{ 1.0f, 1.0f };
	Donya::Vector3	color{ 1.0f, 1.0f, 1.0f };
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( degree	),
			CEREAL_NVP( alpha	),
			CEREAL_NVP( pos		),
			CEREAL_NVP( origin	),
			CEREAL_NVP( texPos	),
			CEREAL_NVP( texSize	),
			CEREAL_NVP( scale	),
			CEREAL_NVP( color	)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	bool LoadSprite( const std::wstring &filePath, size_t maxInstanceCount );

	bool Draw( float drawDepth = 1.0f ) const;
	bool DrawPart( float drawDepth = 1.0f ) const;
public:
	/// <summary>
	/// Returns whole size. or -1 if the sprite didn't loaded.
	/// </summary>
	Donya::Int2 GetSpriteSize() const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( UIObject, 0 )
