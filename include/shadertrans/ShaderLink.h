#pragma once

#include "shadertrans/ShaderStage.h"

#include <vector>
#include <string>
#include <memory>

namespace spvgentwo 
{ 
	class ConsoleLogger; 
	class HeapAllocator; 
	class Grammar; 
	class Module; 
	class Function; 
	class Instruction;
	class EntryPoint;
}

namespace shadertrans
{

class ShaderLink
{
public:
	ShaderLink();
	~ShaderLink();

	spvgentwo::Module* GetMainModule() const { return m_main.get(); }
	spvgentwo::Function* GetMainFunc() const { return m_main_func; }

	spvgentwo::Instruction* AddInput(const std::string& name, const std::string& type);
	spvgentwo::Instruction* AddOutput(const std::string& name, const std::string& type);
	spvgentwo::Instruction* AddUniform(const std::string& name, const std::string& type);

	static spvgentwo::Instruction* AccessChain(spvgentwo::Function* func, spvgentwo::Instruction* base, unsigned int index);
	static spvgentwo::Instruction* ComposeFloat2(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* y);
	static spvgentwo::Instruction* ComposeFloat3(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* y, spvgentwo::Instruction* z);
	static spvgentwo::Instruction* ComposeFloat4(spvgentwo::Function* func, spvgentwo::Instruction* x, spvgentwo::Instruction* y, spvgentwo::Instruction* z, spvgentwo::Instruction* w);
	static spvgentwo::Instruction* ComposeExtract(spvgentwo::Function* func, spvgentwo::Instruction* comp, unsigned int index);
	static spvgentwo::Instruction* Dot(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Add(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Sub(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Mul(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static spvgentwo::Instruction* Div(spvgentwo::Function* func, spvgentwo::Instruction* a, spvgentwo::Instruction* b);
	static void Store(spvgentwo::Function* func, spvgentwo::Instruction* dst, spvgentwo::Instruction* src);

	std::shared_ptr<spvgentwo::Module> AddLibrary(ShaderStage stage, const std::string& glsl);
	static spvgentwo::Function* QueryFunction(spvgentwo::Module& lib, const std::string& name);

	void ReplaceFunc(spvgentwo::Function* from, spvgentwo::Function* to);

	static spvgentwo::Function* CreateDeclFunc(spvgentwo::Module* module, spvgentwo::Function* func);
	static void AddLinkDecl(spvgentwo::Function* func, const std::string& name, bool is_export);

	static spvgentwo::Function* CreateFunc(spvgentwo::Module* module, const std::string& name, 
		const std::string& ret, const std::vector<std::string>& args);
	static spvgentwo::Instruction* GetFuncParam(spvgentwo::Function* func, int index);
	static void GetFuncParamNames(spvgentwo::Function* func, std::vector<std::string>& names);
	static spvgentwo::Instruction* FuncCall(spvgentwo::Function* caller, spvgentwo::Function* callee, const std::vector<spvgentwo::Instruction*>& params);
	static void Return(spvgentwo::Function* func);
	static void ReturnValue(spvgentwo::Function* func, spvgentwo::Instruction* inst);

	static spvgentwo::Instruction* ConstFloat(spvgentwo::Module* module, float x);
	static spvgentwo::Instruction* ConstFloat2(spvgentwo::Module* module, float x, float y);
	static spvgentwo::Instruction* ConstFloat3(spvgentwo::Module* module, float x, float y, float z);
	static spvgentwo::Instruction* ConstFloat4(spvgentwo::Module* module, float x, float y, float z, float w);

	static spvgentwo::Instruction* ConstMatrix2(spvgentwo::Module* module, const float m[4]);
	static spvgentwo::Instruction* ConstMatrix3(spvgentwo::Module* module, const float m[9]);
	static spvgentwo::Instruction* ConstMatrix4(spvgentwo::Module* module, const float m[16]);

	void ImportAll();
	void FinishMain();
	std::string Link();

	static void Print(const spvgentwo::Module& module, bool output_ir = false);

private:
	void InitMain();

private:
	std::unique_ptr<spvgentwo::ConsoleLogger> m_logger;
	std::unique_ptr<spvgentwo::HeapAllocator> m_alloc;
	std::unique_ptr<spvgentwo::Grammar> m_gram;

	std::vector<std::shared_ptr<spvgentwo::Module>> m_libs;
	std::unique_ptr<spvgentwo::Module> m_main = nullptr;
	spvgentwo::Function* m_main_func = nullptr;

}; // ShaderLink

}