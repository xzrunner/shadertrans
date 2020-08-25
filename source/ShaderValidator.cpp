#include "shadertrans/ShaderValidator.h"
#include "shadertrans/ConfigGLSL.h"
#include "shadertrans/CompilerDX.h"

#include <dxc/Support/WinIncludes.h>
#include <dxc/dxcapi.h>

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

bool ShaderValidator::Validate(const std::string& code, bool is_glsl, std::string& msg) const
{
	if (is_glsl) 
	{
		if (!m_glsl) {
			m_glsl = std::make_unique<CompilerGLSL>(m_stage);
		}
		return m_glsl->Validate(code, msg);
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
		Unicode::UTF8ToUTF16String(entryPoint, &entryPointUtf16);

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
				msg = static_cast<const char*>(errors->GetBufferPointer());
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

	EShLanguage gl_stage;
	switch (stage)
	{
	case ShaderStage::VertexShader:
		gl_stage = EShLangVertex;
		break;
	case ShaderStage::PixelShader:
		gl_stage = EShLangFragment;
		break;
	case ShaderStage::GeometryShader:
		gl_stage = EShLangGeometry;
		break;
	case ShaderStage::ComputeShader:
		gl_stage = EShLangCompute;
		break;
	default:
		assert(0);
	}

	m_compiler = ShConstructCompiler(gl_stage, 0);
}

ShaderValidator::CompilerGLSL::~CompilerGLSL()
{
	ShDestruct(m_compiler);

	ShFinalize();
}

bool ShaderValidator::CompilerGLSL::Validate(const std::string& code, std::string& msg) const
{
	const char* shader_str = code.c_str();
	int ret = ShCompile(m_compiler, &shader_str, 1, nullptr, EShOptNone,
		&glsl::DefaultTBuiltInResource, 0, 100, false, EShMsgDefault);

	msg = ShGetInfoLog(m_compiler);

	return ret != 0;
}

}