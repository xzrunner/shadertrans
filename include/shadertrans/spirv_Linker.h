#pragma once

#include "shadertrans/ShaderStage.h"

#include <vector>
#include <memory>
#include <string>

namespace shadertrans
{
namespace spirv
{

struct Module;

class Linker
{
public:
	void AddModule(ShaderStage stage, const std::string& glsl);

	void Link();

private:
	std::vector<std::shared_ptr<Module>> m_modules;

}; // Linker

}
}