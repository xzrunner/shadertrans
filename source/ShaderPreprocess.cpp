#include "shadertrans/ShaderPreprocess.h"

#include <sstream>
#include <filesystem>
#include <fstream>

namespace shadertrans
{

std::string ShaderPreprocess::PrepareGLSL(ShaderStage stage, const std::string& source_code)
{
	std::string ret = source_code;

	if (ret.find("#version") == std::string::npos) {
		if (stage == ShaderStage::ComputeShader) {
			ret.insert(0, "#version 430\n");
		} else {
			ret.insert(0, "#version 330 core\n");
		}
	}

	auto itr = ret.find("#include");
	if (itr != std::string::npos) {
		ret.insert(itr, "#extension GL_GOOGLE_include_directive : require\n");
	}

	return ret;
}

std::string ShaderPreprocess::PrepareHLSL(const std::string& source_code, const std::string& entry_point)
{
	std::stringstream ss(source_code);

	std::string out;
	std::string line;
	while (std::getline(ss, line))
	{
		std::string path;
		if (line == "struct type_Globals") {
			out += "uniform __UBO__\n";
		} else if (line == "uniform type_Globals _Globals;") {
			;
		} else if (line == "void main()") {
			break;
		} else if (line.find("layout(location = ") != std::string::npos) {
			;
		} else {
			out += line + '\n';
		}
	}

	StringReplace(out, " src_" + entry_point + "(", " " + entry_point + "(");
	StringReplace(out, "_Globals.", "");

	return out;
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

void ShaderPreprocess::StringReplace(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty()) {
		return;
	}
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
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