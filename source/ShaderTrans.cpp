#include "shadertrans/ShaderTrans.h"

#include <glslang/public/ShaderLang.h>
#include <glslang/include/StandAlone/DirStackFileIncluder.h>
#include <glslang/include/SPIRV/GlslangToSpv.h>
#include <spirv-cross/spirv.hpp>
#include <spirv-cross/spirv_glsl.hpp>
#include <dxc/Support/Global.h>
#include <dxc/Support/Unicode.h>
#include <dxc/Support/WinAdapter.h>
#include <dxc/Support/WinIncludes.h>
#include <dxc/dxcapi.h>

#include <iostream>
#include <atomic>

namespace glsl
{

const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    ///* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }
};

EShLanguage type2glslang(shadertrans::ShaderStage stage)
{
    switch (stage)
    {
    case shadertrans::ShaderStage::VertexShader:
        return EShLangVertex;
    case shadertrans::ShaderStage::PixelShader:
        return EShLangFragment;
    case shadertrans::ShaderStage::GeometryShader:
        return EShLangGeometry;
    case shadertrans::ShaderStage::ComputeShader:
        return EShLangCompute;
    default:
        assert(0);
        return EShLangCount;
    }
}

bool glslang_inited = false;


}

// code from https://github.com/microsoft/ShaderConductor
namespace hlsl
{

bool dllDetaching = false;

class Dxcompiler
{
public:
    ~Dxcompiler()
    {
        this->Destroy();
    }

    static Dxcompiler& Instance()
    {
        static Dxcompiler instance;
        return instance;
    }

    IDxcLibrary* Library() const
    {
        return m_library;
    }

    IDxcCompiler* Compiler() const
    {
        return m_compiler;
    }

    CComPtr<IDxcLinker> CreateLinker() const
    {
        CComPtr<IDxcLinker> linker;
        IFT(m_createInstanceFunc(CLSID_DxcLinker, __uuidof(IDxcLinker), reinterpret_cast<void**>(&linker)));
        return linker;
    }

    bool LinkerSupport() const
    {
        return m_linkerSupport;
    }

    void Destroy()
    {
        if (m_dxcompilerDll)
        {
            m_compiler = nullptr;
            m_library = nullptr;

            m_createInstanceFunc = nullptr;

#ifdef _WIN32
            ::FreeLibrary(m_dxcompilerDll);
#else
            ::dlclose(m_dxcompilerDll);
#endif

            m_dxcompilerDll = nullptr;
        }
    }

    void Terminate()
    {
        if (m_dxcompilerDll)
        {
            m_compiler.Detach();
            m_library.Detach();

            m_createInstanceFunc = nullptr;

            m_dxcompilerDll = nullptr;
        }
    }

private:
    Dxcompiler()
    {
        if (dllDetaching)
        {
            return;
        }

#ifdef _WIN32
        const char* dllName = "dxcompiler.dll";
#elif __APPLE__
        const char* dllName = "libdxcompiler.dylib";
#else
        const char* dllName = "libdxcompiler.so";
#endif
        const char* functionName = "DxcCreateInstance";

#ifdef _WIN32
        m_dxcompilerDll = ::LoadLibraryA(dllName);
#else
        m_dxcompilerDll = ::dlopen(dllName, RTLD_LAZY);
#endif

        if (m_dxcompilerDll != nullptr)
        {
#ifdef _WIN32
            m_createInstanceFunc = (DxcCreateInstanceProc)::GetProcAddress(m_dxcompilerDll, functionName);
#else
            m_createInstanceFunc = (DxcCreateInstanceProc)::dlsym(m_dxcompilerDll, functionName);
#endif

            if (m_createInstanceFunc != nullptr)
            {
                IFT(m_createInstanceFunc(CLSID_DxcLibrary, __uuidof(IDxcLibrary), reinterpret_cast<void**>(&m_library)));
                IFT(m_createInstanceFunc(CLSID_DxcCompiler, __uuidof(IDxcCompiler), reinterpret_cast<void**>(&m_compiler)));
            }
            else
            {
                this->Destroy();

                throw std::runtime_error(std::string("COULDN'T get ") + functionName + " from dxcompiler.");
            }
        }
        else
        {
            throw std::runtime_error("COULDN'T load dxcompiler.");
        }

        m_linkerSupport = (CreateLinker() != nullptr);
    }

private:
    HMODULE m_dxcompilerDll = nullptr;
    DxcCreateInstanceProc m_createInstanceFunc = nullptr;

    CComPtr<IDxcLibrary> m_library;
    CComPtr<IDxcCompiler> m_compiler;

    bool m_linkerSupport;
}; // Dxcompiler


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
        if (!Unicode::UTF16ToUTF8String(fileName, &utf8FileName))
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
        return Dxcompiler::Instance().Library()->CreateBlobWithEncodingOnHeapCopy(source->Data(), source->Size(), CP_UTF8,
                                                                                    reinterpret_cast<IDxcBlobEncoding**>(includeSource));
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

struct ShaderModel
{
    uint8_t major_ver : 6;
    uint8_t minor_ver : 2;

    uint32_t FullVersion() const noexcept
    {
        return (major_ver << 2) | minor_ver;
    }

