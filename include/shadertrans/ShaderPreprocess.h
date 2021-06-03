#pragma once

#include <string>
#include <vector>

namespace shadertrans
{

class ShaderPreprocess
{
public:
	static std::string AddIncludeMacro(const std::string& source_code);

	static std::string RemoveIncludes(const std::string& source_code, 
		std::vector<std::string>& include_paths);

	static std::string ReplaceIncludes(const std::string& source_code);

private:
	static std::string LoadWithInclude(std::istream& cin);

	static bool GetPathFromLine(const std::string& line, std::string& path);

}; // ShaderPreprocess

}