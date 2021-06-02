#include "shadertrans/ShaderRename.h"
#include "shadertrans/ShaderTrans.h"

#include <spirv_glsl.hpp>

namespace shadertrans
{

bool ShaderRename::FillingUBOInstName(ShaderStage stage, std::vector<unsigned int>& spirv)
{
    spirv_cross::CompilerGLSL compiler(spirv);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    bool dirty = false;

    int idx = 0;
    for (auto& resource : resources.uniform_buffers)
    {
        auto ubo_name = compiler.get_name(resource.id);
        auto base_name = compiler.get_name(resource.base_type_id);
        if (ubo_name.empty() && strncmp(base_name.c_str(), "UBO_", 4) == 0)
        {
            std::string name = "u_" + base_name.substr(4);
            compiler.set_name(resource.id, name);
            dirty = true;
        }
    }

    if (!dirty) {
        return false;
    }

    auto glsl = compiler.compile();

    std::vector<unsigned int> ret;
    ShaderTrans::GLSL2SpirV(stage, glsl, ret);
    if (ret.empty()) {
        return false;
    } else {
        spirv = ret;
        return true;
    }
}

}