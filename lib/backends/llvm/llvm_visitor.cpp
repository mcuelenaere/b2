#include "llvm_visitor.hpp"

#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/TypeBuilder.h>

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APFloat.h>

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/system_error.h>
#include <llvm/Support/MemoryBuffer.h>

using namespace llvm;
using namespace b2;

Function* LLVMVisitor::visit(AST* ast)
{
    // create function
    m_function.reset(Function::Create(m_bindings.getTemplateFunctionType(m_module), GlobalValue::ExternalLinkage, "template", m_module));

    m_bindings.functionSetup(m_function.get());

    // walk AST
    this->ast(ast);

    m_bindings.functionTeardown();

    return m_function.take();
}

void LLVMVisitor::statements(StatementsAST* ast)
{
    for (std::unique_ptr<AST> &statement : *ast->statements) {
        this->ast(statement.get());
    }
}

void LLVMVisitor::raw(RawBlockAST* ast)
{
    Value *stringPtr = m_irBuilder.CreateGlobalStringPtr(ast->text.get());
    m_bindings.createPrintCall(stringPtr);
}

void LLVMVisitor::print_block(PrintBlockAST* ast)
{
    Value *value = this->expression(ast->expr.get());
    m_bindings.createPrintCall(value);

    if (m_bindings.isVariantType(value->getType())) {
        m_bindings.variableGoesOutOfScope(value);
    }
}

void LLVMVisitor::if_block(IfBlockAST* ast)
{
    Value *condition = this->expression(ast->condition.get());
    if (!condition->getType()->isIntegerTy(1)) {
        // TODO
        throw std::runtime_error("unimplemented!");
    }

    BasicBlock* thenBlock = BasicBlock::Create(m_llvmContext, "then", m_function.get());
    BasicBlock* elseBlock = BasicBlock::Create(m_llvmContext, "else");
    BasicBlock* mergeBlock = BasicBlock::Create(m_llvmContext, "ifcont");

    // start then body
    m_irBuilder.CreateCondBr(condition, thenBlock, elseBlock);
    m_irBuilder.SetInsertPoint(thenBlock);

    // walk then body
    if (ast->thenBody) {
        this->ast(ast->thenBody.get());
    }

    m_irBuilder.CreateBr(mergeBlock);

    // start elseBlock
    m_function->getBasicBlockList().push_back(elseBlock);
    m_irBuilder.SetInsertPoint(elseBlock);

    // walk else body
    if (ast->elseBody) {
        this->ast(ast->elseBody.get());
    }

    m_irBuilder.CreateBr(mergeBlock);

    // start mergeBlock
    m_function->getBasicBlockList().push_back(mergeBlock);
    m_irBuilder.SetInsertPoint(mergeBlock);
}

