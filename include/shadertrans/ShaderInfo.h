// code from filament's GLSLTools and ASTUtils

#pragma once

#include <string>
#include <vector>
#include <deque>
#include <list>

class TIntermNode;
namespace glslang  { class TIntermAggregate; }

namespace shadertrans
{

struct Access 
{
    enum Type {Swizzling, DirectIndexForStruct, FunctionCall};
    Type type;
    std::string string;
    size_t parameterIdx = 0; // Only used when type == FunctionCall;
};

class Symbol 
{
public:
    Symbol() {}
    Symbol(const std::string& name) {
        mName = name;
    }

    std::string& getName() {
        return mName;
    }

    std::list<Access>& getAccesses() {
        return mAccesses;
    };

    void setName(const std::string& name) {
        mName = name;
    }

    void add(const Access& access) {
        mAccesses.push_front(access);
    }

    std::string toString() const {
        std::string str(mName);
        for (Access access: mAccesses) {
            str += ".";
            str += access.string;
        }
        return str;
    }

    bool hasDirectIndexForStruct() const noexcept {
        for (Access access : mAccesses) {
            if (access.type == Access::Type::DirectIndexForStruct) {
                return true;
            }
        }
        return false;
    }

    const std::string getDirectIndexStructName() const noexcept {
        for (Access access : mAccesses) {
            if (access.type == Access::Type::DirectIndexForStruct) {
                return access.string;
            }
        }
        return "";
    }

private:
    std::list<Access> mAccesses;
    std::string mName;

}; // Symbol

class ShaderInfo
{
public:
	static glslang::TIntermAggregate* 
		GetFunctionByName(const std::string& name, TIntermNode& root) noexcept;

	struct FunctionParameter {
		enum Qualifier { IN, OUT, INOUT, CONST };
		std::string name;
		std::string type;
		Qualifier qualifier;
	};
	static void GetFunctionParameters(glslang::TIntermAggregate* func, 
		std::vector<FunctionParameter>& output) noexcept;

	static bool FindSymbolsUsage(const std::string& func_sign, TIntermNode& root,
		std::deque<Symbol>& symbols) noexcept;

private:
    static glslang::TIntermAggregate*
        GetFunctionBySignature(const std::string& sign, TIntermNode& root) noexcept;

}; // ShaderInfo

}