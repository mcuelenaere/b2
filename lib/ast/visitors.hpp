#ifndef __VISITORS_H_
#define __VISITORS_H_

#include "ast.hpp"
#include "expressions.hpp"

namespace b2 {

/*
 * NOTE: all values passed to a visitor's method are only valid for the lifetime of that call. Iow, when
 * you want to hold on to those values, you'll need to make a (deep) copy of them!
 */

template<typename T>
class Visitor
{
public:
	Visitor() {}
	virtual ~Visitor() {}

    T ast(AST* ast) {
        switch (ast->type()) {
            case StatementsASTType:
                return this->statements(static_cast<StatementsAST*>(ast));
            case RawBlockASTType:
                return this->raw(static_cast<RawBlockAST*>(ast));
            case PrintBlockASTType:
                return this->print_block(static_cast<PrintBlockAST*>(ast));
            case IfBlockASTType:
                return this->if_block(static_cast<IfBlockAST*>(ast));
            case ForBlockASTType:
                return this->for_block(static_cast<ForBlockAST*>(ast));
            case IncludeBlockASTType:
                return this->include_block(static_cast<IncludeBlockAST*>(ast));
        }
    }

    virtual T statements(StatementsAST* ast) = 0;
    virtual T raw(RawBlockAST* ast) = 0;
    virtual T print_block(PrintBlockAST* ast) = 0;
    virtual T if_block(IfBlockAST* ast) = 0;
    virtual T for_block(ForBlockAST* ast) = 0;
    virtual T include_block(IncludeBlockAST* ast) = 0;
};

template<typename T>
class ExpressionVisitor
{
public:
	ExpressionVisitor() {}
	virtual ~ExpressionVisitor() {}

    T expression(Expression* expr) {
		switch (expr->type()) {
			case VariableReferenceExpressionType:
                return this->variable_reference_expression(static_cast<VariableReferenceExpression*>(expr));
            case GetAttributeExpressionType:
                return this->get_attribute_expression(static_cast<GetAttributeExpression*>(expr));
            case MethodCallExpressionType:
                return this->method_call_expression(static_cast<MethodCallExpression*>(expr));
			case DoubleLiteralExpressionType:
                return this->double_literal_expression(static_cast<DoubleLiteralExpression*>(expr));
			case IntegerLiteralExpressionType:
                return this->integer_literal_expression(static_cast<IntegerLiteralExpression*>(expr));
			case BooleanLiteralExpressionType:
                return this->boolean_literal_expression(static_cast<BooleanLiteralExpression*>(expr));
			case StringLiteralExpressionType:
                return this->string_literal_expression(static_cast<StringLiteralExpression*>(expr));
			case BinaryOperationExpressionType:
                return this->binary_operation_expression(static_cast<BinaryOperationExpression*>(expr));
			case UnaryOperationExpressionType:
                return this->unary_operation_expression(static_cast<UnaryOperationExpression*>(expr));
			case ComparisonExpressionType:
                return this->comparison_expression(static_cast<ComparisonExpression*>(expr));
		}
	}

    virtual T variable_reference_expression(VariableReferenceExpression *expr) = 0;
    virtual T get_attribute_expression(GetAttributeExpression *expr) = 0;
    virtual T method_call_expression(MethodCallExpression *expr) = 0;
    virtual T double_literal_expression(DoubleLiteralExpression *expr) = 0;
    virtual T integer_literal_expression(IntegerLiteralExpression *expr) = 0;
    virtual T boolean_literal_expression(BooleanLiteralExpression *expr) = 0;
    virtual T string_literal_expression(StringLiteralExpression *expr) = 0;
    virtual T binary_operation_expression(BinaryOperationExpression *expr) = 0;
    virtual T unary_operation_expression(UnaryOperationExpression *expr) = 0;
    virtual T comparison_expression(ComparisonExpression *expr) = 0;
};

} // namespace b2

#endif /* __VISITORS_H_ */
