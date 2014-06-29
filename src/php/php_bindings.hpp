#ifndef __PHP_BINDINGS_H_
#define __PHP_BINDINGS_H_

#include "backends/llvm/llvm_bindings.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <memory>
#include <unordered_map>

namespace b2 {

struct ForLoopMetadata {
    llvm::Value* hashTable;
    llvm::Value* hashPosition;
};

class PHPBindings : public LLVMBindings
{
public:
    PHPBindings(llvm::IRBuilder<> &irBuilder);
    virtual ~PHPBindings() {}

	void linkInFunctions(llvm::Module* module);
	llvm::Module* getFunctionsModule() {
		return m_module.get();
	}

    virtual void functionTeardown() override {
        m_irBuilder.CreateRetVoid();

        // reset internal state
		m_variablesRefCount.clear();
    }

    virtual llvm::FunctionType* getTemplateFunctionType(llvm::Module* module) override;
    virtual bool isVariantType(llvm::Type *type) override;

    virtual void variableGoesOutOfScope(llvm::Value *value) override;
    virtual llvm::Value* getNewReferenceForVariable(llvm::Value *value) override;

    virtual llvm::Value* createPrintCall(llvm::Value *value) override;
    virtual llvm::Value* createForLoopInit(llvm::Value* iterable) override;
    virtual void createForLoopCleanup(llvm::Value* iterable) override;
    virtual llvm::Value* createForLoopNextIteration(llvm::Value* iterable) override;
    virtual void createForLoopGetVariables(llvm::Value* iterable, llvm::Value** keyVariable, llvm::Value** valueVariable) override;
    virtual llvm::Value* createVariantComparison(ComparisonOperation op, llvm::Value* left, llvm::Value *right) override;
    virtual llvm::Value* createVariantBinaryOperation(BinaryOperation op, llvm::Value* left, llvm::Value *right) override;
    virtual llvm::Value* createVariantUnaryOperation(UnaryOperation op, llvm::Value* val) override;
    virtual llvm::Value* createVariableLookup(const char* variableName) override;
    virtual llvm::Value* createMethodCall(const char* methodName, llvm::ArrayRef<llvm::Value*> arguments) override;
    virtual llvm::Value* createGetAttribute(const char* attributeName, llvm::Value* variable) override;
private:
	void createRetVoidIfCallFails(llvm::Value* callResult);
    llvm::Value* wrapAsVariant(llvm::Value* value);
    llvm::Function* getPrintMethodForType(llvm::Type* type);
    llvm::Function* findFunction(llvm::StringRef name);
    void linkInModule(llvm::Module* dest);

	inline llvm::StructType* getVariantType() {
		auto module = m_irBuilder.GetInsertBlock()->getParent()->getParent();
		auto variantType = module->getTypeByName("struct._zval_struct");
		if (!variantType) {
			throw std::runtime_error("Couldn't find _zval_struct!");
		}
		return variantType;
	}

	std::unique_ptr<llvm::Module> m_module;

    std::unordered_map<llvm::Value*,ForLoopMetadata> m_forLoopMetadata;
	std::unordered_map<llvm::Value*,int> m_variablesRefCount;
};

} // namespace b2

#endif // __PHP_BINDINGS_H_
