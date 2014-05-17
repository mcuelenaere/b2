#include "simple_variant.h"
#include "simple_bindings.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>

#include <llvm/IRReader/IRReader.h>

#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>

#include <llvm/Linker.h>

using namespace llvm;

extern "C" {
    extern const char simple_bindings_functions[];
    extern int simple_bindings_functions_len;
}

SimpleBindings::SimpleBindings(LLVMContext &context, Module* module) : m_context(context), m_module(module)
{
    SMDiagnostic err;
    auto buffer = MemoryBuffer::getMemBuffer(StringRef(simple_bindings_functions, simple_bindings_functions_len), "simple_bindings_functions.bc", false);
    auto functionsModule = llvm::ParseIR(buffer, err, context);
    // TODO: check err

    std::string linkerErr;
    if (Linker::LinkModules(m_module, functionsModule, Linker::DestroySource, &linkerErr) == true) {
        throw std::runtime_error("Error during linking of modules: " + linkerErr);
    }

#if 0
    m_variantType = m_module->getTypeByName("struct.Variant");
    if (!m_variantType) {
        throw std::runtime_error("Couldn't find struct Variant!");
    }
#else
    Type* structFields[] = {
        IntegerType::get(m_context, 64),
        IntegerType::get(m_context, 32)
    };
    m_variantType = StructType::get(m_context, structFields, false);
#endif
}

Function* SimpleBindings::findFunction(StringRef name)
{
    return m_module->getFunction(name);
}

FunctionType* SimpleBindings::getTemplateFunctionType()
{
    Function *lookupVariableMethod = findFunction("lookup_variable");
    Type* keyValueStruct = lookupVariableMethod->getArgumentList().front().getType();

    return FunctionType::get(Type::getVoidTy(m_context), (Type*) {keyValueStruct}, false);
}

Function* SimpleBindings::getPrintMethodForType(Type* type) {
    if (type->isFloatingPointTy()) {
        return findFunction("print_double");
    } else if (type->isIntegerTy(1)) {
        return findFunction("print_boolean");
    } else if (type->isIntegerTy(64)) {
        /* TODO: find out native integer width */
        return findFunction("print_integer");
    } else if (type->isPointerTy() && type->getPointerElementType()->isIntegerTy(8)) {
        return findFunction("print_string");
    } else if (isVariantType(type)) {
        return findFunction("print_variant");
    }

    // error
    throw std::runtime_error("Unknown type");
}

Value* SimpleBindings::createForLoopInit(IRBuilder<> &irBuilder, Value* iterable)
{
    // TODO
    return nullptr;
}

void SimpleBindings::createForLoopCleanup(IRBuilder<> &irBuilder, Value* iterable)
{
    // TODO
}

Value* SimpleBindings::createForLoopNextIteration(IRBuilder<> &irBuilder, Value* iterable)
{
    // TODO
    return nullptr;
}

void SimpleBindings::createForLoopGetVariables(IRBuilder<> &irBuilder, Value* iterable, Value** keyVariable, Value** valueVariable)
{
    // TODO
}

Value* SimpleBindings::createPrintCall(IRBuilder<> &irBuilder, Value *value)
{
    Type* valueType = value->getType();
    Function* printMethod = this->getPrintMethodForType(valueType);
    if (isVariantType(valueType)) {
        // TODO: this assumes x86_64!
        Value* callArgs[] = {
            irBuilder.CreateExtractValue(value, (unsigned[]){0}),
            irBuilder.CreateExtractValue(value, (unsigned[]){1})
        };
        return irBuilder.CreateCall(printMethod, callArgs);
    } else {
        return irBuilder.CreateCall(printMethod, value);
    }
}

