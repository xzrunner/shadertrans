#pragma once

#include "shadertrans/ShaderStage.h"

#include <dxc/Support/Global.h>
#include <dxc/Support/Unicode.h>
#include <dxc/Support/WinAdapter.h>
#include <dxc/Support/WinIncludes.h>
#include <dxc/dxcapi.h>

namespace shadertrans
{

// code from https://github.com/microsoft/ShaderConductor
class CompilerDX
{
public:
    CompilerDX();
    ~CompilerDX();

    static CompilerDX& Instance();

    IDxcLibrary* Library() const { return m_library; }

    IDxcCompiler* Compiler() const { return m_compiler; }

    CComPtr<IDxcLinker> CreateLinker() const;

    bool LinkerSupport() const { return m_linkerSupport; }

    void Destroy();
    void Terminate();

private:
    HMODULE m_dxcompilerDll = nullptr;
    DxcCreateInstanceProc m_createInstanceFunc = nullptr;

    CComPtr<IDxcLibrary> m_library;
    CComPtr<IDxcCompiler> m_compiler;

    bool m_linkerSupport;

    bool m_dllDetaching = false;

}; // CompilerDX

std::wstring hlsl_shader_profile_name(ShaderStage stage, uint8_t major_ver, uint8_t minor_ver);

}