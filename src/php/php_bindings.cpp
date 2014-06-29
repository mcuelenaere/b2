#include "php_template.h"
#include "php_bindings.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/ValueSymbolTable.h>

#include <llvm/IRReader/IRReader.h>

#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>

#include <llvm/Linker.h>

using namespace llvm;
using namespace b2;

extern "C" {
    extern const char php_bindings_functions[];
    extern int php_bindings_functions_len;
}

PHPBindings::PHPBindings(IRBuilder<> &irBuilder) : LLVMBindings(irBuilder)
{
	SMDiagnostic irErr;
    auto buffer = MemoryBuffer::getMemBuffer(StringRef(php_bindings_functions, php_bindings_functions_len), "php_bindings_functions.bc", false);
	m_module.reset(llvm::ParseIR(buffer, irErr, m_irBuilder.getContext()));
    // TODO: check err
}

void PHPBindings::linkInFunctions(llvm::Module *module)
{
	std::string linkerErr;
    if (Linker::LinkModules(module, m_module.get(), Linker::PreserveSource, &linkerErr) == true) {
        throw std::runtime_error("Error during linking of modules: " + linkerErr);
    }
}

static Value* getArgumentAtIdx(Function* func, size_t argIdx)
{
    if (argIdx >= func->arg_size()) {
        throw std::out_of_range("Function has less arguments than what was asked for");
    }

    size_t i = 0;
    for (auto it = func->arg_begin(); it != func->arg_end(); i++, it++) {
        if (i == argIdx) {
            return it;
        }
    }
}

void PHPBindings::createRetVoidIfCallFails(Value* callResult)
{
    auto templateFn = m_irBuilder.GetInsertBlock()->getParent();
    auto successBlock = BasicBlock::Create(m_irBuilder.getContext(), "callSucceeded", templateFn);
    auto failBlock = BasicBlock::Create(m_irBuilder.getContext(), "callFailed", templateFn);

    m_irBuilder.CreateCondBr(callResult, successBlock, failBlock);

    m_irBuilder.SetInsertPoint(failBlock);
    m_irBuilder.CreateRetVoid();

    m_irBuilder.SetInsertPoint(successBlock);
}


Function* PHPBindings::findFunction(StringRef name)
{
	auto module = m_irBuilder.GetInsertBlock()->getParent()->getParent();
    auto func = module->getFunction(name);
    if (func == nullptr) {
        throw std::runtime_error("Couldn't find function '" + name.str() + "'");
    }
    return func;
}

FunctionType* PHPBindings::getTemplateFunctionType(Module* module)
{
    auto functionType = module->getFunction("php_prototype")->getFunctionType();
    if (!functionType) {
        throw std::runtime_error("Couldn't find php_prototype!");
    }
	return functionType;
}

