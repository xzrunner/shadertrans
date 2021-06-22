#pragma once

#include <vector>
#include <string>

namespace spvgentwo { 
	class Module;
	class Function;
	class BasicBlock;
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

	// block
	static spvgentwo::BasicBlock* GetFuncBlock(spvgentwo::Function* func);

	// inst

	static spvgentwo::BasicBlock* If(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* cond,
		spvgentwo::BasicBlock* bb_true, spvgentwo::BasicBlock* bb_false);

	static spvgentwo::Instruction* VariableFloat(spvgentwo::Function* func, const char* name = nullptr);
	static spvgentwo::Instruction* VariableFloat2(spvgentwo::Function* func, const char* name = nullptr);
	static spvgentwo::Instruction* VariableFloat3(spvgentwo::Function* func, const char* name = nullptr);
	static spvgentwo::Instruction* VariableFloat4(spvgentwo::Function* func, const char* name = nullptr);

	static spvgentwo::BasicBlock* AddBlock(spvgentwo::Function* func, const char* name);

	static spvgentwo::Instruction* AddVariable(spvgentwo::Function* func, const char* name, spvgentwo::Instruction* value);

	static spvgentwo::Instruction* ConstBool(spvgentwo::Module* module, bool b);
	static spvgentwo::Instruction* ConstInt(spvgentwo::Module* module, int x);

	static spvgentwo::Instruction* ConstFloat(spvgentwo::Module* module, float x);
	static spvgentwo::Instruction* ConstFloat2(spvgentwo::Module* module, float x, float y);
	static spvgentwo::Instruction* ConstFloat3(spvgentwo::Module* module, float x, float y, float z);
	static spvgentwo::Instruction* ConstFloat4(spvgentwo::Module* module, float x, float y, float z, float w);

	static spvgentwo::Instruction* ConstMatrix2(spvgentwo::Module* module, const float m[4]);
	static spvgentwo::Instruction* ConstMatrix3(spvgentwo::Module* module, const float m[9]);
	static spvgentwo::Instruction* ConstMatrix4(spvgentwo::Module* module, const float m[16]);

	// bb

	static spvgentwo::Instruction* AccessChain(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* base, unsigned int index);
	static spvgentwo::Instruction* ComposeFloat2(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* x, spvgentwo::Instruction* y);
	static spvgentwo::Instruction* ComposeFloat3(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* x, spvgentwo::Instruction* y, spvgentwo::Instruction* z);
	static spvgentwo::Instruction* ComposeFloat4(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* x, spvgentwo::Instruction* y, spvgentwo::Instruction* z, spvgentwo::Instruction* w);
	static spvgentwo::Instruction* ComposeExtract(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* comp, unsigned int index);
	static spvgentwo::Instruction* Dot(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Cross(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Add(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Sub(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Mul(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Div(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Negate(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* v);
	static spvgentwo::Instruction* Reflect(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* I, spvgentwo::Instruction* N);
	static spvgentwo::Instruction* Sqrt(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* v);
	static spvgentwo::Instruction* Pow(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* x, spvgentwo::Instruction* y);
	static spvgentwo::Instruction* Normalize(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* v);
	static spvgentwo::Instruction* Length(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* v);
	static spvgentwo::Instruction* Max(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Min(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Clamp(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* x, spvgentwo::Instruction* min, spvgentwo::Instruction* max);
	static spvgentwo::Instruction* Mix(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* x, spvgentwo::Instruction* y, spvgentwo::Instruction* a);

	static spvgentwo::Instruction* IsEqual(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* IsNotEqual(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* IsGreater(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* IsGreaterEqual(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* IsLess(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* IsLessEqual(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* a, spvgentwo::Instruction* b);

	static void Kill(spvgentwo::BasicBlock* bb);

	static void Return(spvgentwo::BasicBlock* bb);
	static void ReturnValue(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* inst);

	static spvgentwo::Instruction* Load(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* var);
	static void Store(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* dst, spvgentwo::Instruction* src);

	static spvgentwo::Instruction* ImageSample(spvgentwo::BasicBlock* bb, spvgentwo::Instruction* img, spvgentwo::Instruction* uv, spvgentwo::Instruction* lod);

	// func

	static spvgentwo::Function* QueryFunction(spvgentwo::Module& lib, const std::string& name);

	static spvgentwo::Function* CreateDeclFunc(spvgentwo::Module* module, spvgentwo::Function* func);

	static spvgentwo::Function* CreateFunc(spvgentwo::Module* module, const std::string& name,
		const std::string& ret, const std::vector<std::string>& args);
	static spvgentwo::Instruction* GetFuncParam(spvgentwo::Function* func, int index);
	static void GetFuncParamNames(spvgentwo::Function* func, std::vector<std::string>& names);
	static spvgentwo::Instruction* FuncCall(spvgentwo::Function* caller, spvgentwo::Function* callee, const std::vector<spvgentwo::Instruction*>& params);

	// tools

	static void Print(spvgentwo::Module& module);

}; // SpirvGenTwo

}