Value* SimpleBindings::createVariantComparison(IRBuilder<> &irBuilder, ComparisonOperation op, Value* left, Value *right)
{
    Function *method = findFunction("compare_variant");

    // wrap left and/or right as variants, if they aren't already
    if (!isVariantType(left->getType())) {
        left = wrapAsVariant(left, irBuilder);
    }
    if (!isVariantType(right->getType())) {
        right = wrapAsVariant(right, irBuilder);
    }

    // construct call instruction
    Value* params[] = {
        irBuilder.getInt16(op),
        // TODO: this assumes x86_64!
        irBuilder.CreateExtractValue(left, (unsigned[]){0}),
        irBuilder.CreateExtractValue(left, (unsigned[]){1}),
        irBuilder.CreateExtractValue(right, (unsigned[]){0}),
        irBuilder.CreateExtractValue(right, (unsigned[]){1})
    };
    return irBuilder.CreateCall(method, params);
}

Value* SimpleBindings::createVariantBinaryOperation(IRBuilder<> &irBuilder, BinaryOperation op, Value* left, Value *right)
{
    Function *method = findFunction("binary_variant_operation");

    // wrap left and/or right as variants, if they aren't already
    if (!isVariantType(left->getType())) {
        left = wrapAsVariant(left, irBuilder);
    }
    if (!isVariantType(right->getType())) {
        right = wrapAsVariant(right, irBuilder);
    }

    // construct call instruction
    Value* params[] = {
        irBuilder.getInt8(op),
        // TODO: this assumes x86_64!
        irBuilder.CreateExtractValue(left, (unsigned[]){0}),
        irBuilder.CreateExtractValue(left, (unsigned[]){1}),
        irBuilder.CreateExtractValue(right, (unsigned[]){0}),
        irBuilder.CreateExtractValue(right, (unsigned[]){1})
    };
    return irBuilder.CreateCall(method, params);
}

Value* SimpleBindings::createVariableLookup(llvm::IRBuilder<> &irBuilder, const char* variableName)
{
    Function *lookupVariableMethod = findFunction("lookup_variable");
    Function* templateFunction = irBuilder.GetInsertBlock()->getParent();
    Value* lookupVariableArgs[] = {
        const_cast<Argument*>(&templateFunction->getArgumentList().front()),
        irBuilder.CreateGlobalStringPtr(variableName)
    };
    return irBuilder.CreateCall(lookupVariableMethod, lookupVariableArgs);
}

llvm::Value* SimpleBindings::createMethodCall(llvm::IRBuilder<> &irBuilder, const char* methodName, llvm::ArrayRef<llvm::Value*> arguments)
{
    // TODO
    return nullptr;
}

llvm::Value* SimpleBindings::createGetAttribute(llvm::IRBuilder<> &irBuilder, const char* attributeName, llvm::Value* value)
{
    // TODO
    return nullptr;
}

bool SimpleBindings::isVariantType(llvm::Type *type)
{
    return type->isStructTy() && static_cast<StructType*>(type)->isLayoutIdentical(m_variantType);
}

Value* SimpleBindings::wrapAsVariant(Value *value, llvm::IRBuilder<> &irBuilder)
{
    VariantType variantType;
    Type* valueType = value->getType();

    if (valueType->isFloatingPointTy()) {
        value = irBuilder.CreateBitCast(value, irBuilder.getInt64Ty());
        variantType = VariantDoubleType;
    } else if (valueType->isIntegerTy(1)) {
        value = irBuilder.CreateZExt(value, irBuilder.getInt64Ty());
        variantType = VariantBoolType;
    } else if (valueType->isIntegerTy(64)) {
         // TODO: find out native integer width
        variantType = VariantIntegerType;
    } else if (valueType->isPointerTy() && valueType->getPointerElementType()->isIntegerTy(8)) {
        value = irBuilder.CreatePtrToInt(value, irBuilder.getInt64Ty());
        variantType = VariantStringType;
    } else if (isVariantType(valueType)) {
        // shouldn't happen
        return value;
    } else {
        throw std::runtime_error("Unknown type");
    }

    auto variant = UndefValue::get(m_variantType);
    irBuilder.CreateInsertValue(variant, value, (unsigned[]) { 0 });
    irBuilder.CreateInsertValue(variant, irBuilder.getInt32(variantType), (unsigned[]) { 1 });
    return variant;
}
