#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace shadertrans
{
namespace spirv
{

enum class ValueType
{
	Void,
	Bool,
	Int,
	Float,
	Vector,
	Matrix,
	Struct,
	Unknown
};

struct Variable 
{
	std::string name;
	ValueType type = ValueType::Unknown, base_type = ValueType::Unknown;
	int type_comp_count = 1;
	std::string type_name;
};

struct Function
{
	int line_start;
	int line_end;
	std::vector<Variable> arguments;
	std::vector<Variable> locals;

	Variable ret_type;
};

struct Module
{
	std::unordered_map<std::string, Function> functions;
	std::unordered_map<std::string, std::vector<Variable>> user_types;
	std::vector<Variable> uniforms;
	std::vector<Variable> globals;

	bool barrier_used;
	int local_size_x, local_size_y, local_size_z;

	int arithmetic_inst_count;
	int bit_inst_count;
	int logical_inst_count;
	int texture_inst_count;
	int derivative_inst_count;
	int control_flow_inst_count;

	std::vector<unsigned int> spv;
};

}
}
