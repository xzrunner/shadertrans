#pragma once

#include "shadertrans/ShaderStage.h"

#include <vector>
#include <string>
#include <memory>
#include <set>
#include <map>

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

class ShaderBuilder
{
public:
	struct Module
	{
		std::string name;

		std::shared_ptr<spvgentwo::Module> impl = nullptr;

		std::vector<std::shared_ptr<Module>> includes;
	};

public:
	ShaderBuilder();
	~ShaderBuilder();

	spvgentwo::Module* GetMainModule() const { return m_main.get(); }
	spvgentwo::Function* GetMainFunc() const { return m_main_func; }

	spvgentwo::Instruction* AddInput(const std::string& name, const std::string& type);
	spvgentwo::Instruction* AddOutput(const std::string& name, const std::string& type);
	spvgentwo::Instruction* AddUniform(spvgentwo::Module* module, const std::string& name, const std::string& type);
	const char* QueryUniformName(const spvgentwo::Instruction* unif) const;

	std::shared_ptr<Module> AddModule(ShaderStage stage, const std::string& code, const std::string& lang, const std::string& name, const std::string& entry_point);

	void ReplaceFunc(spvgentwo::Function* from, spvgentwo::Function* to);

	void AddLinkDecl(spvgentwo::Function* func, const std::string& name, bool is_export);

	void ImportAll();
	void FinishMain();
	std::vector<uint32_t> Link();
	std::string ConnectCSMain(const std::string& glsl);

private:
	void InitMain();

	void ResetState();

	std::string GetAvaliableUnifName(const std::string& name) const;

	std::shared_ptr<Module> FindModule(const std::string& name) const;

private:
	std::unique_ptr<spvgentwo::ConsoleLogger> m_logger;
	std::unique_ptr<spvgentwo::HeapAllocator> m_alloc;
	std::unique_ptr<spvgentwo::Grammar> m_gram;

	std::vector<std::shared_ptr<Module>> m_modules;
	std::unique_ptr<spvgentwo::Module> m_main = nullptr;
	spvgentwo::Function* m_main_func = nullptr;

	// state

	std::set<std::string> m_added_export_link_decl;

	std::map<std::string, spvgentwo::Instruction*> m_input_cache;
	std::map<std::string, spvgentwo::Instruction*> m_output_cache;
	std::map<std::string, spvgentwo::Instruction*> m_uniform_cache;

}; // ShaderBuilder

}