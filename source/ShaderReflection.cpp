#include "shadertrans/ShaderReflection.h"
#include "shadertrans/ShaderTrans.h"
#include "shadertrans/spirv_IR.h"
#include "shadertrans/spirv_Parser.h"

#include <spirv.hpp>
#include <spirv_glsl.hpp>
#include <spirv_reflect.h>

#include <fstream>

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

Variable parser_spirv_variable(const shadertrans::spirv::Variable& var,
                               const shadertrans::spirv::Module& module)
{
    Variable ret;
    ret.name = var.name;
    ret.type = VarType::Unknown;
    auto type = var.base_type;
    if (type == shadertrans::spirv::ValueType::Void) {
        type = var.type;
    }
    switch (type)
    {
    case shadertrans::spirv::ValueType::Void:
        ret.type = VarType::Void;
        break;
    case shadertrans::spirv::ValueType::Bool:
        ret.type = VarType::Bool;
        assert(var.type_comp_count == 1);
        break;
    case shadertrans::spirv::ValueType::Int:
        switch (var.type_comp_count)
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
    case shadertrans::spirv::ValueType::Float:
        switch (var.type_comp_count)
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
    case shadertrans::spirv::ValueType::Vector:
        ret.type = VarType::Array;
        break;
    case shadertrans::spirv::ValueType::Matrix:
        switch (var.type_comp_count)
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
    case shadertrans::spirv::ValueType::Struct:
        ret.type = VarType::Struct;
        {
            auto user = module.user_types.find(var.type_name);
            if (user != module.user_types.end()) {
                for (auto& v : user->second) {
                    ret.children.push_back(parser_spirv_variable(v, module));
                }
            }
        }
        break;
    default:
        ret.type = VarType::Unknown;
    }

    return ret;
}
//
//Variable parser_spvgentwo_variable(const spvgentwo::Instruction& inst)
//{
//    Variable ret;
//    if (!inst.isType()) {
//        ret.name = inst.getName();
//    }
//    ret.type = VarType::Unknown;
//
//    spvgentwo::Type type = *inst.getType();
//    if (type.isPointer()) {
//        auto& sub_types = type.getSubTypes();
//        assert(sub_types.size() == 1);
//        type = spvgentwo::Type(sub_types.front());
//    }
//
//    if (type.isVoid()) {
//        ret.type = VarType::Void;
//    } 
//    
//    else if (type.isBool()) {
//        ret.type = VarType::Bool;
//    } else if (type.isInt()) {
//        ret.type = VarType::Int;
//    } else if (type.isFloat()) {
//        ret.type = VarType::Float;
//    } else if (type.isMatrix()) {
//        switch (type.getVectorComponentCount())
//        {
//        case 2:
//            ret.type = VarType::Mat2;
//            break;
//        case 3:
//            ret.type = VarType::Mat3;
//            break;
//        case 4:
//            ret.type = VarType::Mat4;
//            break;
//        }
//    } else if (type.isVectorOfInt()) {
//        switch (type.getVectorComponentCount())
//        {
//        case 2:
//            ret.type = VarType::Int2;
//            break;
//        case 3:
//            ret.type = VarType::Int3;
//            break;
//        case 4:
//            ret.type = VarType::Int4;
//            break;
//        }
//    } else if (type.isVectorOfFloat()) {
//        switch (type.getVectorComponentCount())
//        {
//        case 2:
//            ret.type = VarType::Float2;
//            break;
//        case 3:
//            ret.type = VarType::Float3;
//            break;
//        case 4:
//            ret.type = VarType::Float4;
//            break;
//        }
//    } else if (type.isArray()) {
//        ret.type = VarType::Array;
//    } else if (type.isStruct()) {
//        ret.type = VarType::Struct;
//    }
//    return ret;
//}

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

    spirv::Parser parser;
    spirv::Module module;
    parser.Parse(spirv, module);
    auto f = module.functions.find(name);
    if (f == module.functions.end()) {
        return;
    }

    for (auto& v : f->second.arguments) {
        func.arguments.push_back(parser_spirv_variable(v, module));
    }
    func.ret_type = parser_spirv_variable(f->second.ret_type, module);
}

int ShaderReflection::GetFuncIndex(const std::vector<unsigned int>& spirv, const std::string& name)
{
    spirv::Parser parser;
    spirv::Module module;
    parser.Parse(spirv, module);
    auto f = module.functions.find(name);
    if (f == module.functions.end()) {
        return -1;
    } else {
        return f->second.index;
    }
}

}