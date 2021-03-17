#include "shadertrans/ShaderReflection.h"

#include <spirv.hpp>
#include <spirv_glsl.hpp>

namespace
{

using Uniform = shadertrans::ShaderReflection::Uniform;
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
                         std::vector<Uniform>& uniforms,
                         const std::string& base_name)
{   
    auto member_count = type.member_types.size();
    if (!type.array.empty()) 
    {
        Uniform v_array;
        v_array.name = base_name;
        v_array.type = VarType::Array;

        for (int i = 0, n = type.array[0]; i < n; ++i)
        {
            Uniform v_struct;
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
                    Uniform unif;

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
        Uniform v_struct;
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
                Uniform v_array;
                v_array.name = name;
                v_array.type = VarType::Array;

                for (int i = 0, n = sub_type.array[0]; i < n; ++i)
                {
                    std::string full_name = name + "[" + std::to_string(i) + "]";

                    Uniform unif;

                    unif.name = full_name;
                    unif.type = parse_spir_type(sub_type);

                    v_array.children.push_back(unif);
                }

                v_struct.children.push_back(v_array);
            }
            else
            {
                Uniform unif;

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

}

namespace shadertrans
{

void ShaderReflection::GetUniforms(const std::vector<unsigned int>& spirv, 
	                               std::vector<Uniform>& uniforms)
{
    spirv_cross::CompilerGLSL compiler(spirv);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	//// get_work_group_size_specialization_constants
	//auto entries = compiler.get_entry_points_and_stages();
	//for (auto& e : entries)
	//{
	//	if (e.execution_model == spv::ExecutionModelGLCompute)
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

		uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);		
    }

	for (auto& resource : resources.sampled_images)
	{
		Uniform unif;

		spirv_cross::SPIRType type = compiler.get_type(resource.base_type_id);

		unif.name = compiler.get_name(resource.id);
		uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

		// todo
		unif.type = VarType::Sampler;

		uniforms.push_back(unif);
	}

	//for (auto& resource : resources.storage_images)
	//{
	//	ImageUnit img;

	//	spirv_cross::SPIRType type = compiler.get_type(resource.base_type_id);

	//	img.name = compiler.get_name(resource.id);
	//	uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
	//	img.unit = compiler.get_decoration(resource.id, spv::DecorationBinding);
	//	img.fmt = parse_image_format(type.image.format);
	//	img.access = parse_image_access(type.image.access);

	//	m_images.push_back(img);

	//	Variable unif;
	//	unif.name = img.name;
	//	unif.type = VariableType::Sampler2D;
	//	m_uniforms.push_back(unif);
	//}
}

}