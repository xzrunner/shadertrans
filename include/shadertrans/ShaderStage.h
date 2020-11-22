#pragma once

#include <stdint.h>

namespace shadertrans
{

enum class ShaderStage : uint32_t
{
    VertexShader,
    TessCtrlShader,
    TessEvalShader,
    GeometryShader,
    PixelShader,

    ComputeShader,

    NumShaderStages,
};

}