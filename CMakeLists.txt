cmake_minimum_required(VERSION 3.16)

project(shadertrans)

add_definitions(
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)

################################################################################
# Source groups
################################################################################
set(builder
    "include/shadertrans/ShaderBuilder.h"
    "include/shadertrans/SpirvGenTwo.h"
    "source/ShaderBuilder.cpp"
    "source/SpirvGenTwo.cpp"
)
source_group("builder" FILES ${builder})

set(dataset
    "include/shadertrans/ShaderStage.h"
)
source_group("dataset" FILES ${dataset})

set(external__SPIRV-Reflect
    "external/SPIRV-Reflect/spirv_reflect.c"
    "external/SPIRV-Reflect/spirv_reflect.h"
)
source_group("external\\SPIRV-Reflect" FILES ${external__SPIRV-Reflect})

set(external__SPIRV-Tools
    "external/SPIRV-Tools/include/spirv-tools/linker.hpp"
    "external/SPIRV-Tools/source/link/linker.cpp"
)
source_group("external\\SPIRV-Tools" FILES ${external__SPIRV-Tools})

set(parser
    "include/shadertrans/ShaderInfo.h"
    "include/shadertrans/ShaderParser.h"
    "include/shadertrans/ShaderReflection.h"
    "source/ShaderInfo.cpp"
    "source/ShaderParser.cpp"
    "source/ShaderReflection.cpp"
)
source_group("parser" FILES ${parser})

set(spirv
    "include/shadertrans/spirv_IR.h"
    "include/shadertrans/spirv_Linker.h"
    "include/shadertrans/spirv_Parser.h"
    "source/spirv_IR.cpp"
    "source/spirv_Linker.cpp"
    "source/spirv_Parser.cpp"
)
source_group("spirv" FILES ${spirv})

set(tools
    "include/shadertrans/CompilerDX.h"
    "include/shadertrans/ConfigGLSL.h"
    "include/shadertrans/GLSLangAdapter.h"
    "include/shadertrans/SpirvTools.h"
    "source/CompilerDX.cpp"
    "source/GLSLangAdapter.cpp"
    "source/SpirvTools.cpp"
)
source_group("tools" FILES ${tools})

set(tools__check
    "include/shadertrans/ShaderValidator.h"
    "source/ShaderValidator.cpp"
)
source_group("tools\\check" FILES ${tools__check})

set(tools__format
    "include/shadertrans/ShaderPreprocess.h"
    "source/ShaderPreprocess.cpp"
)
source_group("tools\\format" FILES ${tools__format})

set(tools__rename
    "include/shadertrans/ShaderRename.h"
    "source/ShaderRename.cpp"
)
source_group("tools\\rename" FILES ${tools__rename})

set(tools__trans
    "include/shadertrans/ShaderTrans.h"
    "source/ShaderTrans.cpp"
)
source_group("tools\\trans" FILES ${tools__trans})

set(ALL_FILES
    ${builder}
    ${dataset}
    ${external__SPIRV-Reflect}
    ${external__SPIRV-Tools}
    ${parser}
    ${spirv}
    ${tools}
    ${tools__check}
    ${tools__format}
    ${tools__rename}
    ${tools__trans}
)

add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE include external/SpvGenTwo/lib/include external/SpvGenTwo/common/include external/SPIRV-Tools/include external/SPIRV-Tools external/SPIRV-Headers/include external/SPIRV-Headers/include/spirv/unified1 external/glslang external/SPIRV-Cross external/SPIRV-Reflect external/DirectXShaderCompiler/include)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)