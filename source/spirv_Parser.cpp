#include "shadertrans/spirv_Parser.h"

#include <spirv/unified1/spirv.hpp>
#include <unordered_map>
#include <functional>

typedef unsigned int spv_word;

namespace shadertrans 
{
namespace spirv
{

std::string spvReadString(const unsigned int* data, int length, int& i)
{
	std::string ret(length * 4, 0);

	for (int j = 0; j < length; j++, i++) {
		ret[j*4+0] = (data[i] & 0x000000FF);
		ret[j*4+1] = (data[i] & 0x0000FF00) >> 8;
		ret[j*4+2] = (data[i] & 0x00FF0000) >> 16;
		ret[j*4+3] = (data[i] & 0xFF000000) >> 24;
	}

	for (size_t j = 0; j < ret.size(); j++) {
		if (ret[j] == 0) {
			ret.resize(j);
			break;
		}
	}

	return ret;
}

void Parser::Parse(const std::vector<unsigned int>& ir, Module& module)
{
	module.spv = ir;

	module.functions.clear();
	module.user_types.clear();
	module.uniforms.clear();
	module.globals.clear();

	module.arithmetic_inst_count = 0;
	module.bit_inst_count = 0;
	module.logical_inst_count = 0;
	module.texture_inst_count = 0;
	module.derivative_inst_count = 0;
	module.control_flow_inst_count = 0;

	module.barrier_used = false;
	module.local_size_x = 1;
	module.local_size_y = 1;
	module.local_size_z = 1;

	std::string curFunc = "";
	int lastOpLine = -1;

	std::unordered_map<spv_word, std::string> names;
	std::unordered_map<spv_word, spv_word> pointers;
	std::unordered_map<spv_word, std::pair<ValueType, int>> types;

	std::function<void(Variable&, spv_word)> fetchType = [&](Variable& var, spv_word type) {
		spv_word actualType = type;
		if (pointers.count(type))
			actualType = pointers[type];

		const std::pair<ValueType, int>& info = types[actualType];
		var.type = info.first;
		var.base_type = var.type;
			
		if (var.type == ValueType::Struct)
		{
			var.type_name = names[info.second];
		}
		else if (var.type == ValueType::Vector) 
		{
			var.type_comp_count = info.second & 0x00ffffff;
			var.base_type = (ValueType)((info.second & 0xff000000) >> 24);
		} 
		else if (var.type == ValueType::Matrix)
		{
			var.type_comp_count = info.second & 0x00ffffff;
			var.base_type = ValueType::Matrix;
		}
	};

	for (int i = 5; i < ir.size();) {
		int iStart = i;
		spv_word opcodeData = ir[i];

		spv_word wordCount = ((opcodeData & (~spv::OpCodeMask)) >> spv::WordCountShift) - 1;
		spv_word opcode = (opcodeData & spv::OpCodeMask);

		switch (opcode) {
		case spv::OpName: {
			spv_word loc = ir[++i];
			spv_word stringLength = wordCount - 1;

			names[loc] = spvReadString(ir.data(), stringLength, ++i);
		} break;
		case spv::OpLine: {
			++i; // skip file
			lastOpLine = ir[++i];

			if (!curFunc.empty() && module.functions[curFunc].line_start == -1)
				module.functions[curFunc].line_start = lastOpLine;
		} break;
		case spv::OpTypeStruct: {
			spv_word loc = ir[++i];

			spv_word memCount = wordCount - 1;
			if (module.user_types.count(names[loc]) == 0) {
				std::vector<Variable> mems(memCount);
				for (spv_word j = 0; j < memCount; j++) {
					spv_word type = ir[++i];
					fetchType(mems[j], type);
				}

				module.user_types.insert(std::make_pair(names[loc], mems));
			} else {
				auto& typeInfo = module.user_types[names[loc]];
				for (spv_word j = 0; j < memCount && j < typeInfo.size(); j++) {
					spv_word type = ir[++i];
					fetchType(typeInfo[j], type);
				}
			}

			types[loc] = std::make_pair(ValueType::Struct, loc);
		} break;
		case spv::OpMemberName: {
			spv_word owner = ir[++i];
			spv_word index = ir[++i]; // index

			spv_word stringLength = wordCount - 2;

			auto& typeInfo = module.user_types[names[owner]];

			if (index < typeInfo.size())
				typeInfo[index].name = spvReadString(ir.data(), stringLength, ++i);
			else {
				typeInfo.resize(index + 1);
				typeInfo[index].name = spvReadString(ir.data(), stringLength, ++i);
			}
		} break;
		case spv::OpFunction: {
			spv_word type = ir[++i];
			spv_word loc = ir[++i];

			curFunc = names[loc];
			size_t dot = curFunc.find_first_of('.');
			if (dot != std::string::npos) {
				curFunc = curFunc.substr(dot + 1);
			}
			size_t args = curFunc.find_first_of('(');
			if (args != std::string::npos) {
				curFunc = curFunc.substr(0, args);
			}

			fetchType(module.functions[curFunc].ret_type, type);
			module.functions[curFunc].line_start = -1;
			module.functions[curFunc].index = module.functions.size() - 1;
		} break;
		case spv::OpFunctionEnd: {
			module.functions[curFunc].line_end = lastOpLine;
			lastOpLine = -1;
			curFunc = "";
		} break;
		case spv::OpVariable: {
			spv_word type = ir[++i];
			spv_word loc = ir[++i];

			std::string varName = names[loc];

			if (curFunc.empty()) {
				spv::StorageClass sType = (spv::StorageClass)ir[++i];
				if (sType == spv::StorageClassUniform || sType == spv::StorageClassUniformConstant) {
					Variable uni;
					uni.name = varName;
					fetchType(uni, type);

					if (uni.name.size() == 0 || uni.name[0] == 0) {
						if (module.user_types.count(uni.type_name) > 0) {
							const std::vector<Variable>& mems = module.user_types[uni.type_name];
							for (const auto& mem : mems)
								module.uniforms.push_back(mem);
						}
					} else
						module.uniforms.push_back(uni);
				} else if (varName.size() > 0 && varName[0] != 0) {
					Variable glob;
					glob.name = varName;
					fetchType(glob, type);

					module.globals.push_back(glob);
				}
			} else {
				Variable loc;
				loc.name = varName;
				fetchType(loc, type);
				module.functions[curFunc].locals.push_back(loc);
			}
		} break;
		case spv::OpFunctionParameter: {
			spv_word type = ir[++i];
			spv_word loc = ir[++i];

			Variable arg;
			arg.name = names[loc];
			fetchType(arg, type);
			module.functions[curFunc].arguments.push_back(arg);
		} break;
		case spv::OpTypePointer: {
			spv_word loc = ir[++i];
			++i; // skip storage class
			spv_word type = ir[++i];

			pointers[loc] = type;
		} break;
		case spv::OpTypeBool: {
			spv_word loc = ir[++i];
			types[loc] = std::make_pair(ValueType::Bool, 0);
		} break;
		case spv::OpTypeInt: {
			spv_word loc = ir[++i];
			types[loc] = std::make_pair(ValueType::Int, 0);
		} break;
		case spv::OpTypeFloat: {
			spv_word loc = ir[++i];
			types[loc] = std::make_pair(ValueType::Float, 0);
		} break;
		case spv::OpTypeVector: {
			spv_word loc = ir[++i];
			spv_word comp = ir[++i];
			spv_word compcount = ir[++i];

			spv_word val = (compcount & 0x00FFFFFF) | (((spv_word)types[comp].first) << 24);

			types[loc] = std::make_pair(ValueType::Vector, val);
		} break;
		case spv::OpTypeMatrix: {
			spv_word loc = ir[++i];
			spv_word comp = ir[++i];
			spv_word compcount = ir[++i];

			spv_word val = (compcount & 0x00FFFFFF) | (types[comp].second & 0xFF000000);

			types[loc] = std::make_pair(ValueType::Matrix, val);
		} break;
		case spv::OpExecutionMode: {
			++i; // skip
			spv_word execMode = ir[++i];

			if (execMode == spv::ExecutionMode::ExecutionModeLocalSize) {
				module.local_size_x = ir[++i];
				module.local_size_y = ir[++i];
				module.local_size_z = ir[++i];
			}
		} break;

		case spv::OpControlBarrier:
		case spv::OpMemoryBarrier:
		case spv::OpNamedBarrierInitialize: {
			module.barrier_used = true;
		} break;

		case spv::OpSNegate: case spv::OpFNegate:
		case spv::OpIAdd: case spv::OpFAdd:
		case spv::OpISub: case spv::OpFSub:
		case spv::OpIMul: case spv::OpFMul:
		case spv::OpUDiv: case spv::OpSDiv:
		case spv::OpFDiv: case spv::OpUMod:
		case spv::OpSRem: case spv::OpSMod:
		case spv::OpFRem: case spv::OpFMod:
		case spv::OpVectorTimesScalar:
		case spv::OpMatrixTimesScalar:
		case spv::OpVectorTimesMatrix:
		case spv::OpMatrixTimesVector:
		case spv::OpMatrixTimesMatrix:
		case spv::OpOuterProduct:
		case spv::OpDot:
		case spv::OpIAddCarry:
		case spv::OpISubBorrow:
		case spv::OpUMulExtended:
		case spv::OpSMulExtended:
			module.arithmetic_inst_count++;
			break;

				
		case spv::OpShiftRightLogical:
		case spv::OpShiftRightArithmetic:
		case spv::OpShiftLeftLogical:
		case spv::OpBitwiseOr:
		case spv::OpBitwiseXor:
		case spv::OpBitwiseAnd:
		case spv::OpNot:
		case spv::OpBitFieldInsert:
		case spv::OpBitFieldSExtract:
		case spv::OpBitFieldUExtract:
		case spv::OpBitReverse:
		case spv::OpBitCount:
			module.bit_inst_count++;
			break;

		case spv::OpAny: case spv::OpAll:
		case spv::OpIsNan: case spv::OpIsInf:
		case spv::OpIsFinite: case spv::OpIsNormal:
		case spv::OpSignBitSet: case spv::OpLessOrGreater:
		case spv::OpOrdered: case spv::OpUnordered:
		case spv::OpLogicalEqual: case spv::OpLogicalNotEqual:
		case spv::OpLogicalOr: case spv::OpLogicalAnd:
		case spv::OpLogicalNot: case spv::OpSelect:
		case spv::OpIEqual: case spv::OpINotEqual:
		case spv::OpUGreaterThan: case spv::OpSGreaterThan:
		case spv::OpUGreaterThanEqual: case spv::OpSGreaterThanEqual:
		case spv::OpULessThan: case spv::OpSLessThan:
		case spv::OpULessThanEqual: case spv::OpSLessThanEqual:
		case spv::OpFOrdEqual: case spv::OpFUnordEqual:
		case spv::OpFOrdNotEqual: case spv::OpFUnordNotEqual:
		case spv::OpFOrdLessThan: case spv::OpFUnordLessThan:
		case spv::OpFOrdGreaterThan: case spv::OpFUnordGreaterThan:
		case spv::OpFOrdLessThanEqual: case spv::OpFUnordLessThanEqual:
		case spv::OpFOrdGreaterThanEqual: case spv::OpFUnordGreaterThanEqual:
			module.logical_inst_count++;
			break;

		case spv::OpImageSampleImplicitLod:
		case spv::OpImageSampleExplicitLod:
		case spv::OpImageSampleDrefImplicitLod:
		case spv::OpImageSampleDrefExplicitLod:
		case spv::OpImageSampleProjImplicitLod:
		case spv::OpImageSampleProjExplicitLod:
		case spv::OpImageSampleProjDrefImplicitLod:
		case spv::OpImageSampleProjDrefExplicitLod:
		case spv::OpImageFetch: case spv::OpImageGather:
		case spv::OpImageDrefGather: case spv::OpImageRead:
		case spv::OpImageWrite:
			module.texture_inst_count++;
			break;

		case spv::OpDPdx:
		case spv::OpDPdy:
		case spv::OpFwidth:
		case spv::OpDPdxFine:
		case spv::OpDPdyFine:
		case spv::OpFwidthFine:
		case spv::OpDPdxCoarse:
		case spv::OpDPdyCoarse:
		case spv::OpFwidthCoarse:
			module.derivative_inst_count++;
			break;

		case spv::OpPhi:
		case spv::OpLoopMerge:
		case spv::OpSelectionMerge:
		case spv::OpLabel:
		case spv::OpBranch:
		case spv::OpBranchConditional:
		case spv::OpSwitch:
		case spv::OpKill:
		case spv::OpReturn:
		case spv::OpReturnValue:
			module.control_flow_inst_count++;
			break;
		}

		i = iStart + wordCount + 1;
	}
}

}
}