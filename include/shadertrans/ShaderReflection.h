#pragma once

#include <string>
#include <vector>

namespace shadertrans
{

class ShaderReflection
{
public:
    enum class VarType
    {
        Unknown,
        Array,
        Struct,
        Bool,
        Int,
        Int2,
        Int3,
        Int4,
        Float,
        Float2,
        Float3,
        Float4,
        Mat2,
        Mat3,
        Mat4,
        Sampler,
        Image,
    };

    struct Variable
    {
        std::string name;
        VarType type;
        std::vector<Variable> children;
    };
    };

public:
    static void GetUniforms(const std::vector<unsigned int>& spirv, 
        std::vector<Variable>& uniforms);


}; // ShaderReflection

}