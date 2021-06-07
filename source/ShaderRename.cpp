#include "shadertrans/ShaderRename.h"
#include "shadertrans/ShaderTrans.h"

#include <spirv_glsl.hpp>

namespace
{

static bool is_numeric(char c)
{
	return c >= '0' && c <= '9';
}

}

namespace shadertrans
{

ShaderRename::ShaderRename(const std::vector<unsigned int>& spirv)
    : m_spirv(spirv)
{
    m_compiler = std::make_unique<spirv_cross::CompilerGLSL>(spirv);
}

ShaderRename::~ShaderRename()
{
}

void ShaderRename::FillingUBOInstName()
{
    spirv_cross::ShaderResources resources = m_compiler->get_shader_resources();

    int idx = 0;
    for (auto& resource : resources.uniform_buffers)
    {
        auto ubo_name = m_compiler->get_name(resource.id);
        auto base_name = m_compiler->get_name(resource.base_type_id);
        if (ubo_name.empty() && strncmp(base_name.c_str(), "UBO_", 4) == 0)
        {
            std::string name = "u_" + base_name.substr(4);
            m_compiler->set_name(resource.id, name);
            m_dirty = true;
        }
    }
}

void ShaderRename::RenameSampledImages()
{
    spirv_cross::ShaderResources resources = m_compiler->get_shader_resources();

    int idx = 0;
    for (auto& img : resources.sampled_images) 
    {
        auto name = m_compiler->get_name(img.id);
        if (IsTemporaryName(name)) {
            m_compiler->set_name(img.id, "texture" + std::to_string(idx));
            m_dirty = true;
        }
        ++idx;
    }
}

std::vector<unsigned int> ShaderRename::GetResult(ShaderStage stage)
{
    if (!m_dirty) {
        return m_spirv;
    }

    m_compiler->build_combined_image_samplers();
    auto glsl = m_compiler->compile();

    std::vector<unsigned int> ret;
    ShaderTrans::GLSL2SpirV(stage, glsl, ret);
    return ret;
}

// from spirv-cross
bool ShaderRename::IsTemporaryName(const std::string& name)
{
    if (name.size() < 2)
        return false;

    if (name[0] != '_' || !is_numeric(name[1]))
        return false;

    size_t index = 2;
    while (index < name.size() && is_numeric(name[index]))
        index++;

    return index == name.size() || (index < name.size() && name[index] == '_');
}

}