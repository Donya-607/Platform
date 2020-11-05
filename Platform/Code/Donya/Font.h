#pragma once

#include <array>
#include <limits>	// Use std::numeric_limits
#include <string>
#include <vector>

#undef max
#undef min
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include "Serializer.h"
#include "Vector.h"

namespace Donya
{
	/// <summary>
	/// Use UTF-16
	/// </summary>
	namespace Font
	{
		using CharType = unsigned short; // Must be 2 byte for UTF-16

		/// <summary>
		/// Data per character
		/// </summary>
		struct Character
		{
			struct Code
			{
				static constexpr CharType idNull	= 0;
				static constexpr CharType idEnd		= 0xFFFF; // End of code
				static constexpr CharType idReturn	= 0xFFFE; // Line Feed
				static constexpr CharType idTab		= 0xFFFD; // Horizontal Tab
				static constexpr CharType idSpace	= 0xFFFC;
			};
		public:
			Donya::Vector2	uvMin		{ 0.0f, 0.0f };	// Left-Top origin in texture space
			Donya::Vector2	uvMax		{ 1.0f, 1.0f };	// Left-Top origin in texture space
			Donya::Vector2	uvPartSize	{ 1.0f, 1.0f };	// Whole part size of character in texture space
			Donya::Vector2	offset		{ 0.0f, 0.0f };	// Offset from Left-Top
			float			advance		= 0.0f;			// Advancing distance after drawing this character
			int				pageNo		= -1;			// Page number for multiple font textures, 0-based
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( uvMin		),
					CEREAL_NVP( uvMax		),
					CEREAL_NVP( uvPartSize	),
					CEREAL_NVP( offset		),
					CEREAL_NVP( advance		),
					CEREAL_NVP( pageNo		)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		/// <summary>
		/// Store ".fnt" file data
		/// </summary>
		class Holder
		{
		private:
			int									characterCount	= 256;	// Supporting character count
			Donya::Vector2						fontSize		{ 32.0f, 32.0f };

			// The '/' terminated file directory.
			// The file name that contains the extension.
			std::wstring						textureDirectory;		// The directory of textures that ended by '/'(e.g. "Foo/Base/Font/")
			std::vector<std::wstring>			textureNames;			// The texture file names with extension. The directory of these textures is the same.

			static constexpr CharType twoByteLimit = std::numeric_limits<CharType>::max(); // 0xFFFF
			std::vector<Character>				characters;
			std::array<CharType, twoByteLimit>	characterIndices;		// Store the index of the characters. You can access to character info as: characters[characterIndices[static_cast<CharType>(L'‚ ')]]
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( characterCount		),
					CEREAL_NVP( fontSize			),
					CEREAL_NVP( textureNames		),
					CEREAL_NVP( characters			),
					CEREAL_NVP( characterIndices	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
			static constexpr const char *serializeObjectName = "FontHolder";
		public:
			/// <summary>
			/// It returns false if failed. It accepts only .fnt file.
			/// </summary>
			bool LoadFntFile( const std::wstring &fontFilePath );
			/// <summary>
			/// It returns false if failed. It accepts the only file that serialized by Holder.
			/// </summary>
			bool LoadByCereal( const std::string &filePath );
			bool SaveByCereal( const std::string &filePath ) const;
		public:
			/// <summary>
			/// Please set a directory that ended by '/'(e.g. "Foo/Base/Font/")
			/// </summary>
			void SetTextureDirectory( const std::wstring &directory );
		public:
			const Donya::Vector2						&GetFontSize()			const { return fontSize;			}
			const std::wstring							&GetTextureDirectory()	const { return textureDirectory;	}
			const std::vector<std::wstring>				&GetTextureNames()		const { return textureNames;		}
			const std::vector<Character>				&GetCharacters()		const { return characters;			}
			const std::array<CharType, twoByteLimit>	&GetCharacterIndices()	const { return characterIndices;	}
		};