    bool operator<(const ShaderModel& other) const noexcept
    {
        return this->FullVersion() < other.FullVersion();
    }
    bool operator==(const ShaderModel& other) const noexcept
    {
        return this->FullVersion() == other.FullVersion();
    }
    bool operator>(const ShaderModel& other) const noexcept
    {
        return other < *this;
    }
    bool operator<=(const ShaderModel& other) const noexcept
    {
        return (*this < other) || (*this == other);
    }
    bool operator>=(const ShaderModel& other) const noexcept
    {
        return (*this > other) || (*this == other);
    }
};

std::wstring ShaderProfileName(shadertrans::ShaderStage stage, ShaderModel shaderModel)
{
    std::wstring shaderProfile;
    switch (stage)
    {
    case shadertrans::ShaderStage::VertexShader:
        shaderProfile = L"vs";
        break;

    case shadertrans::ShaderStage::PixelShader:
        shaderProfile = L"ps";
        break;

    case shadertrans::ShaderStage::GeometryShader:
        shaderProfile = L"gs";
        break;

    case shadertrans::ShaderStage::ComputeShader:
        shaderProfile = L"cs";
        break;

    default:
        throw std::runtime_error("Invalid shader stage.");
    }

    shaderProfile.push_back(L'_');
    shaderProfile.push_back(L'0' + shaderModel.major_ver);
    shaderProfile.push_back(L'_');
    shaderProfile.push_back(L'0' + shaderModel.minor_ver);

    return shaderProfile;
}

const char* entryPoint = "main";

}

namespace shadertrans
{

void ShaderTrans::HLSL2SpirV(ShaderStage stage, const std::string& hlsl, 
                             std::vector<unsigned int>& spirv)
{
    std::vector<const wchar_t*> dxcArgs;
    dxcArgs.push_back(L"-Zpr");
    dxcArgs.push_back(L"-O3");
    dxcArgs.push_back(L"-spirv");

    CComPtr<IDxcBlobEncoding> sourceBlob;
    IFT(hlsl::Dxcompiler::Instance().Library()->CreateBlobWithEncodingOnHeapCopy(hlsl.c_str(), hlsl.size(),
        CP_UTF8, &sourceBlob));
    IFTARG(sourceBlob->GetBufferSize() >= 4);

    std::wstring entryPointUtf16;
    Unicode::UTF8ToUTF16String(hlsl::entryPoint, &entryPointUtf16);

    std::wstring shaderProfile = hlsl::ShaderProfileName(stage, { 6, 0 });

    std::vector<DxcDefine> dxcDefines;
    CComPtr<IDxcIncludeHandler> includeHandler = new hlsl::ScIncludeHandler(std::move(hlsl::DefaultLoadCallback));
    CComPtr<IDxcOperationResult> compileResult;
    IFT(hlsl::Dxcompiler::Instance().Compiler()->Compile(sourceBlob, nullptr, entryPointUtf16.c_str(), shaderProfile.c_str(),
        dxcArgs.data(), static_cast<UINT32>(dxcArgs.size()), dxcDefines.data(),
        static_cast<UINT32>(dxcDefines.size()), includeHandler, &compileResult));

    CComPtr<IDxcBlobEncoding> errors;
    IFT(compileResult->GetErrorBuffer(&errors));
    if (errors != nullptr)
    {
        if (errors->GetBufferSize() > 0)
        {
            printf("%s\n", static_cast<const char*>(errors->GetBufferPointer()));
            spirv.clear();
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

void ShaderTrans::GLSL2SpirV(ShaderStage stage, const std::string& glsl, 
	                         std::vector<unsigned int>& spirv)
{
    if (!glsl::glslang_inited)
    {
        glslang::InitializeProcess();
        glsl::glslang_inited = true;
    }

    const EShLanguage shader_type = glsl::type2glslang(stage);
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

    ////Get Path of File
    //std::string Path = GetFilePath(filename);
    //includer.pushExternalLocalDirectory(Path);

    std::string preprocessed_glsl;
    if (!shader.preprocess(&resources, default_version, ENoProfile, false, false, messages, &preprocessed_glsl, includer)) 
    {
        std::cout << "GLSL Preprocessing Failed for: " << glsl << std::endl;
        std::cout << shader.getInfoLog() << std::endl;
        std::cout << shader.getInfoDebugLog() << std::endl;
        return;
    }

    const char* preprocessed_cstr = preprocessed_glsl.c_str();
    shader.setStrings(&preprocessed_cstr, 1);

    if (!shader.parse(&resources, 100, false, messages))
    {
        std::cout << "GLSL Parsing Failed for: " << glsl << std::endl;
        std::cout << shader.getInfoLog() << std::endl;
        std::cout << shader.getInfoDebugLog() << std::endl;
        return;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages))
    {
        std::cout << "GLSL Linking Failed for: " << glsl << std::endl;
        std::cout << shader.getInfoLog() << std::endl;
        std::cout << shader.getInfoDebugLog() << std::endl;
        return;
    }

    spv::SpvBuildLogger logger;
    glslang::SpvOptions spv_options;
    glslang::GlslangToSpv(*program.getIntermediate(shader_type), spirv, &logger, &spv_options);
}

void ShaderTrans::SpirV2GLSL(ShaderStage stage, const std::vector<unsigned int>& spirv, std::string& glsl)
{
    spirv_cross::CompilerGLSL compiler(spirv);

    auto op = compiler.get_common_options();
    op.emit_uniform_buffer_as_plain_uniforms = true;
    compiler.set_common_options(op);

    try {
        compiler.build_combined_image_samplers();
        glsl = compiler.compile();
    } catch (const std::exception& e) {
        printf("%s\n", e.what());
        return;
    }

    printf("%s\n", glsl.c_str());
}

}