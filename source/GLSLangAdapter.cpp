#include "shadertrans/GLSLangAdapter.h"

#include <glslang/public/ShaderLang.h>

namespace shadertrans
{

GLSLangAdapter* GLSLangAdapter::m_instance = nullptr;

GLSLangAdapter* GLSLangAdapter::Instance()
{
	if (!m_instance) {
		m_instance = new GLSLangAdapter();
	}
	return m_instance;
}

GLSLangAdapter::GLSLangAdapter()
{
}

GLSLangAdapter::~GLSLangAdapter()
{
	if (m_inited) {
		glslang::FinalizeProcess();
	}
}

void GLSLangAdapter::Init()
{
	if (!m_inited)
	{
		glslang::InitializeProcess();
		m_inited = true;
	}
}

}