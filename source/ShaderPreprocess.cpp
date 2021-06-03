#include "shadertrans/ShaderPreprocess.h"

#include <sstream>
#include <filesystem>
#include <fstream>

namespace shadertrans
{

std::string ShaderPreprocess::AddIncludeMacro(const std::string& source_code)
{
	std::string ret = source_code;

	auto itr = source_code.find("#include");
	if (itr != std::string::npos) {
		ret.insert(itr, "#extension GL_GOOGLE_include_directive : require\n");
	}

	return ret;
}

std::string ShaderPreprocess::RemoveIncludes(const std::string& source_code, std::vector<std::string>& include_paths)
{
	if (strncmp(source_code.c_str(), "#include", strlen("#include")) != 0) {
		return source_code;
	}

	std::stringstream ss(source_code);

	std::string out;
	std::string line;
	while (std::getline(ss, line)) 
	{
		std::string path;
		if (GetPathFromLine(line, path)) {
			include_paths.push_back(path);
		} else {
			out += line + '\n';
		}
	}

	return out;
}

std::string ShaderPreprocess::ReplaceIncludes(const std::string& source_code)
{
	if (source_code.find("#include") == std::string::npos) {
		return source_code;
	} else {
		std::stringstream ss(source_code);
		return LoadWithInclude(ss);
	}
}

std::string ShaderPreprocess::LoadWithInclude(std::istream& cin)
{
	std::string ret;
	std::string line;
	while (std::getline(cin, line))
	{
		std::string path;
		if (GetPathFromLine(line, path))
		{
			// todo: include files search path
			std::ifstream fin(path);
			if (fin.is_open()) {
				ret += LoadWithInclude(fin);
			}
		}
		else
		{
			ret += line + '\n';
		}
	}

	return ret;
}

bool ShaderPreprocess::GetPathFromLine(const std::string& line, std::string& path)
{
	if (strncmp(line.c_str(), "#include", strlen("#include")) != 0) {
		return false;
	}

	auto start = line.find_first_of('"');
	auto end = line.find_last_of('"');
	path = line.substr(start + 1, end - start - 1);

	return true;
}

}