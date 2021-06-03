#include "shadertrans/ShaderPreprocess.h"

#include <sstream>
#include <filesystem>
#include <fstream>

namespace shadertrans
{

bool ShaderPreprocess::RemoveIncludes(std::string& code)
{
	if (code.find("#include") == std::string::npos) {
		return false;
	}

	std::stringstream ss(code);

	std::string out;
	std::string line;
	while (std::getline(ss, line)) {
		if (line.find("#include") == std::string::npos) {
			out += line + '\n';
		}
	}

	code = out;

	return true;
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
		if (line.find("#include") != std::string::npos)
		{
			auto start = line.find_first_of('"');
			auto end = line.find_last_of('"');
			std::string path = line.substr(start + 1, end - start - 1);

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

}