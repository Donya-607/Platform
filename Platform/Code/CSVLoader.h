#pragma once

#include <string>
#include <vector>

#include "Donya/UseImGui.h" // Use USE_IMGUI macro

class CSVLoader
{
public:
	static constexpr int emptyValue = INT_MAX;
private:
	std::vector<std::vector<int>> data;
public:
	/// <summary>
	/// Return true if the load was succeeded.
	/// </summary>
	bool Load( const std::string &filePath, const char delimiter = ',' );
	void Clear();
public:
	const std::vector<std::vector<int>> &Get() const;
private:
	void Assign( const std::vector<std::vector<std::string>> &source );
public:
#if USE_IMGUI
	/// <summary>
	/// You should call this between ImGui::Begin() and ImGui::End()
	/// </summary>
	void ShowDataToImGui( const char *emptyCharacter = "x" ) const;
#endif // USE_IMGUI
};
