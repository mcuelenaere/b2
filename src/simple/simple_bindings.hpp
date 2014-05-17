#ifndef SIMPLE_BINDINGS_H
#define SIMPLE_BINDINGS_H

#include "backends/llvm/llvm_bindings.h"
#include <llvm/IR/LLVMContext.h>

namespace b2 {

class SimpleBindings : public LLVMBindings
{
public:
    SimpleBindings(llvm::LLVMContext &context, llvm::Module* module);
    virtual ~SimpleBindings() {}

    virtual llvm::FunctionType* getTemplateFunctionType() override;

    virtual llvm::Value* createPrintCall(llvm::IRBuilder<> &irBuilder, llvm::Value *value) override;
    virtual llvm::Value* createForLoopInit(llvm::IRBuilder<> &irBuilder, llvm::Value* iterable) override;
    virtual void createForLoopCleanup(llvm::IRBuilder<> &irBuilder, llvm::Value* iterable) override;
    virtual llvm::Value* createForLoopNextIteration(llvm::IRBuilder<> &irBuilder, llvm::Value* iterable) override;
    virtual void createForLoopGetVariables(llvm::IRBuilder<> &irBuilder, llvm::Value* iterable, llvm::Value** keyVariable, llvm::Value** valueVariable) override;
    virtual llvm::Value* createVariantComparison(llvm::IRBuilder<> &irBuilder, ComparisonOperation op, llvm::Value* left, llvm::Value *right) override;
    virtual llvm::Value* createVariantBinaryOperation(llvm::IRBuilder<> &irBuilder, BinaryOperation op, llvm::Value* left, llvm::Value *right) override;
    virtual llvm::Value* createVariableLookup(llvm::IRBuilder<> &irBuilder, const char* variableName) override;
    virtual llvm::Value* createMethodCall(llvm::IRBuilder<> &irBuilder, const char* methodName, llvm::ArrayRef<llvm::Value*> arguments) override;
    virtual llvm::Value* createGetAttribute(llvm::IRBuilder<> &irBuilder, const char* attributeName, llvm::Value* value) override;

    virtual bool isVariantType(llvm::Type *type) override;
    llvm::Value* wrapAsVariant(llvm::Value* value, llvm::IRBuilder<> &irBuilder);

    llvm::Module* getModule() {
        return m_module;
    }

private:
    llvm::Function* getPrintMethodForType(llvm::Type* type);
    llvm::Function* findFunction(llvm::StringRef name);

    llvm::LLVMContext &m_context;
    llvm::Module *m_module;
    llvm::StructType *m_variantType;
};

} // namespace b2

#endif // SIMPLE_BINDINGS_H
