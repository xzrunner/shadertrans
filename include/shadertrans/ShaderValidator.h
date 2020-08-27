#pragma once

#include "shadertrans/ShaderStage.h"

#include <glslang/Public/ShaderLang.h>

#include <string>
#include <map>

namespace shadertrans
{

class ShaderValidator
{
public:
	ShaderValidator(ShaderStage stage);

	bool Validate(const std::string& code, bool is_glsl, std::ostream& out) const;

private:
	class CompilerGLSL
	{
	public:
		CompilerGLSL(ShaderStage lang);
		~CompilerGLSL();

		bool Validate(const std::string& code, std::ostream& out) const;

	private:
		ShHandle m_compiler = nullptr;

	}; // CompilerGLSL

private:
	ShaderStage m_stage;

	mutable std::unique_ptr<CompilerGLSL> m_glsl;

}; // ShaderValidator

}