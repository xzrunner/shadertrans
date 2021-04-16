#include "shadertrans/ShaderReflection.h"
#include "shadertrans/ShaderTrans.h"

#include <shaderlink/SPIRVParser.h>

#include <spirv.hpp>
#include <spirv_glsl.hpp>
#include <spirv_reflect.h>
#include <spvgentwo/Module.h>
#include <spvgentwo/Grammar.h>
#include <spvgentwo/Reader.h>
#include <spvgentwo/Templates.h>
#include <spvgentwo/TypeAlias.h>
#include <common/HeapAllocator.h>
#include <common/BinaryVectorWriter.h>
#include <common/BinaryFileReader.h>

#include <fstream>

using namespace spvgentwo;

namespace
{

using Variable = shadertrans::ShaderReflection::Variable;
using VarType = shadertrans::ShaderReflection::VarType;

VarType parse_spir_type(const spirv_cross::SPIRType& type)
{
    VarType ret = VarType::Unknown;

	switch (type.basetype)
	{
	case spirv_cross::SPIRType::BaseType::Boolean:
		if (type.columns == 1 && type.vecsize == 1) {
            ret = VarType::Bool;
		}
		break;

	case spirv_cross::SPIRType::BaseType::Int:
		switch (type.vecsize) 
		{
		case 1: 
            ret = VarType::Int;
			break;
		case 2: 
			ret = VarType::Int2;
			break;
		case 3: 
			ret = VarType::Int3;
			break;
		case 4: 
			ret = VarType::Int4;
			break;
		}
		break;

    case spirv_cross::SPIRType::BaseType::UInt:
        switch (type.vecsize)
        {
        case 1:
            ret = VarType::Int;
            break;
        case 2:
            ret = VarType::Int2;
            break;
        case 3:
            ret = VarType::Int3;
            break;
        case 4:
            ret = VarType::Int4;
            break;
        }
        break;

	case spirv_cross::SPIRType::BaseType::Float:
		switch (type.columns) 
		{
		case 1:
			switch (type.vecsize) 
			{
			case 1: 
				ret = VarType::Float;
				break;
			case 2: 
				ret = VarType::Float2;
				break;
			case 3: 
				ret = VarType::Float3;
				break;
			case 4: 
				ret = VarType::Float4;
				break;
			}
			break;

		case 2:
			if (type.vecsize == 2) {
				ret = VarType::Mat2;
			}
			break;
				
		case 3:
			if (type.vecsize == 3) {
				ret = VarType::Mat3;
			}
			break;

		case 4:
			if (type.vecsize == 4) {
				ret = VarType::Mat4;
			}
			break;
		}
		break;
	}
	return ret;
}

void get_struct_uniforms(const spirv_cross::CompilerGLSL& compiler, 
                         spirv_cross::TypeID base_type_id,
                         const spirv_cross::SPIRType& type,
                         std::vector<Variable>& uniforms,
                         const std::string& base_name)
{   
    auto member_count = type.member_types.size();
    if (!type.array.empty()) 
    {
        Variable v_array;
        v_array.name = base_name;
        v_array.type = VarType::Array;

        for (int i = 0, n = type.array[0]; i < n; ++i)
        {
            Variable v_struct;
            v_struct.name = base_name;
            v_struct.type = VarType::Struct;

            for (int j = 0; j < member_count; j++)
            {
                std::string full_name = base_name + "[" + std::to_string(i) + "]";
                auto name = compiler.get_member_name(type.parent_type, j);
                full_name.append("." + name);
                auto sub_type = compiler.get_type(type.member_types[j]);
                if (sub_type.basetype == spirv_cross::SPIRType::Struct)
                {
                    get_struct_uniforms(compiler, type.member_types[j], sub_type, v_struct.children, full_name);
                }
                else
                {
                    Variable unif;

                    unif.name = full_name;
                    unif.type = parse_spir_type(sub_type);

                    //size_t size = compiler.get_declared_struct_member_size(type, j);
                    //size_t offset = compiler.type_struct_member_offset(type, j);

                    v_struct.children.push_back(unif);
                }
            }

            v_array.children.push_back(v_struct);
        }

        uniforms.push_back(v_array);
    }
    else
    {
        Variable v_struct;
        v_struct.name = base_name;
        v_struct.type = VarType::Struct;

        for (int i = 0; i < member_count; i++)
        {
            auto name = compiler.get_member_name(base_type_id, i);
            if (!base_name.empty()) {
                name.insert(0, base_name + ".");
            }
            auto sub_type = compiler.get_type(type.member_types[i]);
            if (sub_type.basetype == spirv_cross::SPIRType::Struct)
            {
                get_struct_uniforms(compiler, type.member_types[i], sub_type, v_struct.children, name);
            }
            else if (!sub_type.array.empty())
            {
                Variable v_array;
                v_array.name = name;
                v_array.type = VarType::Array;

                for (int i = 0, n = sub_type.array[0]; i < n; ++i)
                {
                    std::string full_name = name + "[" + std::to_string(i) + "]";

                    Variable unif;

                    unif.name = full_name;
                    unif.type = parse_spir_type(sub_type);

                    v_array.children.push_back(unif);
                }

                v_struct.children.push_back(v_array);
            }
            else
            {
                Variable unif;

                unif.name = name;
                unif.type = parse_spir_type(sub_type);

                v_struct.children.push_back(unif);
            }
        }

        if (v_struct.name.empty()) {
            std::copy(v_struct.children.begin(), v_struct.children.end(), std::back_inserter(uniforms));
        } else {
            uniforms.push_back(v_struct);
        }
    }
}

template <typename U32Vector>
class BinaryVectorReader : public spvgentwo::IReader 
{
public:
	BinaryVectorReader(U32Vector& spv) : m_vec(spv), m_index(0) { }
	~BinaryVectorReader() { }

