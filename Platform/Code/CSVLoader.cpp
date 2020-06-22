#include "CSVLoader.h"

#include <fstream>
#include <sstream>

bool CSVLoader::Load( const std::string &filePath, const char delimiter )
{
	// Reference: https://www.tetsuyanbo.net/tetsuyanblog/23821

	Clear();

	std::fstream fs{ filePath };
	if ( !fs.is_open() ) { return false; }
	// else

	std::string					cell;
	std::string					currentLine;
	std::vector<std::string>	row;
	while ( !fs.eof() )
	{
		fs >> currentLine;
		if ( fs.eof() ) { break; }
		// else
		
		std::istringstream stream{ currentLine };
		while ( std::getline( stream, cell, delimiter ) )
		{
			row.emplace_back( cell );
		}

		data.emplace_back( row );
		row.clear();
	}

	return true;
}
void CSVLoader::Clear()
{
	data.clear();
}
const std::vector<std::vector<std::string>> &CSVLoader::Get() const
{
	return data;
}
