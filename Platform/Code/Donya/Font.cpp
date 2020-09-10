#include "Font.h"

#include <algorithm>	// Use std::sort

#include "Constant.h"	// Use scast macro
#include "Donya.h"
#include "Sprite.h"
#include "Useful.h"		// Use some functions related to file path

namespace Donya
{
	namespace Font
	{
		bool Holder::LoadFntFile( const std::wstring &filePath )
		{
			if ( !Donya::IsExistFile( filePath ) )
			{
				_ASSERT_EXPR( 0, L"Error: Passed .fnt file is not found." );
				return false;
			}
			// else

			const std::wstring fullPath = Donya::ToFullPath( filePath );

			std::unique_ptr<char[]> pFntData = nullptr;
			const long codeLength = Donya::ReadByteCode( &pFntData, fullPath );
			if ( codeLength <= 0 || !pFntData )
			{
				_ASSERT_EXPR( 0, L"Failed: Read .fnt file is failed!" );
				return false;
			}
			// else

			constexpr const char *delimiters		= " ,=\"\n\r";
			constexpr const char *delimitersNoSpace	= ",=\"\n\r";

			char *pToken		= nullptr;
			char *pNextToken	= nullptr;

			pToken = strtok_s( pFntData.get(), delimiters, &pNextToken );
			if ( strcmp( pToken, "info" ) != 0 )
			{
				_ASSERT_EXPR( 0, L"Error: This file does not match the .fnt format!" );
				return false;
			}
			// else

			auto ReadNextToken = [&pToken, &pNextToken]( const char *delimiters )
			{
				pToken = strtok_s( nullptr, delimiters, &pNextToken );
			};

		#pragma region information
		
			// face
			ReadNextToken( delimiters );
			ReadNextToken( delimitersNoSpace );
			const char *face = pToken; // Used font name

			// size
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int size = atoi( pToken );

			// bold
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int bold = atoi( pToken );

			// italic
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int italic = atoi( pToken );

			// charset
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const char* charset = pToken;

			// unicode
			if ( strcmp( charset, "unicode" ) == 0 )
			{ charset = ""; }
			else
			{ ReadNextToken( delimiters ); }
			ReadNextToken( delimiters );
			const int unicode = atoi( pToken );

			// stretchH
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int stretchH = atoi( pToken );

			// smooth
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int smooth = atoi( pToken );

			// aa
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int aa = atoi( pToken );

			// padding
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int padding1 = atoi( pToken );
			ReadNextToken( delimiters );
			const int padding2 = atoi( pToken );
			ReadNextToken( delimiters );
			const int padding3 = atoi( pToken );
			ReadNextToken( delimiters );
			const int padding4 = atoi( pToken );

			// spacing
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int spacing0 = atoi( pToken );
			ReadNextToken( delimiters );
			const int spacing1 = atoi( pToken );

			// outline
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int outline = atoi( pToken );

		// region information
		#pragma endregion

		#pragma region common
		
			// common
			ReadNextToken( delimiters );

			// lineHeight
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int lineHeight = atoi( pToken );

			// base
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int base = atoi( pToken );

			// scaleW
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int scaleW = atoi( pToken );

			// scaleH
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int scaleH = atoi( pToken );

			// pages
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int pages = atoi( pToken );

			// packed
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int packed = atoi( pToken );

			// alphaChnl
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int alphaChnl = atoi( pToken );

			// redChnl
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int redChnl = atoi( pToken );

			// greenChnl
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int greenChnl = atoi( pToken );

			// blueChnl
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int blueChnl = atoi( pToken );

		// region common
		#pragma endregion
			
		#pragma region page
			
			SetTextureDirectory( Donya::ExtractFileDirectoryFromFullPath( fullPath ) );
			textureNames.resize( pages );
			for ( int i = 0; i < pages; ++i )
			{
				// page
				ReadNextToken( delimiters );

				// id
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int id = atoi( pToken );

				// file
				ReadNextToken( delimiters );
				ReadNextToken( delimitersNoSpace );
				const char* file = pToken;

				// textureNames[i] = Donya::MultiToWide( file );
				textureNames[i] = Donya::UTF8ToWide( file );
			}

		// region page
		#pragma endregion

		#pragma region char

			// chars
			ReadNextToken( delimiters );

			// count
			ReadNextToken( delimiters );
			ReadNextToken( delimiters );
			const int count = atoi( pToken );


			fontSize.x		= fabsf( scast<float>( size			) );
			fontSize.y		= fabsf( scast<float>( lineHeight	) );
			characterCount	= count + 1;
			characters.resize( characterCount );
			characterIndices.fill( 0 );
			characterIndices[0x0000] = Character::Code::idEnd;
			characterIndices[0x0009] = Character::Code::idTab;
			characterIndices[0x000a] = Character::Code::idReturn;
			characterIndices[0x0020] = Character::Code::idSpace;


			int charIndex = 1;
			for ( int i = 0; i < count; ++i )
			{
				// char
				ReadNextToken( delimiters );

				// id
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int id = atoi( pToken );

				// x
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int x = atoi( pToken );

				// y
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int y = atoi( pToken );

				// width
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int width = atoi( pToken );

				// height
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int height = atoi( pToken );

				// xoffset
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int xoffset = atoi( pToken );

				// yoffset
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int yoffset = atoi( pToken );

				// xadvance
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int xadvance = atoi( pToken );

				// page
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int page = atoi( pToken );

				// chnl
				ReadNextToken( delimiters );
				ReadNextToken( delimiters );
				const int chnl = atoi( pToken );

				// Store the information if supported
				if ( 0xFFFF < id ) { continue; }
				// else

				characterIndices[id] = scast<CharType>( i + 1 );

				Character &element		= characters[charIndex];
				element.uvMin.x			= scast<float>( x			) / scast<float>( scaleW );
				element.uvMin.y			= scast<float>( y			) / scast<float>( scaleH );
				element.uvMax.x			= scast<float>( x + width	) / scast<float>( scaleW );
				element.uvMax.y			= scast<float>( y + height	) / scast<float>( scaleH );
				element.uvPartSize.x	= scast<float>( width		);
				element.uvPartSize.y	= scast<float>( height		);
				element.offset.x		= scast<float>( xoffset		);
				element.offset.y		= scast<float>( yoffset		);
				element.advance			= scast<float>( xadvance	);
				element.pageNo			= page;

				charIndex++;
			}

		// region char
		#pragma endregion

			return true;
		}
		bool Holder::LoadByCereal( const std::string &filePath )
		{
			Donya::Serializer tmp;
			const bool result = tmp.LoadBinary( *this, filePath.c_str(), serializeObjectName );

			if ( result )
			{
				const std::wstring widePath = Donya::UTF8ToWide( filePath );
				const std::wstring fullPath = Donya::ToFullPath( widePath );
				SetTextureDirectory( Donya::ExtractFileDirectoryFromFullPath( fullPath ) );
			}

			return result;
		}
		bool Holder::SaveByCereal( const std::string &filePath ) const
		{
			Donya::Serializer tmp;
			return tmp.SaveBinary( *this, filePath.c_str(), serializeObjectName );
		}
		void Holder::SetTextureDirectory( const std::wstring &directory )
		{
			textureDirectory = directory;
			if ( textureDirectory.back() != L'/' )
			{
				textureDirectory += L'/';
			}
		}


