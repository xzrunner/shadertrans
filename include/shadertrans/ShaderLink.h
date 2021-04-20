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

	spvgentwo::Instruction* AddInput(const std::string& name, const std::string& type);
	spvgentwo::Instruction* AddOutput(const std::string& name, const std::string& type);
	spvgentwo::Instruction* AddUniform(const std::string& name, const std::string& type);

	spvgentwo::Instruction* ConstFloat(float x);
	spvgentwo::Instruction* ConstFloat2(float x, float y);
	spvgentwo::Instruction* Call(spvgentwo::Function* func, const std::vector<spvgentwo::Instruction*>& args);
	spvgentwo::Instruction* AccessChain(spvgentwo::Instruction* base, unsigned int index);
	void Store(spvgentwo::Instruction* dst, spvgentwo::Instruction* src);
	void Return();

	std::shared_ptr<spvgentwo::Module> AddLibrary(ShaderStage stage, const std::string& glsl);
	spvgentwo::Function* GetEntryFunc(spvgentwo::Module& lib);

	spvgentwo::Function* CreateDeclFunc(spvgentwo::Function* func) const;
	void AddLinkDecl(spvgentwo::Function* func, const std::string& name, bool is_export) const;

	void ImportAll();
	void FinishMain();
	std::string Link();

private:
	void InitMain();

	void Print(const spvgentwo::Module& module) const;

private:
	std::unique_ptr<spvgentwo::ConsoleLogger> m_logger;
	std::unique_ptr<spvgentwo::HeapAllocator> m_alloc;
	std::unique_ptr<spvgentwo::Grammar> m_gram;

	std::vector<std::shared_ptr<spvgentwo::Module>> m_libs;
	std::unique_ptr<spvgentwo::Module> m_main = nullptr;
	spvgentwo::EntryPoint* m_main_entry = nullptr;

}; // ShaderLink

}