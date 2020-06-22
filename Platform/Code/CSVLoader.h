#pragma once

#include <string>
#include <vector>

class CSVLoader
{
private:
	std::vector<std::vector<std::string>> data;
public:
	/// <summary>
	/// Return true if the load was succeeded.
	/// </summary>
	bool Load( const std::string &filePath, const char delimiter = ',' );
	void Clear();
public:
	const std::vector<std::vector<std::string>> &Get() const;
};
