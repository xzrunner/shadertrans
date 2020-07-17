#pragma once

#include <string>
#include <vector>

namespace shadertrans
{

enum class ShaderStage : uint32_t
{
    VertexShader,
    PixelShader,
    GeometryShader,
    HullShader,
    DomainShader,
    ComputeShader,

    NumShaderStages,
};

class ShaderTrans
{
public:
	static void HLSL2SpirV(ShaderStage stage, const std::string& hlsl,
		std::vector<unsigned int>& spirv);
	static void GLSL2SpirV(ShaderStage stage, const std::string& glsl,
		std::vector<unsigned int>& spirv);
	static void SpirV2GLSL(ShaderStage stage, const std::vector<unsigned int>& spirv,
		std::string& glsl);

}; // ShaderTrans

}