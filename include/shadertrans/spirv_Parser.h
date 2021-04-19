#pragma once

#include "shadertrans/spirv_IR.h"

namespace shadertrans
{
namespace spirv
{

class Parser 
{
public:
	static void Parse(const std::vector<unsigned int>& spv, Module& module);

}; // Parser

}
}