#pragma once

#include <string>
#include <vector>

namespace shadertrans
{

class ShaderPreprocess
{
public:
	static bool RemoveIncludes(std::string& code);

	static std::string ReplaceIncludes(const std::string& source_code);

private:
	static std::string LoadWithInclude(std::istream& cin);

}; // ShaderPreprocess

}