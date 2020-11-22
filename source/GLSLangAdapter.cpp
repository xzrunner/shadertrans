#include "shadertrans/GLSLangAdapter.h"

#include <assert.h>

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

EShLanguage GLSLangAdapter::Type2GLSLang(shadertrans::ShaderStage stage)
{
    switch (stage)
    {
    case shadertrans::ShaderStage::VertexShader:
        return EShLangVertex;
    case shadertrans::ShaderStage::TessCtrlShader:
        return EShLangTessControl;
    case shadertrans::ShaderStage::TessEvalShader:
        return EShLangTessEvaluation;
    case shadertrans::ShaderStage::GeometryShader:
        return EShLangGeometry;
    case shadertrans::ShaderStage::PixelShader:
        return EShLangFragment;
    case shadertrans::ShaderStage::ComputeShader:
        return EShLangCompute;
    default:
        assert(0);
        return EShLangCount;
    }
}

}