#include "shadertrans/ShaderLink.h"
#include "shadertrans/ShaderTrans.h"
#include "shadertrans/SpirvTools.h"

#include <spvgentwo/Reader.h>
#include <spvgentwo/Module.h>
#include <spvgentwo/Grammar.h>
#include <spvgentwo/Templates.h>
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

ShaderLink::ShaderLink()
{
	m_logger = std::make_unique<spvgentwo::ConsoleLogger>();
	m_alloc = std::make_unique<spvgentwo::HeapAllocator>();
	m_gram = std::make_unique<spvgentwo::Grammar>(m_alloc.get());

	InitMain();
}

ShaderLink::~ShaderLink()
{
}

spvgentwo::Instruction* ShaderLink::AddInput(const std::string& name, const std::string& type)
{
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
	return ret;
}

spvgentwo::Instruction* ShaderLink::AddOutput(const std::string& name, const std::string& type)
{
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
	return ret;
}

spvgentwo::Instruction* ShaderLink::AddUniform(const std::string& name, const std::string& type)
{
	spvgentwo::Instruction* ret = nullptr;
	if (type == "float") {
		ret = m_main->uniform<float>(name.c_str());
	} else if (type == "vec2") {
		ret = m_main->uniform<spvgentwo::glsl::vec2>(name.c_str());
	} else if (type == "vec3") {
		ret = m_main->uniform<spvgentwo::glsl::vec3>(name.c_str());
	} else if (type == "vec4") {
		ret = m_main->uniform<spvgentwo::glsl::vec4>(name.c_str());
	}
	return ret;
}

spvgentwo::Instruction* ShaderLink::ConstFloat(float x)
{
	return m_main->constant(x);
}

spvgentwo::Instruction* ShaderLink::ConstFloat2(float x, float y)
{
	return m_main->constant(spvgentwo::make_vector(x, y));
}

spvgentwo::Instruction* ShaderLink::ConstFloat3(float x, float y, float z)
{
	return m_main->constant(spvgentwo::make_vector(x, y, z));
}

spvgentwo::Instruction* ShaderLink::ConstFloat4(float x, float y, float z, float w)
{
	return m_main->constant(spvgentwo::make_vector(x, y, z, w));
}

spvgentwo::Instruction* ShaderLink::Call(spvgentwo::Function* func, const std::vector<spvgentwo::Instruction*>& args)
{
	switch (args.size())
	{
	case 0:
		return (*m_main_entry)->call(func);
	case 1:
		return (*m_main_entry)->call(func, args[0]);
	case 2:
		return (*m_main_entry)->call(func, args[0], args[1]);
	case 3:
		return (*m_main_entry)->call(func, args[0], args[1], args[2]);
	case 4:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3]);
	case 5:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4]);
	case 6:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5]);
	case 7:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
	case 8:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
	case 9:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
	case 10:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
	case 11:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10]);
	case 12:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11]);
	case 13:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12]);
	case 14:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13]);
	case 15:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14]);
	case 16:
		return (*m_main_entry)->call(func, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15]);
	default:
		return nullptr;
	}
}

spvgentwo::Instruction* ShaderLink::AccessChain(spvgentwo::Instruction* base, unsigned int index)
{
	return (*m_main_entry)->opAccessChain(base, index);
}

spvgentwo::Instruction* ShaderLink::ComposeFloat2(spvgentwo::Instruction* x,
	                                              spvgentwo::Instruction* y)
{
	spvgentwo::Instruction* type = (*m_main_entry)->getModule()->type<spvgentwo::vector_t<float, 2>>();
	return (*m_main_entry)->opCompositeConstruct(type, x, y);
}

spvgentwo::Instruction* ShaderLink::ComposeFloat3(spvgentwo::Instruction* x,
	                                              spvgentwo::Instruction* y, 
	                                              spvgentwo::Instruction* z)
{
	spvgentwo::Instruction* type = (*m_main_entry)->getModule()->type<spvgentwo::vector_t<float, 3>>();
	return (*m_main_entry)->opCompositeConstruct(type, x, y, z);
}

spvgentwo::Instruction* ShaderLink::ComposeFloat4(spvgentwo::Instruction* x,
	                                              spvgentwo::Instruction* y, 
	                                              spvgentwo::Instruction* z,
	                                              spvgentwo::Instruction* w)
{
	spvgentwo::Instruction* type = (*m_main_entry)->getModule()->type<spvgentwo::vector_t<float, 4>>();
	return (*m_main_entry)->opCompositeConstruct(type, x, y, z, w);
}

spvgentwo::Instruction* ShaderLink::ComposeFloat4(spvgentwo::Instruction* a,
	                                              spvgentwo::Instruction* b)
{
	spvgentwo::Instruction* type = (*m_main_entry)->getModule()->type<spvgentwo::vector_t<float, 4>>();
	return (*m_main_entry)->opCompositeConstruct(type, a, b);
}

