#include "shadertrans/ShaderTrans.h"
#include "shadertrans/ConfigGLSL.h"
#include "shadertrans/CompilerDX.h"
#include "shadertrans/GLSLangAdapter.h"

#include <glslang/public/ShaderLang.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv.hpp>
#include <spirv_glsl.hpp>
#include <dxc/Support/WinIncludes.h>
#include <dxc/dxcapi.h>

#include <iostream>
#include <atomic>

namespace hlsl
{

class Blob
{
public:
    Blob(const void* data, uint32_t size)
        : data_(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size)
    {
    }

    const void* Data() const
    {
        return data_.data();
    }

    uint32_t Size() const
    {
        return static_cast<uint32_t>(data_.size());
    }

private:
    std::vector<uint8_t> data_;
};

Blob* CreateBlob(const void* data, uint32_t size)
{
    return new Blob(data, size);
}

void DestroyBlob(Blob* blob)
{
    delete blob;
}

class ScIncludeHandler : public IDxcIncludeHandler
{
public:
    explicit ScIncludeHandler(std::function<Blob*(const char* includeName)> loadCallback) : m_loadCallback(std::move(loadCallback))
    {
    }

    HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR fileName, IDxcBlob** includeSource) override
    {
        if ((fileName[0] == L'.') && (fileName[1] == L'/'))
        {
            fileName += 2;
        }

        std::string utf8FileName;
        if (!Unicode::WideToUTF8String(fileName, &utf8FileName))
        {
            return E_FAIL;
        }

        auto blobDeleter = [](Blob* blob) { DestroyBlob(blob); };

        std::unique_ptr<Blob, decltype(blobDeleter)> source(nullptr, blobDeleter);
        try
        {
            source.reset(m_loadCallback(utf8FileName.c_str()));
        }
        catch (...)
        {
            return E_FAIL;
        }

        *includeSource = nullptr;
        return shadertrans::CompilerDX::Instance().Library()->CreateBlobWithEncodingOnHeapCopy(
            source->Data(), source->Size(), CP_UTF8, reinterpret_cast<IDxcBlobEncoding**>(includeSource)
        );
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        ++m_ref;
        return m_ref;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        --m_ref;
        ULONG result = m_ref;
        if (result == 0)
        {
            delete this;
        }
        return result;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override
    {
        if (IsEqualIID(iid, __uuidof(IDxcIncludeHandler)))
        {
            *object = dynamic_cast<IDxcIncludeHandler*>(this);
            this->AddRef();
            return S_OK;
        }
        else if (IsEqualIID(iid, __uuidof(IUnknown)))
        {
            *object = dynamic_cast<IUnknown*>(this);
            this->AddRef();
            return S_OK;
        }
        else
        {
            return E_NOINTERFACE;
        }
    }

private:
    std::function<Blob*(const char* includeName)> m_loadCallback;

    std::atomic<ULONG> m_ref = 0;
};

Blob* DefaultLoadCallback(const char* includeName)
{
    std::vector<char> ret;
    std::ifstream includeFile(includeName, std::ios_base::in);
    if (includeFile)
    {
        includeFile.seekg(0, std::ios::end);
        ret.resize(static_cast<size_t>(includeFile.tellg()));
        includeFile.seekg(0, std::ios::beg);
        includeFile.read(ret.data(), ret.size());
        ret.resize(static_cast<size_t>(includeFile.gcount()));
    }
    else
    {
        throw std::runtime_error(std::string("COULDN'T load included file ") + includeName + ".");
    }
    return CreateBlob(ret.data(), static_cast<uint32_t>(ret.size()));
}

}

