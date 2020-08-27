#include "shadertrans/ShaderInfo.h"

#include <glslang/Include/intermediate.h>

namespace
{

std::string get_function_name(const std::string& functionSignature) noexcept 
{
  auto indexParenthesis = functionSignature.find("(");
  return functionSignature.substr(0, indexParenthesis);
}

// Traverse the AST to find the definition of a function based on its name/signature.
// e.g: prepareMaterial(struct-MaterialInputs-vf4-vf41;
class FunctionDefinitionFinder : public glslang::TIntermTraverser 
{
public:
    explicit FunctionDefinitionFinder(const std::string& functionName, bool useFQN = true)
            : mFunctionName (functionName), mUseFQN(useFQN) {
    }

    bool visitAggregate(glslang::TVisit, glslang::TIntermAggregate* node) override {
        if (node->getOp() == glslang::EOpFunction) {
            bool match;
            if (mUseFQN) {
                match = std::string(node->getName().c_str()) == mFunctionName.c_str();
            } else {
                std::string prospectFunctionName = get_function_name(node->getName().c_str());
                std::string cleanedFunctionName = get_function_name(mFunctionName);
                match = prospectFunctionName == cleanedFunctionName;
            }
            if (match) {
                mFunctionDefinitionNode = node;
                return false;
            }
        }
        return true;
    }

    glslang::TIntermAggregate* getFunctionDefinitionNode() const noexcept {
        return mFunctionDefinitionNode;
    }
private:
    const std::string& mFunctionName;
    bool mUseFQN;
    glslang::TIntermAggregate* mFunctionDefinitionNode = nullptr;
};

shadertrans::ShaderInfo::FunctionParameter::Qualifier 
glslang_qualifier_to_func_param(glslang::TStorageQualifier q)
{
    switch (q) 
    {
    case glslang::EvqIn: 
        return shadertrans::ShaderInfo::FunctionParameter::Qualifier::IN;
    case glslang::EvqInOut: 
        return shadertrans::ShaderInfo::FunctionParameter::Qualifier::INOUT;
    case glslang::EvqOut: 
        return shadertrans::ShaderInfo::FunctionParameter::Qualifier::OUT;
    case glslang::EvqConstReadOnly: 
        return shadertrans::ShaderInfo::FunctionParameter::Qualifier::CONST;
    default: 
        return shadertrans::ShaderInfo::FunctionParameter::Qualifier::IN;
    }
}

std::string getIndexDirectStructString(const glslang::TIntermBinary& node)
{
    const glslang::TTypeList& structNode = *(node.getLeft()->getType().getStruct());
    glslang::TIntermConstantUnion* index = node.getRight()->getAsConstantUnion();
    return structNode[index->getConstArray()[0].getIConst()].type->getFieldName().c_str();
}

const char* op2Str(glslang::TOperator op)
{
    switch (op) {
    case glslang::EOpAssign: return "EOpAssign";
    case glslang::EOpAddAssign: return "EOpAddAssign";
    case glslang::EOpSubAssign: return "EOpSubAssign";
    case glslang::EOpMulAssign: return "EOpMulAssign";
    case glslang::EOpDivAssign: return "EOpDivAssign";
    case glslang::EOpVectorSwizzle: return "EOpVectorSwizzle";
    case glslang::EOpIndexDirectStruct:return "EOpIndexDirectStruct";
    case glslang::EOpFunction:return "EOpFunction";
    case glslang::EOpFunctionCall:return "EOpFunctionCall";
    case glslang::EOpParameters:return "EOpParameters";
    default: return "???";
    }
}

const glslang::TIntermTyped* findLValueBase(const glslang::TIntermTyped* node, shadertrans::Symbol& symbol)
{
    do {
        // Make sure we have a binary node
        const glslang::TIntermBinary* binary = node->getAsBinaryNode();
        if (binary == nullptr) {
            return node;
        }

        // Check Operator
        glslang::TOperator op = binary->getOp();
        if (op != glslang::EOpIndexDirect && 
            op != glslang::EOpIndexIndirect && 
            op != glslang::EOpIndexDirectStruct && 
            op != glslang::EOpVectorSwizzle && 
            op != glslang::EOpMatrixSwizzle) 
        {
            return nullptr;
        }
        shadertrans::Access access;
        if (op == glslang::EOpIndexDirectStruct) {
            access.string = getIndexDirectStructString(*binary);
            access.type = shadertrans::Access::DirectIndexForStruct;
        } else {
            access.string = op2Str(op) ;
            access.type = shadertrans::Access::Swizzling;
        }
        symbol.add(access);
        node = node->getAsBinaryNode()->getLeft();
    } while (true);
}

class SymbolsTracer : public glslang::TIntermTraverser 
{
public:
    explicit SymbolsTracer(std::deque<shadertrans::Symbol>& events) : mEvents(events) {
    }

