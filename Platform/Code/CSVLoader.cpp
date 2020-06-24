#include "CSVLoader.h"

#include <fstream>
#include <sstream>

#include "StageFormat.h"

bool CSVLoader::Load( const std::string &filePath, const char delimiter )
{
	// Reference: https://www.tetsuyanbo.net/tetsuyanblog/23821

	Clear();

	std::ifstream fs{ filePath };
	if ( !fs.is_open() ) { return false; }
	// else

	std::string cell;
	std::string currentLine;
	std::vector<std::string>				row;
	std::vector<std::vector<std::string>>	table;
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

		table.emplace_back( row );
		row.clear();
	}

	Assign( table );

	return true;
}
void CSVLoader::Clear()
{
	data.clear();
}
const std::vector<std::vector<int>> &CSVLoader::Get() const
{
	return data;
}
void CSVLoader::Assign( const std::vector<std::vector<std::string>> &source )
{
	data.clear();

	const size_t rowCount = source.size();
	data.resize( rowCount );

	for ( size_t r = 0; r < rowCount; ++r )
	{
		const size_t columnCount = source[r].size();
		data[r].resize( columnCount );

		for ( size_t c = 0; c < columnCount; ++c )
		{
			const auto &str = source[r][c];
			data[r][c] = ( str.empty() ) ? StageFormat::EmptyValue : std::stoi( str );
		}
	}
}
#if USE_IMGUI
void CSVLoader::ShowDataToImGui( const char *emptyCharacter ) const
{
	std::string line;
	const size_t rowCount = data.size();
	for ( size_t r = 0; r < rowCount; ++r )
	{
		line = "";

		const size_t columnCount = data[r].size();
		for ( size_t c = 0; c < columnCount; ++c )
		{
			const auto &cell = data[r][c];
			line += ( cell == StageFormat::EmptyValue ) ? emptyCharacter : std::to_string( cell );
			line += ",";
		}

		ImGui::Text( line.c_str() );
	}
}
#endif // USE_IMGUI
