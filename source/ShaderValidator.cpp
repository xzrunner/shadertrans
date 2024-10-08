#include "shadertrans/ShaderValidator.h"
#include "shadertrans/ConfigGLSL.h"
#include "shadertrans/CompilerDX.h"
#include "shadertrans/GLSLangAdapter.h"

#include <dxc/Support/WinIncludes.h>
#include <dxc/dxcapi.h>

#include <iostream>

#include <assert.h>

namespace
{

const char* entryPoint = "main";

}

namespace shadertrans
{

ShaderValidator::ShaderValidator(ShaderStage stage)
	: m_stage(stage)
{
}

bool ShaderValidator::Validate(const std::string& code, bool is_glsl, std::ostream& out) const
{
	if (is_glsl) 
	{
		if (!m_glsl) {
			m_glsl = std::make_unique<CompilerGLSL>(m_stage);
		}
		return m_glsl->Validate(code, out);
	} 
	else 
	{
		std::vector<const wchar_t*> dxcArgs;
		dxcArgs.push_back(L"-Zpr");
		dxcArgs.push_back(L"-O3");
		dxcArgs.push_back(L"-spirv");

		CComPtr<IDxcBlobEncoding> sourceBlob;
		IFT(shadertrans::CompilerDX::Instance().Library()->CreateBlobWithEncodingOnHeapCopy(code.c_str(), code.size(),
			CP_UTF8, &sourceBlob));
		IFTARG(sourceBlob->GetBufferSize() >= 4);

		std::wstring entryPointUtf16;
		Unicode::UTF8ToWideString(entryPoint, &entryPointUtf16);

		std::wstring shaderProfile = hlsl_shader_profile_name(m_stage, 6, 0);

		std::vector<DxcDefine> dxcDefines;
		//CComPtr<IDxcIncludeHandler> includeHandler = new hlsl::ScIncludeHandler(std::move(hlsl::DefaultLoadCallback));
		CComPtr<IDxcIncludeHandler> includeHandler = nullptr;
		CComPtr<IDxcOperationResult> compileResult;
		IFT(shadertrans::CompilerDX::Instance().Compiler()->Compile(sourceBlob, nullptr, entryPointUtf16.c_str(), shaderProfile.c_str(),
			dxcArgs.data(), static_cast<UINT32>(dxcArgs.size()), dxcDefines.data(),
			static_cast<UINT32>(dxcDefines.size()), includeHandler, &compileResult));

		CComPtr<IDxcBlobEncoding> errors;
		IFT(compileResult->GetErrorBuffer(&errors));
		if (errors != nullptr) {
			if (errors->GetBufferSize() > 0) {
				out << static_cast<const char*>(errors->GetBufferPointer()) << "\n";
				return false;
			}
			errors = nullptr;
		}
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////
// class ShaderValidator::CompilerGLSL
//////////////////////////////////////////////////////////////////////////

ShaderValidator::CompilerGLSL::CompilerGLSL(ShaderStage stage)
{
	ShInitialize();
	ShInitialize();  // also test reference counting of users
	ShFinalize();    // also test reference counting of users

	m_compiler = ShConstructCompiler(GLSLangAdapter::Type2GLSLang(stage), 0);
}

ShaderValidator::CompilerGLSL::~CompilerGLSL()
{
	ShDestruct(m_compiler);

	ShFinalize();
}

bool ShaderValidator::CompilerGLSL::Validate(const std::string& code, std::ostream& out) const
{
	const char* shader_str = code.c_str();
	int ret = ShCompile(m_compiler, &shader_str, 1, nullptr, EShOptNone,
		&glsl::DefaultTBuiltInResource, 0, 100, false, EShMsgDefault);

	out << ShGetInfoLog(m_compiler) << "\n";

	return ret != 0;
}

}