void ShaderLink::Store(spvgentwo::Instruction* dst, spvgentwo::Instruction* src)
{
	(*m_main_entry)->opStore(dst, src);
}

void ShaderLink::Return()
{
	(*m_main_entry)->opReturn();
}

std::shared_ptr<spvgentwo::Module> ShaderLink::AddLibrary(ShaderStage stage, const std::string& glsl)
{
    std::vector<unsigned int> spv;
    ShaderTrans::GLSL2SpirV(stage, glsl, spv, true);

	auto module = std::make_shared<spvgentwo::Module>(m_alloc.get(), spvgentwo::spv::AddressingModel::Logical, 
		spvgentwo::spv::MemoryModel::GLSL450, m_logger.get());

	// configure capabilities and extensions
	module->addCapability(spvgentwo::spv::Capability::Shader);
	module->addCapability(spvgentwo::spv::Capability::Linkage);

    BinaryVectorReader reader(spv);

    module->reset();
    module->read(reader, *m_gram);
    module->resolveIDs();
    module->reconstructTypeAndConstantInfo();
    module->reconstructNames();

	// clear entry points
	module->getEntryPoints().clear();
	module->getExecutionModes().clear();
	module->getNames().erase(module->getNames().begin());

	module->finalizeEntryPoints();
	module->assignIDs(m_gram.get());

	//Print(*module);

	m_libs.push_back(module);

	return module;
}

spvgentwo::Function* ShaderLink::GetEntryFunc(spvgentwo::Module& lib)
{
	return &lib.getFunctions().front();
}

spvgentwo::Function* ShaderLink::CreateDeclFunc(spvgentwo::Function* func) const
{
	spvgentwo::Function& dst = m_main->addFunction();

	dst.setReturnType(m_main->addType(func->getReturnType()));
	for (auto& p : func->getParameters()) {
		dst.addParameters(m_main->addType(*p.getType()));
	}

	dst.finalize(func->getFunctionControl(), func->getName());

	return &dst;
}

void ShaderLink::AddLinkDecl(spvgentwo::Function* func, const std::string& name, bool is_export) const
{
	spvgentwo::spv::LinkageType type = is_export ? spvgentwo::spv::LinkageType::Export : spvgentwo::spv::LinkageType::Import;
	spvgentwo::LinkerHelper::addLinkageDecoration(func->getFunction(), type, name.c_str());
}

void ShaderLink::ImportAll()
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

	for (auto& m : m_libs) {
		spvgentwo::LinkerHelper::import(*m, *m_main, options);
	}
}

void ShaderLink::FinishMain()
{
	m_main->finalizeEntryPoints();
	m_main->assignIDs(m_gram.get());
}

std::string ShaderLink::Link()
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

    std::vector<std::vector<unsigned int>> contents;
    for (auto& m : m_libs) 
	{
		std::vector<unsigned int> spv;
		spvgentwo::BinaryVectorWriter writer(spv);
		m->write(writer);
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

	//////////////////////////////////////////////////////////////////////////////

	std::string glsl;
	ShaderTrans::SpirV2GLSL(ShaderStage::PixelShader, spv, glsl);
	//printf("%s\n", glsl.c_str());

	//std::string dis;
	//SpirvTools::Disassemble(spv.data(), spv.size(), &dis);
	//printf("%s\n", dis.c_str());

	return glsl;
}

void ShaderLink::InitMain()
{
	m_main = std::make_unique<spvgentwo::Module>(m_alloc.get(), spvgentwo::spv::AddressingModel::Logical,
		spvgentwo::spv::MemoryModel::GLSL450, m_logger.get());

	// configure capabilities and extensions
	m_main->addCapability(spvgentwo::spv::Capability::Shader);
	m_main->addCapability(spvgentwo::spv::Capability::Linkage);

	spvgentwo::EntryPoint& entry = m_main->addEntryPoint(spvgentwo::spv::ExecutionModel::Fragment, u8"main");
	entry.addExecutionMode(spvgentwo::spv::ExecutionMode::OriginUpperLeft);

	m_main_entry = &entry;
}

void ShaderLink::Print(const spvgentwo::Module& module) const
{
	std::vector<unsigned int> spv;
	spvgentwo::BinaryVectorWriter writer(spv);
	module.write(writer);

	std::string glsl;
	ShaderTrans::SpirV2GLSL(ShaderStage::PixelShader, spv, glsl);
	printf("%s\n", glsl.c_str());

	std::string dis;
	SpirvTools::Disassemble(spv.data(), spv.size(), &dis);
	printf("%s\n", dis.c_str());
}

}