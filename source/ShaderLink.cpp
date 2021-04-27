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

#include <assert.h>

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

spvgentwo::Type create_type(spvgentwo::Module* module, const std::string& str)
{
	auto type = module->newType();
	if (str == "float") {
		type.Float();
	} else if (str == "vec2") {
		type.VectorElement(2).Float();
	} else if (str == "vec3") {
		type.VectorElement(3).Float();
	} else if (str == "vec4") {
		type.VectorElement(4).Float();
	} else {
		assert(0);
	}
	return type;
}

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

spvgentwo::Instruction* ShaderLink::AccessChain(spvgentwo::Function* func, spvgentwo::Instruction* base, unsigned int index)
{
	return (*func)->opAccessChain(base, index);
}

spvgentwo::Instruction* ShaderLink::ComposeFloat2(spvgentwo::Function* func,
	                                              spvgentwo::Instruction* x,
	                                              spvgentwo::Instruction* y)
{
	spvgentwo::Instruction* type = (*func)->getModule()->type<spvgentwo::vector_t<float, 2>>();
	return (*func)->opCompositeConstruct(type, x, y);
}

spvgentwo::Instruction* ShaderLink::ComposeFloat3(spvgentwo::Function* func, 
	                                              spvgentwo::Instruction* x,
	                                              spvgentwo::Instruction* y, 
	                                              spvgentwo::Instruction* z)
{
	spvgentwo::Instruction* type = (*func)->getModule()->type<spvgentwo::vector_t<float, 3>>();
	return (*func)->opCompositeConstruct(type, x, y, z);
}

spvgentwo::Instruction* ShaderLink::ComposeFloat4(spvgentwo::Function* func, 
	                                              spvgentwo::Instruction* x,
	                                              spvgentwo::Instruction* y, 
	                                              spvgentwo::Instruction* z,
	                                              spvgentwo::Instruction* w)
{
	spvgentwo::Instruction* type = (*func)->getModule()->type<spvgentwo::vector_t<float, 4>>();
	return (*func)->opCompositeConstruct(type, x, y, z, w);
}

spvgentwo::Instruction* ShaderLink::ComposeExtract(spvgentwo::Function* func,
	                                               spvgentwo::Instruction* comp, 
	                                               unsigned int index)
{
	return (*func)->opCompositeExtract(comp, index);
}

spvgentwo::Instruction* ShaderLink::Dot(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	return (*func)->opDot(a, b);
}

spvgentwo::Instruction* ShaderLink::Add(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	return (*func)->Add(a, b);
}

spvgentwo::Instruction* ShaderLink::Sub(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	return (*func)->Sub(a, b);
}

spvgentwo::Instruction* ShaderLink::Mul(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	return (*func)->Mul(a, b);
}

spvgentwo::Instruction* ShaderLink::Div(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	return (*func)->Div(a, b);
}

