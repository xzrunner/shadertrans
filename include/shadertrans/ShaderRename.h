#pragma once

#include "shadertrans/ShaderStage.h"

#include <vector>

namespace shadertrans
{

class ShaderRename
{
public:
	static bool FillingUBOInstName(ShaderStage stage, std::vector<unsigned int>& spirv);

}; // ShaderRename

}