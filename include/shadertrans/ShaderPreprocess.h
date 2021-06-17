#pragma once

#include "shadertrans/ShaderStage.h"

#include <string>
#include <vector>

namespace shadertrans
{

class ShaderPreprocess
{
public:
	static std::string PrepareGLSL(ShaderStage stage, const std::string& source_code);

	// todo: use IR info
	// fix glsl code trans from hlsl
	static std::string PrepareHLSL(const std::string& source_code, const std::string& entry_point);

	static std::string RemoveIncludes(const std::string& source_code, 
		std::vector<std::string>& include_paths);

	static std::string ReplaceIncludes(const std::string& source_code);

private:
	static std::string LoadWithInclude(std::istream& cin);

	static bool GetPathFromLine(const std::string& line, std::string& path);

	static void StringReplace(std::string& str, const std::string& from, const std::string& to);

}; // ShaderPreprocess

}