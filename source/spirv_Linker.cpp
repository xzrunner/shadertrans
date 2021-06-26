#include "shadertrans/spirv_Linker.h"
#include "shadertrans/spirv_Parser.h"
#include "shadertrans/ShaderTrans.h"

#include <spirv_cross.hpp>
#include <spirv-tools/linker.hpp>

namespace shadertrans
{
namespace spirv
{

void Linker::AddModule(ShaderStage stage, const std::string& glsl)
{
    std::vector<unsigned int> spv;
    ShaderTrans::GLSL2SpirV(stage, glsl, nullptr, spv, true);

    //std::string str;
    //ShaderTrans::SpirV2GLSL(stage, spv, str);
    //printf("new:\n%s\n", str.c_str());

	auto module = std::make_shared<spirv::Module>();
	Parser::Parse(spv, *module);
	m_modules.push_back(module);
}

void Linker::Link()
{
    const spvtools::MessageConsumer consumer = [](spv_message_level_t level,
        const char*,
        const spv_position_t& position,
        const char* message) {
            switch (level) {
            case SPV_MSG_FATAL:
            case SPV_MSG_INTERNAL_ERROR:
            case SPV_MSG_ERROR:
                std::cerr << "error: " << position.index << ": " << message
                    << std::endl;
                break;
            case SPV_MSG_WARNING:
                std::cout << "warning: " << position.index << ": " << message
                    << std::endl;
                break;
            case SPV_MSG_INFO:
                std::cout << "info: " << position.index << ": " << message << std::endl;
                break;
            default:
                break;
            }
    };
	spvtools::Context context(SPV_ENV_UNIVERSAL_1_5);
	context.SetMessageConsumer(consumer);

    std::vector<std::vector<uint32_t>> contents;
    for (auto& m : m_modules) {
        contents.push_back(m->spv);
    }

    spvtools::LinkerOptions options;

	std::vector<uint32_t> ret;
	spv_result_t status = spvtools::Link(context, contents, &ret, options);
}

}
}