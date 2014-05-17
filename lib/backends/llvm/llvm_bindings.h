#ifndef LLVM_BINDINGS_H
#define LLVM_BINDINGS_H

#include "ast/expressions.hpp"

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

namespace b2 {

class LLVMBindings
{
public:
    LLVMBindings(llvm::IRBuilder<> &irBuilder) : m_irBuilder(irBuilder) {}
    virtual ~LLVMBindings() {}

    /*
     */
    virtual llvm::FunctionType* getTemplateFunctionType(llvm::Module* module) = 0;

    /*
     */
    virtual bool isVariantType(llvm::Type *type) = 0;

    /*
     */
    virtual void functionSetup(llvm::Function* function) {
        auto block = llvm::BasicBlock::Create(m_irBuilder.getContext(), "entry", function);
        m_irBuilder.SetInsertPoint(block);
    }

    /*
     */
    virtual void functionTeardown() {
        m_irBuilder.CreateRetVoid();
    }

    /*
     * This tags the the given `value` as used.
     *
     * For example, when refence counting variables this will decrease the refcount and
     * optionally free it.
     */
    virtual void variableGoesOutOfScope(llvm::Value *value) {
        // do nothing
    }

    /*
     * This returns a new reference to the given `value`.
     *
     * For example, when variables are reference counted internally, this will return the same variable
     * but with 1 added to its refcount.
     */
    virtual llvm::Value* getNewReferenceForVariable(llvm::Value *value) {
        // do nothing
        return value;
    }

    /*
     */
    virtual llvm::Value* createPrintCall(llvm::Value *value) = 0;

    /*
     */
    virtual llvm::Value* createForLoopInit(llvm::Value* iterable) = 0;

    /*
     */
    virtual void createForLoopCleanup(llvm::Value* iterable) = 0;

    /*
     */
    virtual llvm::Value* createForLoopNextIteration(llvm::Value* iterable) = 0;

    /*
     */
    virtual void createForLoopGetVariables(llvm::Value* iterable, llvm::Value** keyVariable, llvm::Value** valueVariable) = 0;

    /*
     */
    virtual llvm::Value* createVariantComparison(ComparisonOperation op, llvm::Value* left, llvm::Value *right) = 0;

    /*
     */
    virtual llvm::Value* createVariantBinaryOperation(BinaryOperation op, llvm::Value* left, llvm::Value *right) = 0;

	/*
     */
    virtual llvm::Value* createVariantUnaryOperation(UnaryOperation op, llvm::Value* val) = 0;

    /*
     */
    virtual llvm::Value* createVariableLookup(const char* variableName) = 0;

    /*
     */
    virtual llvm::Value* createMethodCall(const char* methodName, llvm::ArrayRef<llvm::Value*> arguments) = 0;

    /*
     */
    virtual llvm::Value* createGetAttribute(const char* attributeName, llvm::Value* variable) = 0;

protected:
	llvm::IRBuilder<> &m_irBuilder;
};

} // namespace b2

#endif // LLVM_BINDINGS_H
