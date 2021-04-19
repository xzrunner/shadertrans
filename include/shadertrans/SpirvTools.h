#pragma once

#include <vector>
#include <string>

namespace shadertrans
{

class SpirvTools
{
public:
    static bool Assemble(const char* text, size_t text_size, std::vector<uint32_t>* binary);
    static bool Disassemble(const uint32_t* binary, size_t binary_size, std::string* text);

}; // SpirvTools

}