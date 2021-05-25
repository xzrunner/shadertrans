#pragma once

#include <vector>
#include <string>

namespace spvgentwo { 
	class Module;
	class Function;
	class Instruction; 
}

namespace shadertrans
{

class SpirvGenTwo
{
public:
	// info

	static const char* GetType(const spvgentwo::Instruction& inst);
	static bool IsVector(const spvgentwo::Instruction& inst);
	static int GetVectorNum(const spvgentwo::Instruction& inst);

	// inst

	static spvgentwo::Instruction* AccessChain(spvgentwo::Function* func, spvgentwo::Instruction* base, unsigned int index);
	static spvgentwo::Instruction* ComposeFloat2(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* y);
	static spvgentwo::Instruction* ComposeFloat3(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* y, spvgentwo::Instruction* z);
	static spvgentwo::Instruction* ComposeFloat4(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* y, spvgentwo::Instruction* z, spvgentwo::Instruction* w);
	static spvgentwo::Instruction* ComposeExtract(spvgentwo::Function* func, spvgentwo::Instruction* comp, unsigned int index);
	static spvgentwo::Instruction* Dot(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Cross(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Add(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Sub(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Mul(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Div(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Negate(spvgentwo::Function* func, spvgentwo::Instruction* v);
	static spvgentwo::Instruction* Sqrt(spvgentwo::Function* func, spvgentwo::Instruction* v);
	static spvgentwo::Instruction* Pow(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* y);
	static spvgentwo::Instruction* Normalize(spvgentwo::Function* func, spvgentwo::Instruction* v);
	static spvgentwo::Instruction* Max(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Min(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Clamp(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* min, spvgentwo::Instruction* max);
	static spvgentwo::Instruction* Mix(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* y, spvgentwo::Instruction* a);
	static void Store(spvgentwo::Function* func, spvgentwo::Instruction* dst, spvgentwo::Instruction* src);
	static spvgentwo::Instruction* Load(spvgentwo::Function* func, spvgentwo::Instruction* var);
	static spvgentwo::Instruction* ImageSample(spvgentwo::Function* func, spvgentwo::Instruction* img, spvgentwo::Instruction* uv);

	static spvgentwo::Instruction* VariableFloat(spvgentwo::Function* func);
	static spvgentwo::Instruction* VariableFloat2(spvgentwo::Function* func);
	static spvgentwo::Instruction* VariableFloat3(spvgentwo::Function* func);
	static spvgentwo::Instruction* VariableFloat4(spvgentwo::Function* func);

	static spvgentwo::Instruction* AddVariable(spvgentwo::Function* func, const char* name, spvgentwo::Instruction* value);

	static spvgentwo::Instruction* ConstFloat(spvgentwo::Module* module, float x);
	static spvgentwo::Instruction* ConstFloat2(spvgentwo::Module* module, float x, float y);
	static spvgentwo::Instruction* ConstFloat3(spvgentwo::Module* module, float x, float y, float z);
	static spvgentwo::Instruction* ConstFloat4(spvgentwo::Module* module, float x, float y, float z, float w);

	static spvgentwo::Instruction* ConstMatrix2(spvgentwo::Module* module, const float m[4]);
	static spvgentwo::Instruction* ConstMatrix3(spvgentwo::Module* module, const float m[9]);
	static spvgentwo::Instruction* ConstMatrix4(spvgentwo::Module* module, const float m[16]);

	// func

	static spvgentwo::Function* QueryFunction(spvgentwo::Module& lib, const std::string& name);

	static spvgentwo::Function* CreateDeclFunc(spvgentwo::Module* module, spvgentwo::Function* func);

	static spvgentwo::Function* CreateFunc(spvgentwo::Module* module, const std::string& name,
		const std::string& ret, const std::vector<std::string>& args);
	static spvgentwo::Instruction* GetFuncParam(spvgentwo::Function* func, int index);
	static void GetFuncParamNames(spvgentwo::Function* func, std::vector<std::string>& names);
	static spvgentwo::Instruction* FuncCall(spvgentwo::Function* caller, spvgentwo::Function* callee, const std::vector<spvgentwo::Instruction*>& params);
	static void Return(spvgentwo::Function* func);
	static void ReturnValue(spvgentwo::Function* func, spvgentwo::Instruction* inst);

	// tools

	static void Print(const spvgentwo::Module& module, bool output_ir = false);

}; // SpirvGenTwo

}