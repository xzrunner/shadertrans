#include "shadertrans/SpirvTools.h"

#include <spirv-tools/libspirv.hpp>

namespace shadertrans
{

bool SpirvTools::Assemble(const char* text, size_t text_size, std::vector<uint32_t>* binary)
{
	spvtools::SpirvTools tools(SPV_ENV_UNIVERSAL_1_0);
	return tools.Assemble(text, text_size, binary);
}

bool SpirvTools::Disassemble(const uint32_t* binary, size_t binary_size, std::string* text)
{
	spvtools::SpirvTools tools(SPV_ENV_UNIVERSAL_1_0);
	return tools.Disassemble(binary, binary_size, text);
}

}