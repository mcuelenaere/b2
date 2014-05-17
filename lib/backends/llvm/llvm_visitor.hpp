#ifndef __LLVM_VISITOR_H_
#define __LLVM_VISITOR_H_

#include "llvm_bindings.h"
#include "ast/visitors.hpp"

#include <llvm/ADT/OwningPtr.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>

#include <string>
#include <unordered_map>

namespace b2 {

class LLVMVisitor : private Visitor<void>, private ExpressionVisitor<llvm::Value*> {
public:
    LLVMVisitor(llvm::IRBuilder<> &irBuilder, llvm::Module *module, LLVMBindings &bindings) : m_llvmContext(irBuilder.getContext()), m_irBuilder(irBuilder), m_module(module), m_bindings(bindings) {
    }

    llvm::Function* visit(AST* ast);
    LLVMBindings& bindings() { return m_bindings; }

private:
    virtual void statements(StatementsAST* ast) override;
    virtual void raw(RawBlockAST* ast) override;
    virtual void print_block(PrintBlockAST* ast) override;
    virtual void if_block(IfBlockAST* ast) override;
    virtual void for_block(ForBlockAST* ast) override;
    virtual void include_block(IncludeBlockAST* ast) override;

    virtual llvm::Value* variable_reference_expression(VariableReferenceExpression *expr) override;
    virtual llvm::Value* get_attribute_expression(GetAttributeExpression* expr) override;
    virtual llvm::Value* method_call_expression(MethodCallExpression *expr) override;
    virtual llvm::Value* double_literal_expression(DoubleLiteralExpression *expr) override;
    virtual llvm::Value* integer_literal_expression(IntegerLiteralExpression *expr) override;
    virtual llvm::Value* boolean_literal_expression(BooleanLiteralExpression *expr) override;
    virtual llvm::Value* string_literal_expression(StringLiteralExpression *expr) override;
    virtual llvm::Value* binary_operation_expression(BinaryOperationExpression *expr) override;
    virtual llvm::Value* unary_operation_expression(UnaryOperationExpression *expr) override;
    virtual llvm::Value* comparison_expression(ComparisonExpression *expr) override;

    llvm::OwningPtr<llvm::Function> m_function;
    llvm::LLVMContext& m_llvmContext;
    llvm::IRBuilder<>& m_irBuilder;
    llvm::Module* m_module;
    LLVMBindings& m_bindings;
    std::unordered_map<std::string, llvm::Value*> m_overriden_variables;
};

} // namespace b2

#endif /* __LLVM_VISITOR_H_ */