		/// <summary>
		/// Draw a string with Holder's data
		/// </summary>
		class Renderer
		{
		private:
			struct Texture
			{
				size_t			handle = NULL;
				Donya::Vector2	wholeSize;
			};
		private:
			Holder					info;
			std::vector<Texture>	textures; // size() == info.texturenames.size()
		public:
			/// <summary>
			/// Initialize the font data and create textures.<para></para>
			/// "maxInstancesCount" is upper-limit of batching of sprite, and relate in memory usage.<para></para>
			/// It returns false if failed.
			/// </summary>
			bool Init( const Holder &fontData, size_t maxInstanceCount = 512U );
		public:
			/// <summary>
			/// It returns the drawn length in screen space.
			/// Using the Donya::Sprite internally. So you should care the flush timing because the sprite using batching process.
			/// </summary>
			Donya::Vector2 Draw( const wchar_t *string,					const Donya::Vector2 &ssPos, const Donya::Vector2 &pivot01 = { 0.0f, 0.0f }, const Donya::Vector4 &blendColor = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
			/// <summary>
			/// It returns the drawn length in screen space.
			/// Using the Donya::Sprite internally. So you should care the flush timing because the sprite using batching process.
			/// </summary>
			Donya::Vector2 Draw( const std::wstring &string,				const Donya::Vector2 &ssPos, const Donya::Vector2 &pivot01 = { 0.0f, 0.0f }, const Donya::Vector4 &blendColor = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
			/// <summary>
			/// It returns the drawn length in screen space.
			/// Using the Donya::Sprite internally. So you should care the flush timing because the sprite using batching process.
			/// </summary>
			Donya::Vector2 DrawExt( const wchar_t *string,				const Donya::Vector2 &ssPos, const Donya::Vector2 &pivot01 = { 0.0f, 0.0f }, const Donya::Vector2 &drawScale = { 1.0f, 1.0f }, const Donya::Vector4 &blendColor = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
			/// <summary>
			/// It returns the drawn length in screen space.
			/// Using the Donya::Sprite internally. So you should care the flush timing because the sprite using batching process.
			/// </summary>
			Donya::Vector2 DrawExt( const std::wstring &string,			const Donya::Vector2 &ssPos, const Donya::Vector2 &pivot01 = { 0.0f, 0.0f }, const Donya::Vector2 &drawScale = { 1.0f, 1.0f }, const Donya::Vector4 &blendColor = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
		private: // FIXME: Font::Renderer::DrawStretched, Ext is not working expectedly.
			/// <summary>
			/// Draw the string by keeping the size within the bounds of "ssDrawSize".
			/// If you set minus value(e.g. -1.0f) to "ssDrawSize", It will using the original size.<para></para>
			/// It returns the drawn length in screen space.
			/// Using the Donya::Sprite internally. So you should care the flush timing because the sprite using batching process.
			/// </summary>
			Donya::Vector2 DrawStretched( const wchar_t *string,			const Donya::Vector2 &ssPos, const Donya::Vector2 &ssDrawSize, const Donya::Vector2 &pivot01 = { 0.0f, 0.0f }, const Donya::Vector4 &blendColor = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
			/// <summary>
			/// Draw the string by keeping the size within the bounds of "ssDrawSize".
			/// If you set minus value(e.g. -1.0f) to "ssDrawSize", It will using the original size.<para></para>
			/// It returns the drawn length in screen space.
			/// Using the Donya::Sprite internally. So you should care the flush timing because the sprite using batching process.
			/// </summary>
			Donya::Vector2 DrawStretched( const std::wstring &string,		const Donya::Vector2 &ssPos, const Donya::Vector2 &ssDrawSize, const Donya::Vector2 &pivot01 = { 0.0f, 0.0f }, const Donya::Vector4 &blendColor = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
			/// <summary>
			/// Draw the string by keeping the size within the bounds of "ssDrawSize".
			/// If you set minus value(e.g. -1.0f) to "ssDrawSize", It will using the original size.<para></para>
			/// It returns the drawn length in screen space.
			/// Using the Donya::Sprite internally. So you should care the flush timing because the sprite using batching process.
			/// </summary>
			Donya::Vector2 DrawStretchedExt( const wchar_t *string,		const Donya::Vector2 &ssPos, const Donya::Vector2 &ssDrawSize, const Donya::Vector2 &pivot01 = { 0.0f, 0.0f }, const Donya::Vector2 &drawScale = { 1.0f, 1.0f }, const Donya::Vector4 &blendColor = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
			/// <summary>
			/// Draw the string by keeping the size within the bounds of "ssDrawSize".
			/// If you set minus value(e.g. -1.0f) to "ssDrawSize", It will using the original size.<para></para>
			/// It returns the drawn length in screen space.
			/// Using the Donya::Sprite internally. So you should care the flush timing because the sprite using batching process.
			/// </summary>
			Donya::Vector2 DrawStretchedExt( const std::wstring &string,	const Donya::Vector2 &ssPos, const Donya::Vector2 &ssDrawSize, const Donya::Vector2 &pivot01 = { 0.0f, 0.0f }, const Donya::Vector2 &drawScale = { 1.0f, 1.0f }, const Donya::Vector4 &blendColor = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
		};
	}
}
CEREAL_CLASS_VERSION( Donya::Font::Character,	0 )
CEREAL_CLASS_VERSION( Donya::Font::Holder,		0 )