namespace shadertrans
{

void ShaderTrans::HLSL2SpirV(ShaderStage stage, const std::string& hlsl, const std::string& entry_point,
                             std::vector<unsigned int>& spirv, std::ostream& out)
{
    std::vector<const wchar_t*> dxcArgs;
    dxcArgs.push_back(L"-Zpr");
    dxcArgs.push_back(L"-O0");
    dxcArgs.push_back(L"-spirv");

    CComPtr<IDxcBlobEncoding> sourceBlob;
    IFT(shadertrans::CompilerDX::Instance().Library()->CreateBlobWithEncodingOnHeapCopy(hlsl.c_str(), hlsl.size(),
        CP_UTF8, &sourceBlob));
    IFTARG(sourceBlob->GetBufferSize() >= 4);

    std::wstring entryPointUtf16;
    Unicode::UTF8ToWideString(entry_point.c_str(), &entryPointUtf16);

    std::wstring shaderProfile = hlsl_shader_profile_name(stage, 6, 0);;

    std::vector<DxcDefine> dxcDefines;

    CComPtr<IDxcIncludeHandler> includeHandler = new hlsl::ScIncludeHandler(std::move(hlsl::DefaultLoadCallback));
    CComPtr<IDxcOperationResult> compileResult;
    IFT(shadertrans::CompilerDX::Instance().Compiler()->Compile(sourceBlob, nullptr, entryPointUtf16.c_str(), shaderProfile.c_str(),
        dxcArgs.data(), static_cast<UINT32>(dxcArgs.size()), dxcDefines.data(),
        static_cast<UINT32>(dxcDefines.size()), includeHandler, &compileResult));

    CComPtr<IDxcBlobEncoding> errors;
    IFT(compileResult->GetErrorBuffer(&errors));
    if (errors != nullptr)
    {
        if (errors->GetBufferSize() > 0)
        {
            spirv.clear();
            out << static_cast<const char*>(errors->GetBufferPointer()) << "\n";
            return;
        }
        errors = nullptr;
    }

    HRESULT status;
    IFT(compileResult->GetStatus(&status));
    if (SUCCEEDED(status))
    {
        CComPtr<IDxcBlob> program;
        IFT(compileResult->GetResult(&program));
        compileResult = nullptr;
        if (program != nullptr)
        {
            auto data = reinterpret_cast<const unsigned int*>(program->GetBufferPointer());
            auto size = static_cast<uint32_t>(program->GetBufferSize()) >> 2;
            spirv.assign(data, reinterpret_cast<const unsigned int*>(data) + size);
        }
    }
}

void ShaderTrans::GLSL2SpirV(ShaderStage stage, const std::string& glsl, const char* inc_dir,
	                         std::vector<unsigned int>& spirv, bool no_link, std::ostream& out)
{
    if (glsl.empty()) {
        return;
    }

    GLSLangAdapter::Instance()->Init();

    spirv.clear();

    const EShLanguage shader_type = GLSLangAdapter::Type2GLSLang(stage);
    glslang::TShader shader(shader_type);
    const char* src_cstr = glsl.c_str();
    shader.setStrings(&src_cstr, 1);

    int client_input_semantics_version = 100; // maps to, say, #define VULKAN 100
    glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;
    glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;

    shader.setEnvInput(glslang::EShSourceGlsl, shader_type, glslang::EShClientVulkan, client_input_semantics_version);
    shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
    shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

    shader.setAutoMapLocations(true);
    shader.setAutoMapBindings(true);

    TBuiltInResource resources;
    resources = glsl::DefaultTBuiltInResource;
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    const int default_version = 100;

    DirStackFileIncluder includer;
    if (inc_dir) {
        includer.pushExternalLocalDirectory(inc_dir);
    }

    std::string preprocessed_glsl;
    if (!shader.preprocess(&resources, default_version, ENoProfile, false, false, messages, &preprocessed_glsl, includer))
    {
        out << "GLSL Preprocessing Failed for: " << glsl << "\n";
        out << shader.getInfoLog() << "\n" << shader.getInfoDebugLog() << "\n";
        return;
    }

    const char* preprocessed_cstr = preprocessed_glsl.c_str();
    shader.setStrings(&preprocessed_cstr, 1);

    if (!shader.parse(&resources, 100, false, messages))
    {
        out << "GLSL Parsing Failed for: " << glsl << "\n";
        out << shader.getInfoLog() << "\n" << shader.getInfoDebugLog() << "\n";
        return;
    }

    if (no_link)
    {
        spv::SpvBuildLogger logger;
        glslang::SpvOptions spv_options;
        glslang::GlslangToSpv(*shader.getIntermediate(), spirv, &logger, &spv_options);
    }
    else
    {
        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(messages))
        {
            out << "GLSL Linking Failed for: " << glsl << "\n";
            out << shader.getInfoLog() << "\n" << shader.getInfoDebugLog() << "\n";
            return;
        }

        spv::SpvBuildLogger logger;
        glslang::SpvOptions spv_options;
        glslang::GlslangToSpv(*program.getIntermediate(shader_type), spirv, &logger, &spv_options);
    }
}

void ShaderTrans::SpirV2GLSL(ShaderStage stage, const std::vector<unsigned int>& spirv,
                             std::string& glsl, bool use_ubo, std::ostream& out)
{
    try {
        spirv_cross::CompilerGLSL compiler(spirv);

        auto op = compiler.get_common_options();
        if (!use_ubo) {
            op.emit_push_constant_as_uniform_buffer  = true;
            op.emit_uniform_buffer_as_plain_uniforms = true;
        }
        // fix for mac
        op.version = 410;
        compiler.set_common_options(op);

        try {
            compiler.build_combined_image_samplers();

            // fix for mac
            if (stage == ShaderStage::VertexShader)
            {
                auto resources = compiler.get_shader_resources();
                for (auto& resource : resources.stage_outputs)
                {
                    std::string name = compiler.get_name(resource.id);
                    if (name.find("out.var.") == 0)
                        name = name.substr(4);
                    compiler.set_name(resource.id, name);
                }
            }
            else if (stage == ShaderStage::PixelShader)
            {
                auto resources = compiler.get_shader_resources();
                for (auto& resource : resources.stage_inputs)
                {
                    std::string name = compiler.get_name(resource.id);
                    if (name.find("in.var.") == 0)
                        name = name.substr(3);
                    compiler.set_name(resource.id, name);
                }
            }

            glsl = compiler.compile();
        } catch (const std::exception& e) {
            out << e.what() << "\n";
            return;
        }

        //printf("%s\n", glsl.c_str());
    } catch (std::exception& e) {
        out << "spir-v to glsl fail: " << e.what() << "\n";
    }
}

}