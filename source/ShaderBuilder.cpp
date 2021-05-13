#include "shadertrans/ShaderBuilder.h"
#include "shadertrans/ShaderTrans.h"
#include "shadertrans/SpirvTools.h"

#include <spvgentwo/Templates.h>
#include <spvgentwo/Reader.h>
#include <spvgentwo/Module.h>
#include <spvgentwo/Grammar.h>
#include <spvgentwo/TypeAlias.h>
#include <common/ConsoleLogger.h>
#include <common/HeapAllocator.h>
#include <common/LinkerHelper.h>
#include <common/ModulePrinter.h>
#include <common/BinaryVectorWriter.h>
#include <spirv-tools/linker.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <assert.h>

//#define SHADER_DEBUG_PRINT

namespace
{

template <typename U32Vector>
class BinaryVectorReader : public spvgentwo::IReader 
{
public:
	BinaryVectorReader(U32Vector& spv) : m_vec(spv), m_index(0) { }
	~BinaryVectorReader() { }

	bool get(unsigned int& _word) final
	{
		if (m_index >= m_vec.size())
			return false;
		_word = m_vec[m_index++];
		return true;
	}

private:
	U32Vector& m_vec;
	int m_index;
}; // BinaryVectorReader

}

namespace shadertrans
{

ShaderBuilder::ShaderBuilder()
{
	m_logger = std::make_unique<spvgentwo::ConsoleLogger>();
	m_alloc = std::make_unique<spvgentwo::HeapAllocator>();
	m_gram = std::make_unique<spvgentwo::Grammar>(m_alloc.get());

	InitMain();
}

ShaderBuilder::~ShaderBuilder()
{
}

spvgentwo::Instruction* ShaderBuilder::AddInput(const std::string& name, const std::string& type)
{
	auto itr = m_input_cache.find(name);
	if (itr != m_input_cache.end()) {
		return itr->second;
	}

	spvgentwo::Instruction* ret = nullptr;
	if (type == "float") {
		ret = m_main->input<float>(name.c_str());
	} else if (type == "vec2") {
		ret = m_main->input<spvgentwo::glsl::vec2>(name.c_str());
	} else if (type == "vec3") {
		ret = m_main->input<spvgentwo::glsl::vec3>(name.c_str());
	} else if (type == "vec4") {
		ret = m_main->input<spvgentwo::glsl::vec4>(name.c_str());
	}

	if (ret) {
		m_input_cache.insert({ name, ret });
	}

	return ret;
}

spvgentwo::Instruction* ShaderBuilder::AddOutput(const std::string& name, const std::string& type)
{
	auto itr = m_output_cache.find(name);
	if (itr != m_output_cache.end()) {
		return itr->second;
	}

	spvgentwo::Instruction* ret = nullptr;
	if (type == "float") {
		ret = m_main->output<float>(name.c_str());
	} else if (type == "vec2") {
		ret = m_main->output<spvgentwo::glsl::vec2>(name.c_str());
	} else if (type == "vec3") {
		ret = m_main->output<spvgentwo::glsl::vec3>(name.c_str());
	} else if (type == "vec4") {
		ret = m_main->output<spvgentwo::glsl::vec4>(name.c_str());
	}

	if (ret) {
		m_output_cache.insert({ name, ret });
	}

	return ret;
}

spvgentwo::Instruction* ShaderBuilder::AddUniform(spvgentwo::Module* module, const std::string& name, const std::string& type)
{
	auto itr = m_uniform_cache.find(name);
	if (itr != m_uniform_cache.end()) {
		return itr->second;
	}

	spvgentwo::Instruction* ret = nullptr;
	if (type == "float") {
		ret = module->uniformConstant<float>(name.c_str());
	} else if (type == "vec2") {
		ret = module->uniformConstant<spvgentwo::glsl::vec2>(name.c_str());
	} else if (type == "vec3") {
		ret = module->uniformConstant<spvgentwo::glsl::vec3>(name.c_str());
	} else if (type == "vec4") {
		ret = module->uniformConstant<spvgentwo::glsl::vec4>(name.c_str());
	} else if (type == "texture") {
		spvgentwo::dyn_sampled_image_t img{ spvgentwo::spv::Op::OpTypeFloat };
		img.imageType.depth = 0;
		ret = module->uniformConstant(name.c_str(), img);
	} else if (type == "cubemap") {
		spvgentwo::dyn_sampled_image_t img{ spvgentwo::spv::Op::OpTypeFloat };
		img.imageType.dimension = spvgentwo::spv::Dim::Cube;
		img.imageType.depth = 0;
		ret = module->uniformConstant(name.c_str(), img);
	}

	if (ret) {
		m_uniform_cache.insert({ name, ret });
	}

	return ret;
}

std::shared_ptr<spvgentwo::Module> ShaderBuilder::AddModule(ShaderStage stage, const std::string& glsl, const std::string& name)
{
	for (auto& itr : m_modules) {
		if (itr.first == name) {
			return itr.second;
		}
	}

    std::vector<unsigned int> spv;
    ShaderTrans::GLSL2SpirV(stage, glsl, spv, true);

	auto module = std::make_shared<spvgentwo::Module>(m_alloc.get(), spvgentwo::spv::AddressingModel::Logical, 
		spvgentwo::spv::MemoryModel::GLSL450, m_logger.get());

	// configure capabilities and extensions
	module->addCapability(spvgentwo::spv::Capability::Shader);
	module->addCapability(spvgentwo::spv::Capability::Linkage);

    BinaryVectorReader reader(spv);

    module->reset();
    module->readAndInit(reader, *m_gram);

	// clear entry points
	module->getEntryPoints().clear();
	module->getExecutionModes().clear();
	module->getNames().erase(module->getNames().begin());

	module->finalizeEntryPoints();
	module->assignIDs(m_gram.get());

	//Print(*module);

	m_modules.push_back(std::make_pair(name, module));

	return module;
}

void ShaderBuilder::ReplaceFunc(spvgentwo::Function* from, spvgentwo::Function* to)
{
	auto module = from->getModule();
	module->replace(from, to);
	module->assignIDs(m_gram.get());
}

void ShaderBuilder::AddLinkDecl(spvgentwo::Function* func, const std::string& name, bool is_export)
{
	if (is_export && m_added_export_link_decl.find(name) != m_added_export_link_decl.end()) {
		return;
	}

	spvgentwo::spv::LinkageType type = is_export ? spvgentwo::spv::LinkageType::Export : spvgentwo::spv::LinkageType::Import;
	spvgentwo::LinkerHelper::addLinkageDecoration(func->getFunction(), type, name.c_str());

	if (is_export) {
		m_added_export_link_decl.insert(name);
	}
}

void ShaderBuilder::ImportAll()
{
	auto printer = spvgentwo::ModulePrinter::ModuleSimpleFuncPrinter([](const char* str) {
		printf("%s", str);

#ifdef _WIN32
		OutputDebugStringA(str);
#endif
		});

	spvgentwo::LinkerHelper::LinkerOptions options{};
	options.flags = spvgentwo::LinkerHelper::LinkerOptionBits::All;
	options.grammar = m_gram.get();
	options.printer = &printer;
	options.allocator = m_alloc.get();

	for (auto& itr : m_modules) {
		spvgentwo::LinkerHelper::import(*itr.second, *m_main, options);
	}
}

void ShaderBuilder::FinishMain()
{
	m_main->finalizeEntryPoints();
	m_main->assignIDs(m_gram.get());
}

std::vector<uint32_t> ShaderBuilder::Link()
{
	ResetState();

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

    std::vector<std::vector<unsigned int>> contents;
    for (auto& itr : m_modules) 
	{
		std::vector<unsigned int> spv;
		spvgentwo::BinaryVectorWriter writer(spv);
		itr.second->write(writer);
        contents.emplace_back(spv);
    }
	{
		std::vector<unsigned int> spv;
		spvgentwo::BinaryVectorWriter writer(spv);
		m_main->write(writer);
		contents.emplace_back(spv);
	}

    spvtools::LinkerOptions options;

	std::vector<uint32_t> spv;
	spv_result_t status = spvtools::Link(context, contents, &spv, options);
#ifdef SHADER_DEBUG_PRINT
	std::string glsl;
	ShaderTrans::SpirV2GLSL(ShaderStage::PixelShader, spv, glsl);
	printf("%s\n", glsl.c_str());

	std::string dis;
	SpirvTools::Disassemble(spv.data(), spv.size(), &dis);
	printf("%s\n", dis.c_str());
#endif // SHADER_DEBUG_PRINT

	return spv;
}

std::string ShaderBuilder::ConnectCSMain(const std::string& main_glsl)
{
	auto spv = Link();
	
	std::string glsl;
	ShaderTrans::SpirV2GLSL(ShaderStage::PixelShader, spv, glsl);

	std::string ret = "#version 430\n";
	auto b_pos = glsl.find_first_of('\n');
	auto e_pos = glsl.find("void main()");
	ret += glsl.substr(b_pos, e_pos - b_pos);
	ret += main_glsl;
	
	return ret;
}

void ShaderBuilder::InitMain()
{
	m_main = std::make_unique<spvgentwo::Module>(m_alloc.get(), spvgentwo::spv::AddressingModel::Logical,
		spvgentwo::spv::MemoryModel::GLSL450, m_logger.get());

	// configure capabilities and extensions
	m_main->addCapability(spvgentwo::spv::Capability::Shader);
	m_main->addCapability(spvgentwo::spv::Capability::Linkage);

	spvgentwo::EntryPoint& entry = m_main->addEntryPoint(spvgentwo::spv::ExecutionModel::Fragment, u8"main");
	entry.addExecutionMode(spvgentwo::spv::ExecutionMode::OriginUpperLeft);

	m_main_func = &entry;
}

void ShaderBuilder::ResetState()
{
	m_added_export_link_decl.clear();

	m_input_cache.clear();
	m_output_cache.clear();
	m_uniform_cache.clear();
}

}