	bool get(unsigned int& _word) final
	{
		if (m_index >= m_vec.size())
			return false;
		_word = m_vec[m_index++];
		return true;
	}

private:
	U32Vector& m_vec;
	int m_index;
}; // BinaryVectorReader

Variable parser_spirv_variable(const shaderlink::SPIRVParser::Variable& var,
                               const shaderlink::SPIRVParser& parser)
{
    Variable ret;
    ret.name = var.Name;
    ret.type = VarType::Unknown;
    auto type = var.BaseType;
    if (type == shaderlink::SPIRVParser::ValueType::Void) {
        type = var.Type;
    }
    switch (type)
    {
    case shaderlink::SPIRVParser::ValueType::Void:
        ret.type = VarType::Void;
        break;
    case shaderlink::SPIRVParser::ValueType::Bool:
        ret.type = VarType::Bool;
        assert(var.TypeComponentCount == 1);
        break;
    case shaderlink::SPIRVParser::ValueType::Int:
        switch (var.TypeComponentCount)
        {
        case 1:
            ret.type = VarType::Int;
            break;
        case 2:
            ret.type = VarType::Int2;
            break;
        case 3:
            ret.type = VarType::Int3;
            break;
        case 4:
            ret.type = VarType::Int4;
            break;
        }
        break;
    case shaderlink::SPIRVParser::ValueType::Float:
        switch (var.TypeComponentCount)
        {
        case 1:
            ret.type = VarType::Float;
            break;
        case 2:
            ret.type = VarType::Float2;
            break;
        case 3:
            ret.type = VarType::Float3;
            break;
        case 4:
            ret.type = VarType::Float4;
            break;
        }
        break;
    case shaderlink::SPIRVParser::ValueType::Vector:
        ret.type = VarType::Array;
        break;
    case shaderlink::SPIRVParser::ValueType::Matrix:
        switch (var.TypeComponentCount)
        {
        case 2:
            ret.type = VarType::Mat2;
            break;
        case 3:
            ret.type = VarType::Mat3;
            break;
        case 4:
            ret.type = VarType::Mat4;
            break;
        }
        break;
    case shaderlink::SPIRVParser::ValueType::Struct:
        ret.type = VarType::Struct;
        {
            auto user = parser.UserTypes.find(var.TypeName);
            if (user != parser.UserTypes.end()) {
                for (auto& v : user->second) {
                    ret.children.push_back(parser_spirv_variable(v, parser));
                }
            }
        }
        break;
    default:
        ret.type = VarType::Unknown;
    }

    return ret;
}

Variable parser_spvgentwo_variable(const spvgentwo::Instruction& inst)
{
    Variable ret;
    if (!inst.isType()) {
        ret.name = inst.getName();
    }
    ret.type = VarType::Unknown;

    spvgentwo::Type type = *inst.getType();
    if (type.isPointer()) {
        auto& sub_types = type.getSubTypes();
        assert(sub_types.size() == 1);
        type = spvgentwo::Type(sub_types.front());
    }

    if (type.isVoid()) {
        ret.type = VarType::Void;
    } 
    
    else if (type.isBool()) {
        ret.type = VarType::Bool;
    } else if (type.isInt()) {
        ret.type = VarType::Int;
    } else if (type.isFloat()) {
        ret.type = VarType::Float;
    } else if (type.isMatrix()) {
        switch (type.getVectorComponentCount())
        {
        case 2:
            ret.type = VarType::Mat2;
            break;
        case 3:
            ret.type = VarType::Mat3;
            break;
        case 4:
            ret.type = VarType::Mat4;
            break;
        }
    } else if (type.isVectorOfInt()) {
        switch (type.getVectorComponentCount())
        {
        case 2:
            ret.type = VarType::Int2;
            break;
        case 3:
            ret.type = VarType::Int3;
            break;
        case 4:
            ret.type = VarType::Int4;
            break;
        }
    } else if (type.isVectorOfFloat()) {
        switch (type.getVectorComponentCount())
        {
        case 2:
            ret.type = VarType::Float2;
            break;
        case 3:
            ret.type = VarType::Float3;
            break;
        case 4:
            ret.type = VarType::Float4;
            break;
        }
    } else if (type.isArray()) {
        ret.type = VarType::Array;
    } else if (type.isStruct()) {
        ret.type = VarType::Struct;
    }
    return ret;
}

}

