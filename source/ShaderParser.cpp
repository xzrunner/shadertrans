#include "shadertrans/ShaderParser.h"
#include "shadertrans/ConfigGLSL.h"
#include "shadertrans/GLSLangAdapter.h"

#include <glslang/public/ShaderLang.h>

#include <iostream>

namespace shadertrans
{

glslang::TShader* ShaderParser::ParseHLSL(const std::string& glsl)
{
    GLSLangAdapter::Instance()->Init();

    std::vector<unsigned int> spirv;

    const EShLanguage shader_type = EShLanguage::EShLangFragment;
    glslang::TShader* shader = new glslang::TShader(shader_type);
    const char* src_cstr = glsl.c_str();
    shader->setStrings(&src_cstr, 1);

    //shader->setEnvTargetHlslFunctionality1();
    //shader->setHlslIoMapping(true);
    
    int client_input_semantics_version = 100; // maps to, say, #define VULKAN 100
    glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;
    glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;

//    glslang::EProfile

    shader->setEnvInput(glslang::EShSourceHlsl, shader_type, glslang::EShClientVulkan, client_input_semantics_version);
    shader->setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
    shader->setEnvTarget(glslang::EShTargetSpv, TargetVersion);

    shader->setAutoMapLocations(true);
    shader->setAutoMapBindings(true);

    TBuiltInResource resources;
    resources = glsl::DefaultTBuiltInResource;
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    glslang::TShader::ForbidIncluder forbid_includer;

    if (!shader->parse(&resources, 100, true, messages))
    {
        std::cout << "GLSL Parsing Failed for: " << glsl << std::endl;
        std::cout << shader->getInfoLog() << std::endl;
        std::cout << shader->getInfoDebugLog() << std::endl;
        return nullptr;
    }

    //glslang::TProgram program;
    //program.addShader(shader);
    //if (!program.link(messages))
    //{
    //    std::cout << "GLSL Linking Failed for: " << glsl << std::endl;
    //    std::cout << shader->getInfoLog() << std::endl;
    //    std::cout << shader->getInfoDebugLog() << std::endl;
    //    return nullptr;
    //}

    return shader;
}

}