void LLVMVisitor::for_block(ForBlockAST* ast)
{
    auto iterable = this->expression(ast->iterable.get());

    // create blocks
    auto loopBlock = BasicBlock::Create(m_llvmContext, "loop", m_function.get());
    auto loopElseBlock = BasicBlock::Create(m_llvmContext, "loopElse", m_function.get());
    auto afterLoopBlock = BasicBlock::Create(m_llvmContext, "afterLoop", m_function.get());

    // do initial termination test
    auto initalizationResult = m_bindings.createForLoopInit(iterable);
    m_irBuilder.CreateCondBr(initalizationResult, loopBlock, loopElseBlock);

    // create loop block
    m_irBuilder.SetInsertPoint(loopBlock);

    bool hasKeyVariable = (ast->keyVariable != nullptr);
    bool hasValueVariable = (ast->valueVariable != nullptr);

    std::string keyVariableName = hasKeyVariable ? ast->keyVariable->variableName.get() : "";
    std::string valueVariableName = hasValueVariable ? ast->valueVariable->variableName.get() : "";

    auto oldKeyVariable = m_overriden_variables[keyVariableName], oldValueVariable = m_overriden_variables[valueVariableName];
    llvm::Value *keyVariable = nullptr, *valueVariable = nullptr;

    m_bindings.createForLoopGetVariables(iterable, hasKeyVariable ? &keyVariable : nullptr, hasValueVariable ? &valueVariable : nullptr);

    // temporarily overwrite the variables
    if (hasKeyVariable && keyVariable) {
        m_overriden_variables[keyVariableName] = keyVariable;
    }
    if (hasValueVariable && valueVariable) {
        m_overriden_variables[valueVariableName] = valueVariable;
    }

    // visit the body
    if (ast->body) {
        this->ast(ast->body.get());
    }

    // destroy variables
    if (hasKeyVariable) {
        m_bindings.variableGoesOutOfScope(keyVariable);
    }
    if (hasValueVariable) {
        m_bindings.variableGoesOutOfScope(valueVariable);
    }

    // restore the old variables
    if (hasKeyVariable) {
        if (oldKeyVariable) {
            m_overriden_variables[keyVariableName] = oldKeyVariable;
        } else {
            m_overriden_variables.erase(keyVariableName);
        }
    }
    if (hasValueVariable) {
        if (oldValueVariable) {
            m_overriden_variables[valueVariableName] = oldValueVariable;
        } else {
            m_overriden_variables.erase(valueVariableName);
        }
    }

    // do termination test
    auto cond = m_bindings.createForLoopNextIteration(iterable);
    m_irBuilder.CreateCondBr(cond, loopBlock, afterLoopBlock);

    // create loopElse block
    m_irBuilder.SetInsertPoint(loopElseBlock);
    if (ast->elseBody) {
        this->ast(ast->elseBody.get());
    }
    m_irBuilder.CreateBr(afterLoopBlock);

    // all done
    m_irBuilder.SetInsertPoint(afterLoopBlock);
    m_bindings.createForLoopCleanup(iterable);
    m_bindings.variableGoesOutOfScope(iterable);
}

void LLVMVisitor::include_block(IncludeBlockAST *ast)
{
    throw std::runtime_error("LLVMVisitor doesn't support include blocks!");
}

Value* LLVMVisitor::variable_reference_expression(VariableReferenceExpression *expr)
{
    auto variableName = expr->variableName.get();
    auto overridenVariable = m_overriden_variables[variableName];
    if (overridenVariable) {
        return m_bindings.getNewReferenceForVariable(overridenVariable);
    }

    return m_bindings.createVariableLookup(variableName);
}

Value* LLVMVisitor::get_attribute_expression(GetAttributeExpression *expr)
{
    Value* variable = this->expression(expr->variable.get());
    return m_bindings.createGetAttribute(expr->attributeName.get(), variable);
}

Value* LLVMVisitor::method_call_expression(MethodCallExpression *expr)
{
    std::vector<llvm::Value*> arguments;
    for (auto &argument : *expr->arguments) {
        arguments.push_back(this->expression(argument.get()));
    }
    return m_bindings.createMethodCall(expr->methodName.get(), arguments);
}

Value* LLVMVisitor::double_literal_expression(DoubleLiteralExpression *expr)
{
    return ConstantFP::get(m_llvmContext, APFloat(expr->value));
}

Value* LLVMVisitor::integer_literal_expression(IntegerLiteralExpression *expr)
{
    // FIXME: hard-coded 64-bits
    return m_irBuilder.getInt64(expr->value);
}

Value* LLVMVisitor::boolean_literal_expression(BooleanLiteralExpression *expr)
{
    return expr->value ? m_irBuilder.getTrue() : m_irBuilder.getFalse();
}

Value* LLVMVisitor::string_literal_expression(StringLiteralExpression *expr)
{
    return m_irBuilder.CreateGlobalStringPtr(expr->value.get());
}

