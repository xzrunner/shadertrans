#pragma once

#include "shadertrans/ShaderStage.h"

#include <glslang/public/ShaderLang.h>

namespace shadertrans
{

class GLSLangAdapter
{
public:
	void Init();

	static EShLanguage Type2GLSLang(ShaderStage stage);

	static GLSLangAdapter* Instance();

private:
	GLSLangAdapter();
	~GLSLangAdapter();

private:
	bool m_inited = false;

	static GLSLangAdapter* m_instance;

}; // GLSLangAdapter

}