    // Function call site.
    bool visitAggregate(glslang::TVisit, glslang::TIntermAggregate* node) override {
        if (node->getOp() != glslang::EOpFunctionCall) {
            return true;
        }

        // Find function name.
        std::string functionName = node->getName().c_str();

        // Iterate on function parameters.
        for (size_t parameterIdx = 0; parameterIdx < node->getSequence().size(); parameterIdx++) {
            TIntermNode* parameter = node->getSequence().at(parameterIdx);
            // Parameter is not a pure symbol. It is indexed or swizzled.
            if (parameter->getAsBinaryNode()) {
                shadertrans::Symbol symbol;
                std::vector<shadertrans::Symbol> events;
                const glslang::TIntermTyped* n = findLValueBase(parameter->getAsBinaryNode(), symbol);
                if (n != nullptr && n->getAsSymbolNode() != nullptr) {
                    const glslang::TString& symbolTString = n->getAsSymbolNode()->getName();
                    symbol.setName(symbolTString.c_str());
                    events.push_back(symbol);
                }

                for (shadertrans::Symbol symbol: events) {
                    shadertrans::Access fCall = { shadertrans::Access::FunctionCall, functionName, parameterIdx};
                    symbol.add(fCall);
                    mEvents.push_back(symbol);
                }

            }
            // Parameter is a pure symbol.
            if (parameter->getAsSymbolNode()) {
                shadertrans::Symbol s(parameter->getAsSymbolNode()->getName().c_str());
                shadertrans::Access fCall = { shadertrans::Access::FunctionCall, functionName, parameterIdx};
                s.add(fCall);
                mEvents.push_back(s);
            }
        }

        return true;
    }

    // Assign operations
    bool visitBinary(glslang::TVisit, glslang::TIntermBinary* node) override 
    {
        glslang::TOperator op = node->getOp();
        shadertrans::Symbol symbol;
        if (op == glslang::EOpAssign || 
            op == glslang::EOpAddAssign || 
            op == glslang::EOpDivAssign || 
            op == glslang::EOpSubAssign || 
            op == glslang::EOpMulAssign) 
        {
            const glslang::TIntermTyped* n = findLValueBase(node->getLeft(), symbol);
            if (n != nullptr && n->getAsSymbolNode() != nullptr) {
                const glslang::TString& symbolTString = n->getAsSymbolNode()->getName();
                symbol.setName(symbolTString.c_str());
                mEvents.push_back(symbol);
                return false; // Don't visit subtree since we just traced it with findLValueBase()
            }
        }
        return true;
    }

private:
    std::deque<shadertrans::Symbol>& mEvents;
};

}

namespace shadertrans
{

glslang::TIntermAggregate* 
ShaderInfo::GetFunctionByName(const std::string& name, TIntermNode& root) noexcept
{
    FunctionDefinitionFinder functionDefinitionFinder(name, false);
    root.traverse(&functionDefinitionFinder);
    return functionDefinitionFinder.getFunctionDefinitionNode();
}

void ShaderInfo::GetFunctionParameters(glslang::TIntermAggregate* func, std::vector<FunctionParameter>& output) noexcept 
{
    if (func == nullptr) {
        return;
    }

    // Does it have a list of params
    // The second aggregate is the list of instructions, but the function may be empty
    if (func->getSequence().size() < 1) {
        return;
    }

    // A function aggregate has a sequence of two aggregate children:
    // Index 0 is a list of params (IntermSymbol).
    for(TIntermNode* parameterNode : func->getSequence().at(0)->getAsAggregate()->getSequence()) {
        glslang::TIntermSymbol* parameter = parameterNode->getAsSymbolNode();
        FunctionParameter p = {
                parameter->getName().c_str(),
                parameter->getType().getCompleteString().c_str(),
                glslang_qualifier_to_func_param(parameter->getType().getQualifier().storage)
        };
        output.push_back(p);
    }
}

bool ShaderInfo::FindSymbolsUsage(const std::string& func_sign, TIntermNode& root, 
                                  std::deque<shadertrans::Symbol>& symbols) noexcept
{
    //TIntermNode* functionAST = GetFunctionBySignature(func_sign, root);
    TIntermNode* functionAST = GetFunctionByName(func_sign, root);

    SymbolsTracer variableTracer(symbols);
    functionAST->traverse(&variableTracer);

    return true;
}

glslang::TIntermAggregate*
ShaderInfo::GetFunctionBySignature(const std::string& sign, TIntermNode& root) noexcept
{
    FunctionDefinitionFinder functionDefinitionFinder(sign);
    root.traverse(&functionDefinitionFinder);
    return functionDefinitionFinder.getFunctionDefinitionNode();
}

}