namespace shadertrans
{

void ShaderReflection::GetUniforms(const std::vector<unsigned int>& spirv, 
	                               std::vector<Variable>& uniforms)
{
    spirv_cross::CompilerGLSL compiler(spirv);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	//// get_work_group_size_specialization_constants
	//auto entries = compiler.get_entry_points_and_stages();
	//for (auto& e : entries)
	//{
	//	if (e.execution_model == ::spv::ExecutionModelGLCompute)
	//	{
	//		const auto& spv_entry = compiler.get_entry_point(e.name, e.execution_model);
	//		m_props.insert({ "local_size_x", ShaderVariant(static_cast<int>(spv_entry.workgroup_size.x)) });
	//		m_props.insert({ "local_size_y", ShaderVariant(static_cast<int>(spv_entry.workgroup_size.y)) });
	//		m_props.insert({ "local_size_z", ShaderVariant(static_cast<int>(spv_entry.workgroup_size.z)) });
	//		break;
	//	}
	//}

	// uniforms
    for (auto& resource : resources.uniform_buffers)
    {
        auto ubo_name = compiler.get_name(resource.id);;
        spirv_cross::SPIRType type = compiler.get_type(resource.base_type_id);
        if (type.basetype == spirv_cross::SPIRType::Struct) {
            get_struct_uniforms(compiler, resource.base_type_id, type, uniforms, ubo_name);
        }

		uint32_t set = compiler.get_decoration(resource.id, ::spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(resource.id, ::spv::DecorationBinding);		
    }

	for (auto& resource : resources.sampled_images)
	{
		Variable unif;

		spirv_cross::SPIRType type = compiler.get_type(resource.base_type_id);

		unif.name = compiler.get_name(resource.id);
		uint32_t set = compiler.get_decoration(resource.id, ::spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(resource.id, ::spv::DecorationBinding);

		// todo
		unif.type = VarType::Sampler;

		uniforms.push_back(unif);
	}

	for (auto& resource : resources.storage_images)
	{
		//ImageUnit img;

		spirv_cross::SPIRType type = compiler.get_type(resource.base_type_id);

		//img.name = compiler.get_name(resource.id);
		//uint32_t set = compiler.get_decoration(resource.id, ::spv::DecorationDescriptorSet);
		//img.unit = compiler.get_decoration(resource.id, ::spv::DecorationBinding);
		//img.fmt = parse_image_format(type.image.format);
		//img.access = parse_image_access(type.image.access);

		//m_images.push_back(img);

		//Variable unif;
		//unif.name = img.name;
		//unif.type = VariableType::Sampler2D;
		//m_uniforms.push_back(unif);
        
        Variable unif;

        unif.name = compiler.get_name(resource.id);

        unif.type = VarType::Image;

        uniforms.push_back(unif);
	}
}

void ShaderReflection::GetFunction(const std::vector<unsigned int>& spirv,
                                   const std::string& name, Function& func)
{
    //spirv_cross::CompilerGLSL compiler(spirv);
    //spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    //////////////////////////////////////////////////////////////////////////

    //SpvReflectShaderModule module;
    //spvReflectCreateShaderModule(spirv.size() * sizeof(unsigned int), spirv.data(), &module);

    //////////////////////////////////////////////////////////////////////////

    shaderlink::SPIRVParser parser;
    parser.Parse(spirv);
    auto f = parser.Functions.find(name);
    if (f == parser.Functions.end()) {
        return;
    }

    for (auto& v : f->second.Arguments) {
        func.arguments.push_back(parser_spirv_variable(v, parser));
    }
    func.ret_type = parser_spirv_variable(f->second.ReturnType, parser);

    ////////////////////////////////////////////////////////////////////////////

    //spvgentwo::HeapAllocator alloc;
    //spvgentwo::Module module(&alloc, spvgentwo::spv::Version);
    //spvgentwo::Grammar gram(&alloc);

    //BinaryVectorReader reader(spirv);

    //module.reset();
    //module.read(&reader, gram);
    //module.resolveIDs();
    //module.reconstructTypeAndConstantInfo();
    //module.reconstructNames();

    //auto& eps = module.getEntryPoints();

    //auto& funcs = module.getFunctions();
    //auto& f = funcs.front();
    //auto& params = f.getParameters();
    //for (auto& p : params) {
    //    func.arguments.push_back(parser_spvgentwo_variable(p));
    //}
    //spvgentwo::Instruction* ret_type = f.getReturnType();
    //func.ret_type = parser_spvgentwo_variable(*ret_type);

    ////spvgentwo::String buffer(&alloc, 2048u);
    ////spvgentwo::ModulePrinter::ModuleStringPrinter printer(buffer, true);
    ////spvgentwo::ModulePrinter::printModule(module, gram, printer);
    ////printf("%s", buffer.c_str());

    ////////////////////////////////////////////////////////////////////////////

    //spvgentwo::HeapAllocator alloc;

    //// create a new spir-v module
    //Module module(&alloc, spvgentwo::spv::Version);

    //// configure capabilities and extensions
    //module.addCapability(spvgentwo::spv::Capability::Shader);

    //// global variables
    //Instruction* uniformVar = module.uniform<vector_t<float, 3>>(u8"u_Position");

    //// float add(float x, float y)
    //auto& funcAdd = module.addFunction<float, float, float>(u8"add", spvgentwo::spv::FunctionControlMask::Const);
    //{
    //    BasicBlock& bb = *funcAdd; // get entry block to this function

    //    Instruction* x = funcAdd.getParameter(0);
    //    Instruction* y = funcAdd.getParameter(1);

    //    Instruction* z = bb.Add(x, y);
    //    Instruction* z2 = bb.Sub(z, y);
    //    bb.returnValue(z2);
    //}

    //// void entryPoint();
    //{
    //    EntryPoint& entry = module.addEntryPoint(spvgentwo::spv::ExecutionModel::Fragment, u8"main");
    //    entry.addExecutionMode(spvgentwo::spv::ExecutionMode::OriginUpperLeft);
    //    BasicBlock& bb = *entry; // get entry block to this function

    //    Instruction* uniVec = bb->opLoad(uniformVar);
    //    Instruction* s = bb->opDot(uniVec, uniVec);
    //    entry->call(&funcAdd, s, s); // call add(s, s)
    //    entry->opReturn();
    //}

    ////////////////////////////////////////////////////////////////////////////

    //spvgentwo::HeapAllocator alloc;
    //Module module(&alloc, spvgentwo::spv::Version);
    //Grammar gram(&alloc);

    //BinaryFileReader reader("D:/projects/spirv/SpvGenTwo/cmake_out/fragment.spv");
    //reader.isOpen();

    //module.read(&reader, gram);

    ////////////////////////////////////////////////////////////////////////////

    //std::vector<unsigned int> newSPV;
    //spvgentwo::BinaryVectorWriter writer(newSPV);
    //module.write(&writer);

    //std::string glsl;
    //ShaderTrans::SpirV2GLSL(ShaderStage::PixelShader, newSPV, glsl);

    //printf("%s\n", glsl.c_str());
}

}