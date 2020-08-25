#pragma once

#include "shadertrans/ShaderStage.h"

#include <string>
#include <vector>

namespace shadertrans
{

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