Function* PHPBindings::getPrintMethodForType(Type* type) {
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

Value* PHPBindings::createPrintCall(Value *value)
{
    auto valueType = value->getType();
    auto printMethod = this->getPrintMethodForType(valueType);

    auto templateFn = m_irBuilder.GetInsertBlock()->getParent();

    Value* callArgs[] = {
        /*v*/      value,
        /*buffer*/ getArgumentAtIdx(templateFn, 1) // TODO: use "buffer" instead of 1
    };
    return m_irBuilder.CreateCall(printMethod, callArgs);
}

llvm::Value* PHPBindings::createForLoopInit(llvm::Value* iterable)
{
    auto hashPositionPtrType = m_module->getTypeByName("struct.bucket")->getPointerTo();
    auto hashPosition = m_irBuilder.CreateAlloca(hashPositionPtrType);

    Value* callArgs[] = {
        /*value*/  iterable,
        /*ht_pos*/ hashPosition,
    };
    auto hashTable = m_irBuilder.CreateCall(findFunction("forloop_init"), callArgs);

    m_forLoopMetadata[iterable] = (ForLoopMetadata) {
        hashTable,
        hashPosition,
    };

    auto hashTablePtrType = m_module->getTypeByName("struct._hashtable")->getPointerTo();
    return m_irBuilder.CreateICmpNE(hashTable, ConstantPointerNull::get(hashTablePtrType));
}

void PHPBindings::createForLoopCleanup(llvm::Value* iterable) {
    m_forLoopMetadata.erase(iterable);
}

llvm::Value* PHPBindings::createForLoopNextIteration(llvm::Value* iterable)
{
    auto metadata = m_forLoopMetadata[iterable];
    Value* callArgs[] = {
        /*ht*/     metadata.hashTable,
        /*ht_pos*/ metadata.hashPosition,
    };
    return m_irBuilder.CreateCall(findFunction("forloop_next"), callArgs);
}

void PHPBindings::createForLoopGetVariables(llvm::Value* iterable, llvm::Value** keyVariablePtr, llvm::Value** valueVariablePtr)
{
    auto metadata = m_forLoopMetadata[iterable];
    auto variantPtrType = getVariantType()->getPointerTo();
    auto variantPtrPtrType = variantPtrType->getPointerTo();

#if 0
    llvm::Value* keyVariable = (keyVariablePtr ? irBuilder.CreateAlloca(variantPtrType) : ConstantPointerNull::get(variantPtrPtrType));
    llvm::Value* valueVariable = (valueVariablePtr ? irBuilder.CreateAlloca(variantPtrType) : ConstantPointerNull::get(variantPtrPtrType));
#else
    llvm::Value* keyVariable;
    if (keyVariablePtr) {
        keyVariable = m_irBuilder.CreateAlloca(variantPtrType);
    } else {
        keyVariable = ConstantPointerNull::get(variantPtrPtrType);
    }
    llvm::Value* valueVariable;
    if (valueVariablePtr) {
        valueVariable = m_irBuilder.CreateAlloca(variantPtrType);
    } else {
        valueVariable = ConstantPointerNull::get(variantPtrPtrType);
    }
#endif

    Value* callArgs[] = {
        /*ht*/     metadata.hashTable,
        /*ht_pos*/ metadata.hashPosition,
        /*value*/  valueVariable,
        /*key*/    keyVariable,
    };
    m_irBuilder.CreateCall(findFunction("forloop_getvalues"), callArgs);

    if (keyVariablePtr) {
        *keyVariablePtr = m_irBuilder.CreateLoad(keyVariable);
        // key should be destroyed when it goes out of scope
		m_variablesRefCount[*keyVariablePtr] = 1;
    }

    if (valueVariablePtr) {
        *valueVariablePtr = m_irBuilder.CreateLoad(valueVariable);
        // value should _not_ be destroyed when it goes out of scope
		m_variablesRefCount[*valueVariablePtr] = -1;
    }
}

Value* PHPBindings::createVariantComparison(ComparisonOperation op, Value* left, Value *right)
{
    // wrap left and/or right as variants, if they aren't already
    if (!isVariantType(left->getType())) {
        left = wrapAsVariant(left);
    }
    if (!isVariantType(right->getType())) {
        right = wrapAsVariant(right);
    }

    auto ptrToResult = m_irBuilder.CreateAlloca(m_irBuilder.getInt8Ty());

    // construct call instruction
    Value* params[] = {
        /*op*/         m_irBuilder.getInt16(op),
        /*cmp_result*/ ptrToResult,
        /*left*/       left,
        /*right*/      right
    };
    auto callResult = m_irBuilder.CreateCall(findFunction("compare_variant"), params);
    createRetVoidIfCallFails(callResult);

    llvm::Value* result = m_irBuilder.CreateLoad(ptrToResult);
    result = m_irBuilder.CreateTrunc(result, m_irBuilder.getInt1Ty());

    // destroy variants, if refcount == 1
    variableGoesOutOfScope(left);
    variableGoesOutOfScope(right);

    return result;
}

Value* PHPBindings::createVariantBinaryOperation(BinaryOperation op, Value* left, Value *right)
{
    // wrap left and/or right as variants, if they aren't already
    if (!isVariantType(left->getType())) {
        left = wrapAsVariant(left);
    }
    if (!isVariantType(right->getType())) {
        right = wrapAsVariant(right);
    }

    auto variantPtrType = getVariantType()->getPointerTo();
    auto ptrToResult = m_irBuilder.CreateAlloca(variantPtrType);

    // construct call instruction
    Value* params[] = {
        /*op*/     m_irBuilder.getInt8(op),
        /*result*/ ptrToResult,
        /*left*/   left,
        /*right*/  right
    };
    auto callResult = m_irBuilder.CreateCall(findFunction("binary_variant_operation"), params);
    createRetVoidIfCallFails(callResult);

    auto value = m_irBuilder.CreateLoad(ptrToResult);

	// init variable refcount
	m_variablesRefCount[value] = 1;

    // destroy variants, if refcount == 1
    variableGoesOutOfScope(left);
    variableGoesOutOfScope(right);

    return value;
}

Value* PHPBindings::createVariantUnaryOperation(UnaryOperation op, Value *val)
{
    // wrap val as variant, if it isn't already
    if (!isVariantType(val->getType())) {
        val = wrapAsVariant(val);
    }
	
    auto variantPtrType = getVariantType()->getPointerTo();
    auto ptrToResult = m_irBuilder.CreateAlloca(variantPtrType);
	
    // construct call instruction
    Value* params[] = {
        /*op*/     m_irBuilder.getInt8(op),
        /*result*/ ptrToResult,
        /*val*/    val,
    };
    auto callResult = m_irBuilder.CreateCall(findFunction("unary_variant_operation"), params);
    createRetVoidIfCallFails(callResult);
	
    auto value = m_irBuilder.CreateLoad(ptrToResult);

	// init variable refcount
	m_variablesRefCount[value] = 1;

    // destroy variants, if refcount == 1
    variableGoesOutOfScope(val);

    return value;
}

Value* PHPBindings::createVariableLookup(const char* variableName)
{
    auto function = m_irBuilder.GetInsertBlock()->getParent();

    Value* lookupVariableArgs[] = {
        /*map*/       getArgumentAtIdx(function, 0), // TODO: use "assignments" instead of 1
        /*key*/       m_irBuilder.CreateGlobalStringPtr(variableName),
        /*keyLength*/ m_irBuilder.getInt32(strlen(variableName))
    };
    auto value = m_irBuilder.CreateCall(findFunction("get_value_from_hashtable"), lookupVariableArgs);

    // tag this variable as not-to-be-destroyed
	m_variablesRefCount[value] = -1;

    return value;
}

llvm::Value* PHPBindings::createMethodCall(const char* methodName, llvm::ArrayRef<llvm::Value*> arguments)
{
    auto variantPtrType = getVariantType()->getPointerTo();
    llvm::Value* argumentsValue;
    std::vector<llvm::Value*> valuesToDestroy;

    if (arguments.size() > 0) {
        argumentsValue = m_irBuilder.CreateAlloca(variantPtrType, m_irBuilder.getInt32(arguments.size()));

        int index = 0;
        for (llvm::Value* argument : arguments) {
            auto ptr = m_irBuilder.CreateInBoundsGEP(argumentsValue, m_irBuilder.getInt32(index++));
            if (!isVariantType(argument->getType())) {
                argument = wrapAsVariant(argument);
                valuesToDestroy.push_back(argument);
            }
            m_irBuilder.CreateStore(argument, ptr);
        }
    } else {
        argumentsValue = ConstantPointerNull::get(variantPtrType->getPointerTo());
    }

    Value* methodCallArgs[] = {
        /*functionName*/       m_irBuilder.CreateGlobalStringPtr(methodName),
        /*functionNameLength*/ m_irBuilder.getInt32(strlen(methodName)),
        /*param_count*/        m_irBuilder.getInt32(arguments.size()),
        /*params*/             argumentsValue,
    };
    auto returnValue = m_irBuilder.CreateCall(findFunction("do_method_call"), methodCallArgs);

    for (auto value : valuesToDestroy) {
        variableGoesOutOfScope(value);
    }

    return returnValue;
}

llvm::Value* PHPBindings::createGetAttribute(const char* attribute, llvm::Value* variable)
{
    Value* getAttributeArgs[] = {
        /*map*/       variable,
        /*key*/       m_irBuilder.CreateGlobalStringPtr(attribute),
        /*keyLength*/ m_irBuilder.getInt32(strlen(attribute))
    };
    auto value = m_irBuilder.CreateCall(findFunction("get_attribute"), getAttributeArgs);

    // tag this variable as not-to-be-destroyed
	m_variablesRefCount[value] = -1;

	// destroy parent variable, if it has refcount==1
	variableGoesOutOfScope(variable);

    return value;
}

bool PHPBindings::isVariantType(llvm::Type *type)
{
    return type->isPointerTy() && static_cast<PointerType*>(type)->getElementType()->isStructTy() &&
            static_cast<StructType*>(type->getPointerElementType())->isLayoutIdentical(getVariantType());
}

Value* PHPBindings::wrapAsVariant(Value *value)
{
    Value* wrappingFunction;
    Type* valueType = value->getType();

    if (valueType->isFloatingPointTy()) {
        wrappingFunction = this->findFunction("set_zval_double");
    } else if (valueType->isIntegerTy(1)) {
        wrappingFunction = this->findFunction("set_zval_bool");
    } else if (valueType->isIntegerTy(64)) {
         // TODO: find out native integer width
        wrappingFunction = this->findFunction("set_zval_int");
    } else if (valueType->isPointerTy() && valueType->getPointerElementType()->isIntegerTy(8)) {
        wrappingFunction = this->findFunction("set_zval_string_nocopy");
    } else if (isVariantType(valueType)) {
        // shouldn't happen
        return value;
    } else {
        throw std::runtime_error("Unknown type");
    }

    // alloc zval
    auto zval = m_irBuilder.CreateAlloca(getVariantType());

    // init zval
    Value* args[] = {
        /*zval*/  zval,
        /*value*/ value,
    };
    m_irBuilder.CreateCall(wrappingFunction, args);

    // tag zval as not-to-be-destroyed
	m_variablesRefCount[zval] = -1;

    return zval;
}

void PHPBindings::variableGoesOutOfScope(Value* value)
{
	auto it = m_variablesRefCount.find(value);

	if (it == m_variablesRefCount.end()) {
		throw std::runtime_error("Variable found without refcount!");
	}

    // check if we need to skip the destruction of this variable
    if (it->second == -1) {
        return;
    }

	if (--it->second == 0) {
		Value* args[] = {
			/*value*/ value,
		};
		m_irBuilder.CreateCall(findFunction("destruct_zval"), args);
	}
}

Value* PHPBindings::getNewReferenceForVariable(Value *value)
{
	auto it = m_variablesRefCount.find(value);

	if (it == m_variablesRefCount.end()) {
		throw std::runtime_error("Variable found without refcount!");
	}

	if (it->second != -1) {
		it->second++;
	}

    return value;
}
