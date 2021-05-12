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
	ShaderBuilder();
	~ShaderBuilder();

	spvgentwo::Module* GetMainModule() const { return m_main.get(); }
	spvgentwo::Function* GetMainFunc() const { return m_main_func; }

	spvgentwo::Instruction* AddInput(const std::string& name, const std::string& type);
	spvgentwo::Instruction* AddOutput(const std::string& name, const std::string& type);
	spvgentwo::Instruction* AddUniform(spvgentwo::Module* module, const std::string& name, const std::string& type);
	int GetUniformNum() const { return m_unif_num; }

	std::shared_ptr<spvgentwo::Module> AddModule(ShaderStage stage, const std::string& glsl, const std::string& name);

	void ReplaceFunc(spvgentwo::Function* from, spvgentwo::Function* to);

	void AddLinkDecl(spvgentwo::Function* func, const std::string& name, bool is_export);

	void ImportAll();
	void FinishMain();
	std::vector<uint32_t> Link();
	std::string ConnectCSMain(const std::string& glsl);

private:
	void InitMain();

	void ResetState();

private:
	std::unique_ptr<spvgentwo::ConsoleLogger> m_logger;
	std::unique_ptr<spvgentwo::HeapAllocator> m_alloc;
	std::unique_ptr<spvgentwo::Grammar> m_gram;

	std::vector<std::pair<std::string, std::shared_ptr<spvgentwo::Module>>> m_modules;
	std::unique_ptr<spvgentwo::Module> m_main = nullptr;
	spvgentwo::Function* m_main_func = nullptr;

	// state
	int m_unif_num = 0;
	std::set<std::string> m_added_export_link_decl;

	// cache
	std::map<std::string, spvgentwo::Instruction*> m_input_cache;

}; // ShaderBuilder

}