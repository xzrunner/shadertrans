#include "shadertrans/CompilerDX.h"

namespace
{

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
    case shadertrans::ShaderStage::TessCtrlShader:
        shaderProfile = L"tcs";
        break;
    case shadertrans::ShaderStage::TessEvalShader:
        shaderProfile = L"tes";
        break;
    case shadertrans::ShaderStage::GeometryShader:
        shaderProfile = L"gs";
        break;
    case shadertrans::ShaderStage::PixelShader:
        shaderProfile = L"ps";
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

}

namespace shadertrans
{

CompilerDX::CompilerDX()
{
    if (m_dllDetaching)
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

CompilerDX::~CompilerDX()
{
    this->Destroy();
}

CompilerDX& CompilerDX::Instance()
{
    static CompilerDX instance;
    return instance;
}

CComPtr<IDxcLinker> CompilerDX::CreateLinker() const
{
    CComPtr<IDxcLinker> linker;
    IFT(m_createInstanceFunc(CLSID_DxcLinker, __uuidof(IDxcLinker), reinterpret_cast<void**>(&linker)));
    return linker;
}

void CompilerDX::Destroy()
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

void CompilerDX::Terminate()
{
    if (m_dxcompilerDll)
    {
        m_compiler.Detach();
        m_library.Detach();

        m_createInstanceFunc = nullptr;

        m_dxcompilerDll = nullptr;
    }
}

std::wstring hlsl_shader_profile_name(shadertrans::ShaderStage stage, uint8_t major_ver, uint8_t minor_ver)
{
    return ShaderProfileName(stage, { major_ver, minor_ver });
}

}