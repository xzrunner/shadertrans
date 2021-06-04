#include "shadertrans/SpirvGenTwo.h"
#include "shadertrans/ShaderTrans.h"
#include "shadertrans/SpirvTools.h"

#include <spvgentwo/Instruction.h>
#include <spvgentwo/Type.h>
#include <spvgentwo/Templates.h>
#include <spvgentwo/GLSL450Instruction.h>
#include <spvgentwo/TypeAlias.h>
#include <common/BinaryVectorWriter.h>

#include <assert.h>

namespace
{

spvgentwo::Type create_type(spvgentwo::Module* module, const std::string& str)
{
	auto type = module->newType();
	if (str == "bool") {
		type.Bool();
	} else if (str == "int") {
		type.Int();
	} else if (str == "float") {
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

// info

const char* SpirvGenTwo::GetType(const spvgentwo::Instruction& inst)
{
	return inst.getType()->getString();
}

bool SpirvGenTwo::IsVector(const spvgentwo::Instruction& inst)
{
	return inst.getType()->isVector();
}

int SpirvGenTwo::GetVectorNum(const spvgentwo::Instruction& inst)
{
	return inst.getType()->getVectorComponentCount();
}

// inst

spvgentwo::Instruction* SpirvGenTwo::AccessChain(spvgentwo::Function* func, 
	                                             spvgentwo::Instruction* base, 
	                                             unsigned int index)
{
	if (!base) {
		return nullptr;
	}
	return (*func)->opAccessChain(base, index);
}

spvgentwo::Instruction* SpirvGenTwo::ComposeFloat2(spvgentwo::Function* func,
	                                               spvgentwo::Instruction* x,
	                                               spvgentwo::Instruction* y)
{
	if (!x || !y) {
		return nullptr;
	}
	spvgentwo::Instruction* type = (*func)->getModule()->type<spvgentwo::vector_t<float, 2>>();
	return (*func)->opCompositeConstruct(type, x, y);
}

spvgentwo::Instruction* SpirvGenTwo::ComposeFloat3(spvgentwo::Function* func, 
	                                               spvgentwo::Instruction* x,
	                                               spvgentwo::Instruction* y, 
	                                               spvgentwo::Instruction* z)
{
	if (!x || !y || !z) {
		return nullptr;
	}
	spvgentwo::Instruction* type = (*func)->getModule()->type<spvgentwo::vector_t<float, 3>>();
	return (*func)->opCompositeConstruct(type, x, y, z);
}

spvgentwo::Instruction* SpirvGenTwo::ComposeFloat4(spvgentwo::Function* func, 
	                                               spvgentwo::Instruction* x,
	                                               spvgentwo::Instruction* y, 
	                                               spvgentwo::Instruction* z,
	                                               spvgentwo::Instruction* w)
{
	if (!x || !y || !z || !w) {
		return nullptr;
	}
	spvgentwo::Instruction* type = (*func)->getModule()->type<spvgentwo::vector_t<float, 4>>();
	return (*func)->opCompositeConstruct(type, x, y, z, w);
}

spvgentwo::Instruction* SpirvGenTwo::ComposeExtract(spvgentwo::Function* func,
	                                                spvgentwo::Instruction* comp, 
	                                                unsigned int index)
{
	if (!comp) {
		return nullptr;
	}
	return (*func)->opCompositeExtract(comp, index);
}

spvgentwo::Instruction* SpirvGenTwo::Dot(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	if (!a || !b) {
		return nullptr;
	}
	return (*func)->opDot(a, b);
}

spvgentwo::Instruction* SpirvGenTwo::Cross(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	if (!a || !b) {
		return nullptr;
	}
	spvgentwo::BasicBlock& bb = *func;
	return bb.ext<spvgentwo::ext::GLSL>()->opCross(a, b);
}

spvgentwo::Instruction* SpirvGenTwo::Add(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	if (!a || !b) {
		return nullptr;
	}
	return (*func)->Add(a, b);
}

spvgentwo::Instruction* SpirvGenTwo::Sub(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{	
	if (!a || !b) {
		return nullptr;
	}
	return (*func)->Sub(a, b);
}

spvgentwo::Instruction* SpirvGenTwo::Mul(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	if (!a || !b) {
		return nullptr;
	}
	return (*func)->Mul(a, b);
}

spvgentwo::Instruction* SpirvGenTwo::Div(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	if (!a || !b) {
		return nullptr;
	}
	return (*func)->Div(a, b);
}

spvgentwo::Instruction* SpirvGenTwo::Negate(spvgentwo::Function* func, spvgentwo::Instruction* v)
{
	if (!v) {
		return nullptr;
	}
	if (v->getType()->isFloat()) {
		return (*func)->opFNegate(v);
	} else if (v->getType()->isInt()) {
		return (*func)->opSNegate(v);
	} else {
		return nullptr;
	}
}

spvgentwo::Instruction* SpirvGenTwo::Reflect(spvgentwo::Function* func, spvgentwo::Instruction* I, spvgentwo::Instruction* N)
{
	if (!I || !N) {
		return nullptr;
	}
	spvgentwo::BasicBlock& bb = *func;
	return bb.ext<spvgentwo::ext::GLSL>()->opReflect(I, N);
}

spvgentwo::Instruction* SpirvGenTwo::Sqrt(spvgentwo::Function* func, spvgentwo::Instruction* v)
{
	if (!v) {
		return nullptr;
	}
	spvgentwo::BasicBlock& bb = *func;
	return bb.ext<spvgentwo::ext::GLSL>()->opSqrt(v);
}

spvgentwo::Instruction* SpirvGenTwo::Pow(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* y)
{
	if (!x || !y) {
		return nullptr;
	}
	spvgentwo::BasicBlock& bb = *func;
	return bb.ext<spvgentwo::ext::GLSL>()->opPow(x, y);
}

spvgentwo::Instruction* SpirvGenTwo::Normalize(spvgentwo::Function* func, spvgentwo::Instruction* v)
{
	if (!v || !IsVector(*v)) {
		return nullptr;
	}
	spvgentwo::BasicBlock& bb = *func;
	return bb.ext<spvgentwo::ext::GLSL>()->opNormalize(v);
}

spvgentwo::Instruction* SpirvGenTwo::Length(spvgentwo::Function* func, spvgentwo::Instruction* v)
{
	if (!v || !IsVector(*v)) {
		return nullptr;
	}
	spvgentwo::BasicBlock& bb = *func;
	return bb.ext<spvgentwo::ext::GLSL>()->opLength(v);
}

spvgentwo::Instruction* SpirvGenTwo::Max(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	if (!a || !b) {
		return nullptr;
	}
	spvgentwo::BasicBlock& bb = *func;
	if (a->getType()->isFloat()) {
		return bb.ext<spvgentwo::ext::GLSL>()->opFMax(a, b);
	} else if (a->getType()->isInt()) {
		return bb.ext<spvgentwo::ext::GLSL>()->opSMax(a, b);
	} else if (a->getType()->isUnsigned()) {
		return bb.ext<spvgentwo::ext::GLSL>()->opUMax(a, b);
	} else {
		return nullptr;
	}
}

spvgentwo::Instruction* SpirvGenTwo::Min(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b)
{
	if (!a || !b) {
		return nullptr;
	}
	spvgentwo::BasicBlock& bb = *func;
	if (a->getType()->isFloat()) {
		return bb.ext<spvgentwo::ext::GLSL>()->opFMin(a, b);
	} else if (a->getType()->isInt()) {
		return bb.ext<spvgentwo::ext::GLSL>()->opSMin(a, b);
	} else if (a->getType()->isUnsigned()) {
		return bb.ext<spvgentwo::ext::GLSL>()->opUMin(a, b);
	} else {
		return nullptr;
	}
}

spvgentwo::Instruction* SpirvGenTwo::Clamp(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* min, spvgentwo::Instruction* max)
{
	if (!x || !min || !max) {
		return nullptr;
	}

	spvgentwo::BasicBlock& bb = *func;
	if (x->getType()->isFloat()) {
		return bb.ext<spvgentwo::ext::GLSL>()->opFClamp(x, min, max);
	} else if (x->getType()->isInt()) {
		return bb.ext<spvgentwo::ext::GLSL>()->opSClamp(x, min, max);
	} else if (x->getType()->isUnsigned()) {
		return bb.ext<spvgentwo::ext::GLSL>()->opUClamp(x, min, max);
	} else {
		return nullptr;
	}
}

spvgentwo::Instruction* SpirvGenTwo::Mix(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* y, spvgentwo::Instruction* a)
{
	if (!x || !y || !a) {
		return nullptr;
	}
	spvgentwo::BasicBlock& bb = *func;
	return bb.ext<spvgentwo::ext::GLSL>()->opFMix(x, y, a);
}

void SpirvGenTwo::Store(spvgentwo::Function* func, spvgentwo::Instruction* dst, spvgentwo::Instruction* src)
{
	if (!dst || !src) {
		return;
	}
	(*func)->opStore(dst, src);
}

spvgentwo::Instruction* SpirvGenTwo::Load(spvgentwo::Function* func, spvgentwo::Instruction* var)
{
	if (!var) {
		return nullptr;
	}
	return (*func)->opLoad(var);
}

spvgentwo::Instruction* SpirvGenTwo::ImageSample(spvgentwo::Function* func, spvgentwo::Instruction* img, spvgentwo::Instruction* uv, spvgentwo::Instruction* lod)
{
	if (!img || !uv) {
		return nullptr;
	}

	if (lod) {
		return (*func)->opImageSampleExplicitLodLevel(img, uv, lod);
	} else {
		return (*func)->opImageSampleImplictLod(img, uv);
	}
}

spvgentwo::Instruction* SpirvGenTwo::VariableFloat(spvgentwo::Function* func)
{
	return func->variable<float>();
}

spvgentwo::Instruction* SpirvGenTwo::VariableFloat2(spvgentwo::Function* func)
{
	return func->variable<spvgentwo::glsl::vec2>();
}

spvgentwo::Instruction* SpirvGenTwo::VariableFloat3(spvgentwo::Function* func)
{
	return func->variable<spvgentwo::glsl::vec3>();
}

spvgentwo::Instruction* SpirvGenTwo::VariableFloat4(spvgentwo::Function* func)
{
	return func->variable<spvgentwo::glsl::vec4>();
}

spvgentwo::Instruction* SpirvGenTwo::AddVariable(spvgentwo::Function* func, const char* name, spvgentwo::Instruction* value)
{
	if (!value) {
		return nullptr;
	}

	spvgentwo::Instruction* ret = nullptr;

	if (IsVector(*value))
	{
		int num = GetVectorNum(*value);
		switch (num)
		{
		case 2:
			ret = func->variable<spvgentwo::glsl::vec2>(name);
			break;
		case 3:
			ret = func->variable<spvgentwo::glsl::vec3>(name);
			break;
		case 4:
			ret = func->variable<spvgentwo::glsl::vec4>(name);
			break;
		}
	}
	else
	{
		auto type = value->getType();
		if (type->isInt()) {
			ret = func->variable<int>(name);
		} else if (type->isFloat()) {
			ret = func->variable<float>(name);
		}
	}

	if (ret) {
		(*func)->opStore(ret, value);
	}

	return ret;
}

spvgentwo::Instruction* SpirvGenTwo::ConstBool(spvgentwo::Module* module, bool b)
{
	return module->constant(b);
}

spvgentwo::Instruction* SpirvGenTwo::ConstInt(spvgentwo::Module* module, int x)
{
	return module->constant(x);
}

spvgentwo::Instruction* SpirvGenTwo::ConstFloat(spvgentwo::Module* module, float x)
{
	return module->constant(x);
}

spvgentwo::Instruction* SpirvGenTwo::ConstFloat2(spvgentwo::Module* module, float x, float y)
{
	return module->constant(spvgentwo::make_vector(x, y));
}

spvgentwo::Instruction* SpirvGenTwo::ConstFloat3(spvgentwo::Module* module, float x, float y, float z)
{
	return module->constant(spvgentwo::make_vector(x, y, z));
}

spvgentwo::Instruction* SpirvGenTwo::ConstFloat4(spvgentwo::Module* module, float x, float y, float z, float w)
{
	return module->constant(spvgentwo::make_vector(x, y, z, w));
}

spvgentwo::Instruction* SpirvGenTwo::ConstMatrix2(spvgentwo::Module* module, const float m[4])
{
	float m2[2][2];
	memcpy(m2, m, sizeof(float) * 4);
	return module->constant(spvgentwo::make_matrix(m2));
}

spvgentwo::Instruction* SpirvGenTwo::ConstMatrix3(spvgentwo::Module* module, const float m[9])
{
	float m2[3][3];
	memcpy(m2, m, sizeof(float) * 9);
	return module->constant(spvgentwo::make_matrix(m2));
}

spvgentwo::Instruction* SpirvGenTwo::ConstMatrix4(spvgentwo::Module* module, const float m[16])
{
	float m2[4][4];
	memcpy(m2, m, sizeof(float) * 16);
	return module->constant(spvgentwo::make_matrix(m2));
}

// func

spvgentwo::Function* SpirvGenTwo::QueryFunction(spvgentwo::Module& lib, const std::string& name)
{
	for (auto& func : lib.getFunctions()) 
	{
		std::string f_name = func.getName();
		size_t dot = f_name.find_first_of('.');
		if (dot != std::string::npos) {
			f_name = f_name.substr(dot + 1);
		}
		f_name = f_name.substr(0, f_name.find_first_of('('));
		if (f_name == name) {
			return &func;
		}
	}
	return nullptr;
}

spvgentwo::Function* SpirvGenTwo::CreateDeclFunc(spvgentwo::Module* module, spvgentwo::Function* func)
{
	spvgentwo::Function& dst = module->addFunction();

	dst.setReturnType(module->addType(func->getReturnType()));
	for (auto& p : func->getParameters()) {
		dst.addParameters(module->addType(*p.getType()));
	}

	dst.finalize(func->getFunctionControl(), func->getName());

	return &dst;
}

spvgentwo::Function* SpirvGenTwo::CreateFunc(spvgentwo::Module* module, const std::string& name, 
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

spvgentwo::Instruction* SpirvGenTwo::GetFuncParam(spvgentwo::Function* func, int index)
{
	return func->getParameter(index);
}

void SpirvGenTwo::GetFuncParamNames(spvgentwo::Function* func, std::vector<std::string>& names)
{
	for (auto& param : func->getParameters()) {
		names.push_back(param.getName());
	}
}

spvgentwo::Instruction* SpirvGenTwo::FuncCall(spvgentwo::Function* caller, spvgentwo::Function* callee, const std::vector<spvgentwo::Instruction*>& params)
{
	for (auto& p : params) {
		if (!p) {
			return nullptr;
		}
	}

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

void SpirvGenTwo::Return(spvgentwo::Function* func)
{
	(*func)->opReturn();
}

void SpirvGenTwo::ReturnValue(spvgentwo::Function* func, spvgentwo::Instruction* inst)
{
	(*func)->opReturnValue(inst);
}

// tools

void SpirvGenTwo::Print(const spvgentwo::Module& module, bool output_ir)
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

}