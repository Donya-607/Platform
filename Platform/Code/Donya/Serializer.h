#pragma once

#include <assert.h>
#include <fstream>
#include <memory>
#include <sstream>

#undef max
#undef min

#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"

namespace Donya
{
	class Serializer
	{
	public:
		enum class Extension
		{
			Binary = 0,
			JSON,
		};
	private:	// Use for Begin() ~ End() process.
		Extension	ext;
		std::unique_ptr<std::ifstream>					pIfs;
		std::unique_ptr<std::ofstream>					pOfs;
		std::unique_ptr<std::stringstream>				pSS;
		std::unique_ptr<cereal::BinaryInputArchive>		pBinInArc;
		std::unique_ptr<cereal::JSONInputArchive>		pJsonInArc;
		std::unique_ptr<cereal::BinaryOutputArchive>	pBinOutArc;
		std::unique_ptr<cereal::JSONOutputArchive>		pJsonOutArc;
		bool isValid;	// It will be true while Begin() ~ End(), else false.
	public:
		Serializer() : ext( Extension::Binary ), pIfs( nullptr ), pSS( nullptr ), pBinInArc( nullptr ), pJsonInArc( nullptr ), pBinOutArc( nullptr ), pJsonOutArc( nullptr ), isValid( false )
		{}
	private:
		template<class InputArchiveType, class SerializeObject>
		bool LoadImpl( int fileOpenMode, const char *filePath, const char *objectName, SerializeObject &instance ) const
		{
			std::ifstream ifs( filePath, fileOpenMode );
			if ( !ifs.is_open() ) { return false; }
			// else

			std::stringstream ss{};
			ss << ifs.rdbuf();

			{
				InputArchiveType archive( ss );
				archive( cereal::make_nvp( objectName, instance ) );
			}

			ifs.close();
			ss.clear();

			return true;
		}
		template<class OutputArchiveType, class SerializeObject>
		bool SaveImpl( int fileOpenMode, const char *filePath, const char *objectName, SerializeObject &instance ) const
		{
			std::ofstream ofs( filePath, fileOpenMode );
			if ( !ofs.is_open() ) { return false; }
			// else

			std::stringstream ss{};

			{
				OutputArchiveType archive( ss );
				archive( cereal::make_nvp( objectName, instance ) );
			}

			ofs << ss.str();

			ofs.close();
			ss.clear();

			return true;
		}
	public:
		/// <summary>
		/// The "objectName" is used to identify the object in reading files.
		/// </summary>
		template<class SerializeObject>
		bool LoadBinary	( SerializeObject &instance, const char *filePath, const char *objectName ) const
		{
			constexpr int openMode = std::ios::in | std::ios::binary;
			return LoadImpl<cereal::BinaryInputArchive>( openMode, filePath, objectName, instance );
		}
		/// <summary>
		/// The "objectName" is used to identify the object in reading files.
		/// </summary>
		template<class SerializeObject>
		bool LoadJSON	( SerializeObject &instance, const char *filePath, const char *objectName ) const
		{
			constexpr int openMode = std::ios::in;
			return LoadImpl<cereal::JSONInputArchive>( openMode, filePath, objectName, instance );
		}
		/// <summary>
		/// The "objectName" is used to identify the object in reading files.
		/// </summary>
		template<class SerializeObject>
		bool SaveBinary	( SerializeObject &instance, const char *filePath, const char *objectName ) const
		{
			constexpr int openMode = std::ios::out | std::ios::binary;
			return SaveImpl<cereal::BinaryOutputArchive>( openMode, filePath, objectName, instance );
		}
		/// <summary>
		/// The "objectName" is used to identify the object in reading files.
		/// </summary>
		template<class SerializeObject>
		bool SaveJSON	( SerializeObject &instance, const char *filePath, const char *objectName ) const
		{
			constexpr int openMode = std::ios::out;
			return SaveImpl<cereal::JSONOutputArchive>( openMode, filePath, objectName, instance );
		}
	public: // TODO: Separate these loading method between extension(binary version, json versoin, etc) also
		bool LoadBegin( Extension extension, const char *filePath )
		{
			if ( isValid )
			{
				/*
				If you called LoadBegin(),
				you must call the LoadEnd() after the LoadBegin().
				*/
				assert( 0 && "Serializer : Error ! LoadBegin() called before LoadEnd() !" );
				return false;
			}
			// else

			auto openMode = ( extension == Extension::Binary ) ? std::ios::in | std::ios::binary : std::ios::in;
			pIfs = std::make_unique<std::ifstream>( filePath, openMode );
			if ( !pIfs->is_open() ) { return false; }
			// else

			pSS = std::make_unique<std::stringstream>();
			*pSS << pIfs->rdbuf();

			ext = extension;
			switch ( extension )
			{
			case Extension::Binary:
				pBinInArc  = std::make_unique<cereal::BinaryInputArchive>( *pSS );
				break;
			case Extension::JSON:
				pJsonInArc = std::make_unique<cereal::JSONInputArchive>( *pSS );
				break;
			default:
				return false;
			}

			isValid = true;

			return true;
		}
		template<class SerializeObject>
		bool LoadPart( const char *objectName, SerializeObject &instance )
		{
			if ( !isValid ) { return false; }
			// else

			switch ( ext )
			{
			case Extension::Binary:
				{
					( *pBinInArc )( cereal::make_nvp( objectName, instance ) );
				}
				break;
			case Extension::JSON:
				{
					( *pJsonInArc )( cereal::make_nvp( objectName, instance ) );
				}
				break;
			default:
				return false;
			}

			return true;
		}
		void LoadEnd()
		{
			if ( !isValid ) { return; }
			// else

			switch ( ext )
			{
			case Extension::Binary:
				pBinInArc.reset( nullptr );
				break;
			case Extension::JSON:
				pJsonInArc.reset( nullptr );
				break;
			default:
				break;
			}

			pIfs->close();
			pSS->clear();

			pIfs.reset( nullptr );
			pSS.reset( nullptr );

			isValid  = false;
		}
		bool SaveBegin( Extension extension, const char *filePath )
		{
			if ( isValid )
			{
				/*
				If you called SaveBegin(),
				you must call the SaveEnd() after the SaveBegin().
				*/
				assert( 0 && "Serializer : Error ! SaveBegin() called before SaveEnd() !" );
				return false;
			}
			// else

			pSS = std::make_unique<std::stringstream>();

			ext = extension;
			switch ( extension )
			{
			case Extension::Binary:
				pBinOutArc  = std::make_unique<cereal::BinaryOutputArchive>( *pSS );
				break;
			case Extension::JSON:
				pJsonOutArc = std::make_unique<cereal::JSONOutputArchive>( *pSS );
				break;
			default:
				return false;
			}

			auto openMode = ( extension == Extension::Binary ) ? std::ios::out | std::ios::binary : std::ios::out;
			pOfs = std::make_unique<std::ofstream>( filePath, openMode );
			if ( !pOfs->is_open() ) { return false; }
			// else

			isValid = true;

			return true;
		}
		template<class SerializeObject>
		bool SavePart( const char *objectName, SerializeObject &instance )
		{
			if ( !isValid ) { return false; }
			// else

			switch ( ext )
			{
			case Extension::Binary:
				{
					( *pBinOutArc )( cereal::make_nvp( objectName, instance ) );
				}
				break;
			case Extension::JSON:
				{
					( *pJsonOutArc )( cereal::make_nvp( objectName, instance ) );
				}
				break;
			default:
				return false;
			}

			return true;
		}
		void SaveEnd()
		{
			if ( !isValid ) { return; }
			// else

			switch ( ext )
			{
			case Extension::Binary:
				pBinOutArc.reset( nullptr );
				break;
			case Extension::JSON:
				pJsonOutArc.reset( nullptr );
				break;
			default:
				break;
			}

			*pOfs << pSS->str();

			pOfs->close();
			pSS->clear();

			pOfs.reset( nullptr );
			pSS.reset( nullptr );

			isValid = false;
		}
	};
}
