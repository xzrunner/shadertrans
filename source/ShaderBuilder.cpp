#include "shadertrans/ShaderBuilder.h"
#include "shadertrans/ShaderTrans.h"
#include "shadertrans/SpirvTools.h"
#include "shadertrans/ShaderRename.h"
#include "shadertrans/ShaderPreprocess.h"
#include "shadertrans/SpirvGenTwo.h"

#include <spvgentwo/SpvGenTwo.h>
#include <spvgentwo/Grammar.h>
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

#include <fstream>
#include <streambuf>
#include <filesystem>
#include <assert.h>

//#define UNIQUE_INCLUDE_MODULE

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

	if (type == "bool") {
		ret = m_main->input<bool>(name.c_str());
	} else if (type == "int") {
		ret = m_main->input<int>(name.c_str());
	} else if (type == "float") {
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
	if (type == "bool") {
		ret = m_main->output<bool>(name.c_str());
	} else if (type == "int") {
		ret = m_main->output<int>(name.c_str());
	} else if (type == "float") {
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
	std::string unif_name = GetAvaliableUnifName(name);

	spvgentwo::Instruction* ret = nullptr;
	if (type == "bool") {
		ret = m_main->uniformConstant<bool>(name.c_str());
	} else if (type == "int") {
		ret = m_main->uniformConstant<int>(name.c_str());
	} else if (type == "float") {
		ret = module->uniformConstant<float>(unif_name.c_str());
	} else if (type == "vec2") {
		ret = module->uniformConstant<spvgentwo::glsl::vec2>(unif_name.c_str());
	} else if (type == "vec3") {
		ret = module->uniformConstant<spvgentwo::glsl::vec3>(unif_name.c_str());
	} else if (type == "vec4") {
		ret = module->uniformConstant<spvgentwo::glsl::vec4>(unif_name.c_str());
	} else if (type == "texture") {
		spvgentwo::dyn_sampled_image_t img{ spvgentwo::spv::Op::OpTypeFloat };
		img.imageType.depth = 0;
		ret = module->uniformConstant(unif_name.c_str(), img);
	} else if (type == "cubemap") {
		spvgentwo::dyn_sampled_image_t img{ spvgentwo::spv::Op::OpTypeFloat };
		img.imageType.dimension = spvgentwo::spv::Dim::Cube;
		img.imageType.depth = 0;
		ret = module->uniformConstant(unif_name.c_str(), img);
	}

	if (ret) {
		m_uniform_cache.insert({ unif_name, ret });
	}

	return ret;
}

const char* ShaderBuilder::QueryUniformName(const spvgentwo::Instruction* unif) const
{
	for (auto& itr : m_uniform_cache) {
		if (itr.second == unif) {
			return itr.first.c_str();
		}
	}
	return nullptr;
}

std::shared_ptr<ShaderBuilder::Module> 
ShaderBuilder::AddModule(ShaderStage stage, const std::string& _code, const std::string& lang, const std::string& name, const std::string& entry_point)
{
	auto module = FindModule(name);
	if (module) {
		return module;
	}

	module = std::make_shared<Module>();
	module->name = name;

#ifdef UNIQUE_INCLUDE_MODULE
	std::vector<std::string> include_paths;
	auto code = ShaderPreprocess::RemoveIncludes(_code, include_paths);
	for (auto& inc : include_paths)
	{
		if (!std::filesystem::exists(inc)) {
			printf("Can't find file %s\n", inc.c_str());
			continue;
		}

		auto absolute = std::filesystem::absolute(inc).string();
		auto inc_module = FindModule(absolute);
		if (!inc_module) 
		{
			std::ifstream fin(absolute.c_str());
			std::string str((std::istreambuf_iterator<char>(fin)),
				std::istreambuf_iterator<char>());
			inc_module = AddModule(stage, str, lang, name, entry_point);

			// export funcs
			int idx = 0;
			for (auto& func : inc_module->impl->getFunctions()) 
			{
				std::string decl_name = func->getFunction()->getName();
				decl_name = decl_name.substr(0, decl_name.find_first_of("("));
				
				//std::string decl_name = absolute + "_func" + std::to_string(idx++);
				AddLinkDecl(&func, decl_name, true);
			}
		}
		module->includes.push_back(inc_module);
	}
#else
	std::string code = _code;
	if (lang == "glsl") {
		code = ShaderPreprocess::PrepareGLSL(code);
	}
#endif // UNIQUE_INCLUDE_MODULE

	auto spv_module = std::make_shared<spvgentwo::Module>(m_alloc.get(), spvgentwo::spv::AddressingModel::Logical, 
		spvgentwo::spv::MemoryModel::GLSL450, m_logger.get());

	spv_module->reset();

#ifdef UNIQUE_INCLUDE_MODULE
	for (auto& inc : module->includes)
	{
		// import funcs
		int idx = 0;
		for (auto& func : inc->impl->getFunctions())
		{
			auto _func = SpirvGenTwo::CreateDeclFunc(spv_module.get(), &func);

			std::string decl_name = func->getFunction()->getName();
			decl_name = decl_name.substr(0, decl_name.find_first_of("("));

			//std::string decl_name = inc->name + "_func" + std::to_string(idx++);
			AddLinkDecl(_func, decl_name, false);
		}
	}
#endif // UNIQUE_INCLUDE_MODULE

	// configure capabilities and extensions
	spv_module->addCapability(spvgentwo::spv::Capability::Shader);
	spv_module->addCapability(spvgentwo::spv::Capability::Linkage);

	std::vector<unsigned int> spv;
    if (lang == "glsl") {
        shadertrans::ShaderTrans::GLSL2SpirV(stage, code, spv, true);
    }  else if (lang == "hlsl") {
        shadertrans::ShaderTrans::HLSL2SpirV(stage, code, entry_point, spv);
    }

    BinaryVectorReader reader(spv);

    spv_module->readAndInit(reader, *m_gram);

	// clear entry points
	spv_module->getEntryPoints().clear();
	spv_module->getExecutionModes().clear();
	spv_module->getNames().erase(spv_module->getNames().begin());

	spv_module->finalizeEntryPoints();
	spv_module->assignIDs(m_gram.get());

	module->impl = spv_module;

	m_modules.push_back(module);

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

	for (auto& module : m_modules) {
		spvgentwo::LinkerHelper::import(*module->impl, *m_main, options);
	}
}

void ShaderBuilder::FinishMain()
{
	m_main->finalizeEntryPoints();
	m_main->assignIDs(m_gram.get());
}

std::vector<uint32_t> ShaderBuilder::Link()
{
	return LinkSpvtools();
	//return LinkSpvgentwo();
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

std::string ShaderBuilder::GetAvaliableUnifName(const std::string& name) const
{
	if (m_uniform_cache.find(name) == m_uniform_cache.end()) {
		return name;
	}

	int i = 1;
	while (true)
	{
		std::string _name = name + std::to_string(i++);
		if (m_uniform_cache.find(_name) == m_uniform_cache.end()) {
			return _name;
		}
	}

	return name;
}

std::shared_ptr<ShaderBuilder::Module> ShaderBuilder::FindModule(const std::string& name) const
{
	for (auto& module : m_modules) {
		if (module->name == name) {
			return module;
		}
	}
	return nullptr;
}

std::vector<uint32_t> ShaderBuilder::LinkSpvtools()
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
    for (auto& module : m_modules)
	{
		std::vector<unsigned int> spv;
		spvgentwo::BinaryVectorWriter writer(spv);
		module->impl->write(writer);
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

	ShaderRename rename(spv);
	rename.FillingUBOInstName();
	rename.RenameSampledImages();
	spv = rename.GetResult(ShaderStage::PixelShader);

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

std::vector<uint32_t> ShaderBuilder::LinkSpvgentwo()
{
	ResetState();

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

	std::vector<unsigned int> spv;

	bool success = true;
	for (auto& module : m_modules) {
		success &= spvgentwo::LinkerHelper::import(*module->impl, *m_main, options);
	}

	if (success == false) {
		return spv;
	}

	//m_main->finalizeEntryPoints();


	spvgentwo::BinaryVectorWriter writer(spv);
	// don't need to call assignIDs, we are using AssignResultIDs
	if (m_main->write(writer) == false) {
		return spv;
	}

#ifdef SHADER_DEBUG_PRINT
	std::string glsl;
	ShaderTrans::SpirV2GLSL(ShaderStage::PixelShader, spv, glsl);
	printf("%s\n", glsl.c_str());
#endif // SHADER_DEBUG_PRINT

	return spv;
}

}