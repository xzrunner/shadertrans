#pragma once

#include <stdint.h>

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

}