		bool Renderer::Init( const Holder &fontData, size_t maxInstanceCount )
		{
			info = fontData;

			const auto   &textureDir	= info.GetTextureDirectory();
			const auto   &textureNames	= info.GetTextureNames();
			const size_t textureCount	= textureNames.size();

			std::wstring filePath;
			textures.resize( textureCount );
			for ( size_t i = 0; i < textureCount; ++i )
			{
				filePath = textureDir + textureNames[i];
				if ( !Donya::IsExistFile( filePath ) )
				{
				#if DEBUG_MODE
					std::wstring errMsg = L"Error: Font texture is not found!\n";
					errMsg += L"Search directory: [";
					errMsg += textureDir;
					errMsg += L"]\n";
					errMsg += L"Search fileName: [";
					errMsg += textureNames[i];
					errMsg += L"]";
					Donya::ShowMessageBox( errMsg, L"File load is failed", MB_OK );
				#endif // DEBUG_MODE
					continue;
				}
				// else

				textures[i].handle		= Donya::Sprite::Load( filePath, maxInstanceCount );
				textures[i].wholeSize	= Donya::Sprite::GetTextureSizeF( textures[i].handle );
			}


			for ( const auto &it : textures )
			{ if ( it.handle == NULL ) { return false; } }
			// else
			return true;
		}
		namespace
		{
			constexpr Donya::Vector2 defaultScale	{ 1.0f, 1.0f };
			constexpr Donya::Vector2 defaultSize	{-1.0f,-1.0f };
		}
		void Renderer::Draw( const wchar_t *string,					const Donya::Vector2 &ssPos, const Donya::Vector2 &pivot, const Donya::Vector4 &color ) const
		{
			DrawStretchedExt( string,			ssPos, defaultSize, pivot, defaultScale, color );
		}
		void Renderer::Draw( const std::wstring &string,			const Donya::Vector2 &ssPos, const Donya::Vector2 &pivot, const Donya::Vector4 &color ) const
		{
			DrawStretchedExt( string.c_str(),	ssPos, defaultSize, pivot, defaultScale, color );
		}
		void Renderer::DrawExt( const wchar_t *string,				const Donya::Vector2 &ssPos, const Donya::Vector2 &pivot, const Donya::Vector2 &scale, const Donya::Vector4 &color ) const
		{
			DrawStretchedExt( string,			ssPos, defaultSize, pivot, scale, color );
		}
		void Renderer::DrawExt( const std::wstring &string,			const Donya::Vector2 &ssPos, const Donya::Vector2 &pivot, const Donya::Vector2 &scale, const Donya::Vector4 &color ) const
		{
			DrawStretchedExt( string.c_str(),	ssPos, defaultSize, pivot, scale, color );
		}
		void Renderer::DrawStretched( const wchar_t *string,		const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &pivot, const Donya::Vector4 &color ) const
		{
			DrawStretchedExt( string,			ssPos, ssSize, pivot, defaultScale, color );
		}
		void Renderer::DrawStretched( const std::wstring &string,	const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &pivot, const Donya::Vector4 &color ) const
		{
			DrawStretchedExt( string.c_str(),	ssPos, ssSize, pivot, defaultScale, color );
		}
		void Renderer::DrawStretchedExt( const wchar_t *string,		const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &pivot, const Donya::Vector2 &scale, const Donya::Vector4 &color ) const
		{
			// FIXME: "ssSize"(screen space size) specification is not working if specified valid value(not defaultSize, not minus value).
			/*
			Detail:
			The currently fitting process code tries fit the characters to whole string size.
			But in the correct way, I must fit the characters to character size in part of "ssSize", then offset appropriate position.

			|string|                |
			       ^original-width  ^extended-width(i.e. "ssSize")
			*/

			const auto &characters			= info.GetCharacters();
			const auto &charIndices			= info.GetCharacterIndices();
			const Donya::Vector2 &origin	= ssPos;
			const Donya::Vector2 &fontSize	= info.GetFontSize();

			const size_t strLength = wcslen( string );
			if ( strLength < 1 ) { return; }
			// else

			struct Instance
			{
				size_t			handle = NULL;
				Donya::Vector2	pos;	// Left-Top
				Donya::Vector2	size;	// Whole size
				Donya::Vector2	uvPos;	// Left-Top
				Donya::Vector2	uvSize;	// Whole size
			};
			std::vector<Instance> elements{ strLength };

			Donya::Vector2 drawPos  = origin; // Next drawing position of character (screen space)
			Donya::Vector2 drawSize = { 0.0f, fontSize.y * scale.y }; // Sum of character widths in the string (screen space)

			// Set drawing informations to "elements"
			for ( size_t i = 0; i < strLength; ++i )
			{
				const CharType character = scast<CharType>( string[i] );
				const CharType charCode  = charIndices[character];

				if ( charCode == Character::Code::idEnd ) { break; }
				// else
				if ( charCode == Character::Code::idReturn )
				{
					drawPos.x  =  origin.x;
					drawPos.y  += fontSize.y * scale.y;
					drawSize.y += fontSize.y * scale.y;
					continue;
				}
				// else
				if ( charCode == Character::Code::idTab )
				{
					drawPos.x  += fontSize.x * scale.x * 4.0f;
					drawSize.x += fontSize.x * scale.x * 4.0f;
					continue;
				}
				// else
				if ( charCode == Character::Code::idSpace )
				{
					drawPos.x  += fontSize.x * scale.x;
					drawSize.x += fontSize.x * scale.x;
					continue;
				}
				// else

				const auto &data = characters[charCode];
				if ( data.pageNo < 0 || scast<int>( textures.size() ) <= data.pageNo ) { continue; }
				// else

				const auto &usingTexture = textures[data.pageNo];
				elements[i].handle		= usingTexture.handle;
				elements[i].pos			= drawPos;
				elements[i].pos.x		+= data.offset.x * scale.x;
				elements[i].pos.y		+= data.offset.y * scale.y;
				elements[i].size		= data.uvPartSize;
				elements[i].uvPos		= data.uvMin;
				elements[i].uvSize		= data.uvMax - data.uvMin;
				// Donya::Sprite::DrawXX requires uv parameter as texture space
				elements[i].uvPos.x		*= usingTexture.wholeSize.x;
				elements[i].uvPos.y		*= usingTexture.wholeSize.y;
				elements[i].uvSize.x	*= usingTexture.wholeSize.x;
				elements[i].uvSize.y	*= usingTexture.wholeSize.y;

				drawPos.x  += data.advance * scale.x;
				drawSize.x += data.advance * scale.x;
			}

			// Sort for sprite batching
			auto AscendingOrder = []( const Instance &lhs, const Instance &rhs )
			{
				return lhs.handle < rhs.handle;
			};
			std::sort( elements.begin(), elements.end(), AscendingOrder );
			
			// Draw
			const Donya::Vector2 center // Texture space
			{
				pivot.x * drawSize.x,
				pivot.y * drawSize.y,
			};
			const Donya::Vector2 drawScale
			{
				( ssSize.x < 0.0f ) ? scale.x : ssSize.x / drawSize.x,
				( ssSize.y < 0.0f ) ? scale.y : ssSize.y / drawSize.y,
			};
			for ( const auto &it : elements )
			{
				Donya::Sprite::DrawGeneralExt
				(
					it.handle,
					it.pos.x,		it.pos.y,
					it.size.x,		it.size.y,
					it.uvPos.x,		it.uvPos.y,
					it.uvSize.x,	it.uvSize.y,
					drawScale.x,	drawScale.y,
					0.0f,			center,
					color.w, color.x, color.y, color.z
				);
			}
		}
		void Renderer::DrawStretchedExt( const std::wstring &string,	const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &pivot, const Donya::Vector2 &scale, const Donya::Vector4 &color ) const
		{
			DrawStretchedExt( string.c_str(),	ssPos, ssSize, pivot, scale, color );
		}
	}
}