Value* LLVMVisitor::binary_operation_expression(BinaryOperationExpression *expr)
{
    Value* left = this->expression(expr->left.get());
    Value* right = this->expression(expr->right.get());

    if (m_bindings.isVariantType(left->getType()) || m_bindings.isVariantType(right->getType())) {
        // left and/or right are variants, this is a variant operation
        return m_bindings.createVariantBinaryOperation(expr->op, left, right);
    }

    if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        // left and right are integers, this is an integer operation
        switch (expr->op) {
            case '+':
                return m_irBuilder.CreateAdd(left, right);
            case '-':
                return m_irBuilder.CreateSub(left, right);
            case '*':
                return m_irBuilder.CreateMul(left, right);
            case '/':
                return m_irBuilder.CreateExactSDiv(left, right);
            case '%':
                return m_irBuilder.CreateSRem(left, right);
        }
    } else {
        // left and/or right are float types, this is a float comparison

        // convert to double if necessary
        if (!left->getType()->isFloatingPointTy()) {
            left = m_irBuilder.CreateSIToFP(left, m_irBuilder.getDoubleTy());
        }

        // same for right
        if (!right->getType()->isFloatingPointTy()) {
            right = m_irBuilder.CreateSIToFP(right, m_irBuilder.getDoubleTy());
        }

        switch (expr->op) {
            case '+':
                return m_irBuilder.CreateFAdd(left, right);
            case '-':
                return m_irBuilder.CreateFSub(left, right);
            case '*':
                return m_irBuilder.CreateFMul(left, right);
            case '/':
                return m_irBuilder.CreateFDiv(left, right);
            case '%':
                return m_irBuilder.CreateFRem(left, right);
        }
    }
}

Value* LLVMVisitor::unary_operation_expression(UnaryOperationExpression *expr)
{
    Value* subexpr = this->expression(expr->expr.get());

    // TODO: check operations
	if (m_bindings.isVariantType(subexpr->getType())) {
		return m_bindings.createVariantUnaryOperation(expr->op, subexpr);
	}

    switch (expr->op) {
        case NumericPositive:
            // TODO
            //m_irBuilder->CreateCast(NULL, subexpr, m_irBuilder.getDoubleTy());
            return subexpr;
        case NumericNegative:
            return m_irBuilder.CreateNeg(subexpr);
        case BooleanNegation:
            return m_irBuilder.CreateNot(subexpr);
    }
}

Value* LLVMVisitor::comparison_expression(ComparisonExpression *expr)
{
    Value* left = this->expression(expr->left.get());
    Value* right = this->expression(expr->right.get());

    if (m_bindings.isVariantType(left->getType()) || m_bindings.isVariantType(right->getType())) {
        // left and/or right are variants, this is a variant comparison
        return m_bindings.createVariantComparison(expr->op, left, right);
    }

    if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        // left and right are integers, this is an integer comparison
        switch (expr->op) {
            case Equal:
                return m_irBuilder.CreateICmpEQ(left, right);
            case NotEqual:
                return m_irBuilder.CreateICmpNE(left, right);
            case LessOrEqual:
                return m_irBuilder.CreateICmpSLE(left, right);
            case GreaterOrEqual:
                return m_irBuilder.CreateICmpSGE(left, right);
            case GreaterThan:
                return m_irBuilder.CreateICmpSGT(left, right);
            case LessThan:
                return m_irBuilder.CreateICmpSLT(left, right);
			case And:
				return m_irBuilder.CreateAnd(left, right);
			case Or:
				return m_irBuilder.CreateOr(left, right);
        }
    } else {
        // left and/or right are float types, this is a float comparison

        // TODO: support strings/bools!

        // convert to double if necessary
        if (!left->getType()->isFloatingPointTy()) {
            left = m_irBuilder.CreateSIToFP(left, m_irBuilder.getDoubleTy());
        }

        // same for right
        if (!right->getType()->isFloatingPointTy()) {
            right = m_irBuilder.CreateSIToFP(right, m_irBuilder.getDoubleTy());
        }

        // construct float compare instruction
        switch (expr->op) {
            case Equal:
                return m_irBuilder.CreateFCmpOEQ(left, right);
            case NotEqual:
                return m_irBuilder.CreateFCmpONE(left, right);
            case LessOrEqual:
                return m_irBuilder.CreateFCmpOLE(left, right);
            case GreaterOrEqual:
                return m_irBuilder.CreateFCmpOGE(left, right);
            case GreaterThan:
                return m_irBuilder.CreateFCmpOGT(left, right);
            case LessThan:
                return m_irBuilder.CreateFCmpOLT(left, right);
			case And:
				return m_irBuilder.CreateAnd(left, right);
			case Or:
				return m_irBuilder.CreateOr(left, right);
        }
    }
}
