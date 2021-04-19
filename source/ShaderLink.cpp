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
}

ShaderLink::~ShaderLink()
{
}

void ShaderLink::AddLibrary(ShaderStage stage, const std::string& glsl)
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

	auto& func = module->getFunctions().front();
	auto name = "entry_point" + std::to_string(m_libs.size());
	spvgentwo::LinkerHelper::addLinkageDecoration(func.getFunction(), spvgentwo::spv::LinkageType::Export, name.c_str());

	// clear entry points
	module->getEntryPoints().clear();
	module->getExecutionModes().clear();
	module->getNames().erase(module->getNames().begin());

	module->finalizeEntryPoints();
	module->assignIDs(m_gram.get());

	//Print(*module);

	m_libs.push_back(module);
}

void ShaderLink::Link()
{
	InitConsumer();

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
		m_consumer->write(writer);
		contents.emplace_back(spv);
	}

    spvtools::LinkerOptions options;

	std::vector<uint32_t> spv;
	spv_result_t status = spvtools::Link(context, contents, &spv, options);

	//////////////////////////////////////////////////////////////////////////////

	std::string glsl;
	ShaderTrans::SpirV2GLSL(ShaderStage::PixelShader, spv, glsl);
	printf("%s\n", glsl.c_str());

	//std::string dis;
	//SpirvTools::Disassemble(spv.data(), spv.size(), &dis);
	//printf("%s\n", dis.c_str());
}


void ShaderLink::InitConsumer()
{
	m_consumer = std::make_unique<spvgentwo::Module>(m_alloc.get(), spvgentwo::spv::AddressingModel::Logical,
		spvgentwo::spv::MemoryModel::GLSL450, m_logger.get());

	// configure capabilities and extensions
	m_consumer->addCapability(spvgentwo::spv::Capability::Shader);
	m_consumer->addCapability(spvgentwo::spv::Capability::Linkage);

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
		spvgentwo::LinkerHelper::import(*m, *m_consumer, options);
	}

	auto& func0 = CreateFuncSign(m_libs[0]->getFunctions().front());
	auto& func1 = CreateFuncSign(m_libs[1]->getFunctions().front());

	spvgentwo::LinkerHelper::addLinkageDecoration(func0.getFunction(), spvgentwo::spv::LinkageType::Import, "entry_point0");
	spvgentwo::LinkerHelper::addLinkageDecoration(func1.getFunction(), spvgentwo::spv::LinkageType::Import, "entry_point1");

	// out vec4 frag_out;
	spvgentwo::Type type = m_consumer->newType();
	type.VectorElement(4).Float();
	spvgentwo::Instruction* out_frag_out = m_consumer->output(type, u8"frag_out");

	// void main();
	{
		spvgentwo::EntryPoint& entry = m_consumer->addEntryPoint(spvgentwo::spv::ExecutionModel::Fragment, u8"main");
		entry.addExecutionMode(spvgentwo::spv::ExecutionMode::OriginUpperLeft);
		spvgentwo::BasicBlock& bb = *entry;

		spvgentwo::Instruction* uv = m_consumer->constant(spvgentwo::make_vector(0.f, 0.f));
		spvgentwo::Instruction* width = m_consumer->constant(0.f);
		spvgentwo::Instruction* height = m_consumer->constant(0.f);

		spvgentwo::Instruction* col0 = bb->call(&func0, uv, width, height);
		auto out_x = bb->opAccessChain(out_frag_out, 0u);
		bb->opStore(out_x, col0);

		spvgentwo::Instruction* col1 = bb->call(&func1, uv, width, height);
		auto out_y = bb->opAccessChain(out_frag_out, 1u);
		bb->opStore(out_y, col1);

		entry->opReturn();
	}

	m_consumer->finalizeEntryPoints();
	m_consumer->assignIDs(m_gram.get());

	Print(*m_consumer);
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

spvgentwo::Function& ShaderLink::CreateFuncSign(const spvgentwo::Function& src) const
{
	spvgentwo::Function& dst = m_consumer->addFunction();

	dst.setReturnType(m_consumer->addType(src.getReturnType()));
	for (auto& p : src.getParameters()) {
		dst.addParameters(m_consumer->addType(*p.getType()));
	}

	dst.finalize(src.getFunctionControl(), src.getName());

	return dst;
}

}