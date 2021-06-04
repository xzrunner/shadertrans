#pragma once

#include "shadertrans/ShaderStage.h"

#include <string>
#include <vector>
#include <iostream>

namespace shadertrans
{

class ShaderTrans
{
public:
	static void HLSL2SpirV(ShaderStage stage, const std::string& hlsl, const std::string& entry_point,
		std::vector<unsigned int>& spirv, std::ostream& out = std::cerr);
	static void GLSL2SpirV(ShaderStage stage, const std::string& glsl,
		std::vector<unsigned int>& spirv, bool no_link = false, std::ostream& out = std::cerr);
	static void SpirV2GLSL(ShaderStage stage, const std::vector<unsigned int>& spirv,
		std::string& glsl, std::ostream& out = std::cerr);

}; // ShaderTrans

}