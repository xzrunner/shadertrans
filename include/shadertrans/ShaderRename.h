#pragma once

#include "shadertrans/ShaderStage.h"

#include <vector>
#include <string>
#include <memory>

namespace spirv_cross { class CompilerGLSL; }

namespace shadertrans
{

class ShaderRename
{
public:
	ShaderRename(std::vector<unsigned int>& spirv);
	~ShaderRename();

	bool FillingUBOInstName();
	bool RenameSampledImages();

	std::vector<unsigned int> GetResult(ShaderStage stage);

	static bool IsTemporaryName(const std::string& name);

private:
	std::unique_ptr<spirv_cross::CompilerGLSL> m_compiler;

}; // ShaderRename

}