#pragma once

#include "shadertrans/ShaderStage.h"

#include <vector>
#include <string>
#include <memory>

namespace spvgentwo { class ConsoleLogger; class HeapAllocator; class Grammar; class Module; class Function; }

namespace shadertrans
{

class ShaderLink
{
public:
	ShaderLink();
	~ShaderLink();

	void AddLibrary(ShaderStage stage, const std::string& glsl);

	void Link();

private:
	void InitConsumer();

	void Print(const spvgentwo::Module& module) const;

	spvgentwo::Function& CreateFuncSign(const spvgentwo::Function& src) const;

private:
	std::unique_ptr<spvgentwo::ConsoleLogger> m_logger;
	std::unique_ptr<spvgentwo::HeapAllocator> m_alloc;
	std::unique_ptr<spvgentwo::Grammar> m_gram;

	std::vector<std::shared_ptr<spvgentwo::Module>> m_libs;
	std::unique_ptr<spvgentwo::Module> m_consumer = nullptr;

}; // ShaderLink

}