void ShaderLink::Store(spvgentwo::Function* func, spvgentwo::Instruction* dst, spvgentwo::Instruction* src)
{
	(*func)->opStore(dst, src);
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
    module->readAndInit(reader, *m_gram);

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

spvgentwo::Function* ShaderLink::QueryFunction(spvgentwo::Module& lib, const std::string& name)
{
	for (auto& func : lib.getFunctions()) 
	{
		std::string f_name = func.getName();
		f_name = f_name.substr(0, f_name.find_first_of('('));
		if (f_name == name) {
			return &func;
		}
	}
	return nullptr;
}

void ShaderLink::ReplaceFunc(spvgentwo::Function* from, spvgentwo::Function* to)
{
	auto module = from->getModule();
	module->replace(from, to);
	module->assignIDs(m_gram.get());
}

spvgentwo::Function* ShaderLink::CreateDeclFunc(spvgentwo::Module* module, spvgentwo::Function* func)
{
	spvgentwo::Function& dst = module->addFunction();

	dst.setReturnType(module->addType(func->getReturnType()));
	for (auto& p : func->getParameters()) {
		dst.addParameters(module->addType(*p.getType()));
	}

	dst.finalize(func->getFunctionControl(), func->getName());

	return &dst;
}

void ShaderLink::AddLinkDecl(spvgentwo::Function* func, const std::string& name, bool is_export)
{
	spvgentwo::spv::LinkageType type = is_export ? spvgentwo::spv::LinkageType::Export : spvgentwo::spv::LinkageType::Import;
	spvgentwo::LinkerHelper::addLinkageDecoration(func->getFunction(), type, name.c_str());
}

spvgentwo::Function* ShaderLink::CreateFunc(spvgentwo::Module* module, const std::string& name, 
	                                        const std::string& ret, const std::vector<std::string>& args)
{
	auto& func = module->addFunction();
	
	func.setReturnType(module->addType(create_type(module, ret)));
	for (auto& arg : args) {
		func.addParameters(module->addType(create_type(module, arg)));
	}
	func.finalize(spvgentwo::spv::FunctionControlMask::Const, name.c_str());
	func.addBasicBlock("FunctionEntry");

	return &func;
}

spvgentwo::Instruction* ShaderLink::GetFuncParam(spvgentwo::Function* func, int index)
{
	return func->getParameter(index);
}

void ShaderLink::GetFuncParamNames(spvgentwo::Function* func, std::vector<std::string>& names)
{
	for (auto& param : func->getParameters()) {
		names.push_back(param.getName());
	}
}

spvgentwo::Instruction* ShaderLink::FuncCall(spvgentwo::Function* caller, spvgentwo::Function* callee, const std::vector<spvgentwo::Instruction*>& params)
{
	spvgentwo::BasicBlock& bb = *caller;
	switch (params.size())
	{
	case 0:
		return bb->call(callee);
	case 1:
		return bb->call(callee, params[0]);
	case 2:
		return bb->call(callee, params[0], params[1]);
	case 3:
		return bb->call(callee, params[0], params[1], params[2]);
	case 4:
		return bb->call(callee, params[0], params[1], params[2], params[3]);
	case 5:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4]);
	case 6:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5]);
	case 7:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5], params[6]);
	case 8:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7]);
	case 9:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8]);
	case 10:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9]);
	case 11:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9], params[10]);
	case 12:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9], params[10], params[11]);
	case 13:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9], params[10], params[11], params[12]);
	case 14:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9], params[10], params[11], params[12], params[13]);
	case 15:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9], params[10], params[11], params[12], params[13], params[14]);
	case 16:
		return bb->call(callee, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9], params[10], params[11], params[12], params[13], params[14], params[15]);
	default:
		return nullptr;
	}
}

void ShaderLink::Return(spvgentwo::Function* func)
{
	(*func)->opReturn();
}

void ShaderLink::ReturnValue(spvgentwo::Function* func, spvgentwo::Instruction* inst)
{
	(*func)->opReturnValue(inst);
}

spvgentwo::Instruction* ShaderLink::ConstFloat(spvgentwo::Module* module, float x)
{
	return module->constant(x);
}

spvgentwo::Instruction* ShaderLink::ConstFloat2(spvgentwo::Module* module, float x, float y)
{
	return module->constant(spvgentwo::make_vector(x, y));
}

spvgentwo::Instruction* ShaderLink::ConstFloat3(spvgentwo::Module* module, float x, float y, float z)
{
	return module->constant(spvgentwo::make_vector(x, y, z));
}

spvgentwo::Instruction* ShaderLink::ConstFloat4(spvgentwo::Module* module, float x, float y, float z, float w)
{
	return module->constant(spvgentwo::make_vector(x, y, z, w));
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

void ShaderLink::Print(const spvgentwo::Module& module, bool output_ir)
{
	std::vector<unsigned int> spv;
	spvgentwo::BinaryVectorWriter writer(spv);
	module.write(writer);

	std::string glsl;
	ShaderTrans::SpirV2GLSL(ShaderStage::PixelShader, spv, glsl);
	printf("%s\n", glsl.c_str());

	if (output_ir)
	{
		std::string dis;
		SpirvTools::Disassemble(spv.data(), spv.size(), &dis);
		printf("%s\n", dis.c_str());
	}
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

	m_main_func = &entry;
}

}