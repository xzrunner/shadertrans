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
	ShaderRename(const std::vector<unsigned int>& spirv);
	~ShaderRename();

	void FillingUBOInstName();
	void RenameSampledImages();

	std::vector<unsigned int> GetResult(ShaderStage stage);

	static bool IsTemporaryName(const std::string& name);

private:
	const std::vector<unsigned int>& m_spirv;
	std::unique_ptr<spirv_cross::CompilerGLSL> m_compiler;

	bool m_dirty = false;

}; // ShaderRename

}