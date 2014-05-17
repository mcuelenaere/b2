#ifndef AST_WALKER_HPP
#define AST_WALKER_HPP

#include "ast/visitors.hpp"

namespace b2 {

class ASTPass : private Visitor<AST*>
{
public:
    AST* process(AST* ast) {
        return this->ast(ast);
    }

protected:
    virtual AST* process_node(StatementsAST* ast) { return ast; }
    virtual AST* process_node(RawBlockAST* ast) { return ast; }
    virtual AST* process_node(PrintBlockAST* ast) { return ast; }
    virtual AST* process_node(IfBlockAST* ast) { return ast; }
    virtual AST* process_node(ForBlockAST* ast) { return ast; }
    virtual AST* process_node(IncludeBlockAST *ast) { return ast; }

private:
    virtual AST* statements(StatementsAST* ast) override {
        auto new_ast = this->process_node(ast);
        if (new_ast->type() != StatementsASTType) {
            return this->ast(new_ast);
        }
        ast = static_cast<StatementsAST*>(new_ast);

        for (auto it = ast->statements->begin(); it != ast->statements->end(); ++it) {
            auto &statement = *it;
            auto old_statement = statement.get();
            auto new_statement = this->ast(old_statement);

            if (new_statement != old_statement) {
                statement.reset(new_statement);
            }

            if (statement->type() == StatementsASTType) {
                // this child is also a StatementsAST, merge it with its parent
                auto statementToRemove = it;

                // move statements from child StatementsAST to parent StatementsAST
                auto childStatements = static_cast<StatementsAST*>(statement.get());
                for (auto &childStatement : *childStatements->statements) {
                    it = ast->statements->insert(it, std::move(childStatement));
                    it++;
                }

                // remove child StatementsAST
                it = ast->statements->erase(statementToRemove);
                --it;
            }
        }

        // fold StatementsAST containing 1 child to its single child
        if (ast->statements->size() == 1) {
            return ast->statements->front().release();
        }

        return ast;
    }

    virtual AST* raw(RawBlockAST* ast) override {
        return this->process_node(ast);
    }

    virtual AST* print_block(PrintBlockAST* ast) override {
        return this->process_node(ast);
    }

    virtual AST* if_block(IfBlockAST* ast) override {
        auto new_ast = this->process_node(ast);
        if (new_ast->type() != IfBlockASTType) {
            return this->ast(new_ast);
        }
        ast = static_cast<IfBlockAST*>(new_ast);

        if (ast->thenBody) {
            auto oldThenBody = ast->thenBody.get();
            auto newThenBody = this->ast(oldThenBody);

            if (newThenBody != oldThenBody) {
                ast->thenBody.reset(newThenBody);
            }
        }

        if (ast->elseBody) {
            auto oldElseBody = ast->elseBody.get();
            auto newElseBody = this->ast(oldElseBody);

            if (newElseBody != oldElseBody) {
                ast->elseBody.reset(newElseBody);
            }
        }

        return ast;
    }

    virtual AST* for_block(ForBlockAST* ast) override {
        auto new_ast = this->process_node(ast);
        if (new_ast->type() != ForBlockASTType) {
            return this->ast(new_ast);
        }
        ast = static_cast<ForBlockAST*>(new_ast);

        if (ast->body) {
            auto oldBody = ast->body.get();
            auto newBody = this->ast(oldBody);

            if (newBody != oldBody) {
                ast->body.reset(newBody);
            }
        }

        if (ast->elseBody) {
            auto oldElseBody = ast->elseBody.get();
            auto newElseBody = this->ast(oldElseBody);

            if (newElseBody != oldElseBody) {
                ast->elseBody.reset(newElseBody);
            }
        }

        return ast;
    }

    virtual AST* include_block(IncludeBlockAST *ast) override {
        return this->process_node(ast);
    }
};

class ExpressionPass : private ExpressionVisitor<Expression*> {
public:
    Expression* process(Expression* expression) {
        return this->expression(expression);
    }

protected:
    virtual Expression* process_node(VariableReferenceExpression *expr) { return expr; }
    virtual Expression* process_node(GetAttributeExpression *expr) { return expr; }
    virtual Expression* process_node(MethodCallExpression *expr) { return expr; }
    virtual Expression* process_node(DoubleLiteralExpression *expr) { return expr; }
    virtual Expression* process_node(IntegerLiteralExpression *expr) { return expr; }
    virtual Expression* process_node(BooleanLiteralExpression *expr) { return expr; }
    virtual Expression* process_node(StringLiteralExpression *expr) { return expr; }
    virtual Expression* process_node(BinaryOperationExpression *expr) { return expr; }
    virtual Expression* process_node(UnaryOperationExpression *expr) { return expr; }
    virtual Expression* process_node(ComparisonExpression *expr) { return expr; }

private:
    virtual Expression* variable_reference_expression(VariableReferenceExpression *expr) override {
        return this->process_node(expr);
    }

    virtual Expression* get_attribute_expression(GetAttributeExpression *expr) override {
        auto new_expr = this->process_node(expr);
        if (new_expr->type() != GetAttributeExpressionType) {
            return this->expression(new_expr);
        }
        expr = static_cast<GetAttributeExpression*>(new_expr);

        auto old_variable = expr->variable.get();
        auto new_variable = this->expression(old_variable);
        if (new_variable != old_variable) {
            expr->variable.reset(new_variable);
        }

        return expr;
    }

    virtual Expression* method_call_expression(MethodCallExpression *expr) override {
        auto new_expr = this->process_node(expr);
        if (new_expr->type() != MethodCallExpressionType) {
            return this->expression(new_expr);
        }
        expr = static_cast<MethodCallExpression*>(new_expr);

        for (auto &argument : *expr->arguments) {
            auto old_argument = argument.get();
            auto new_argument = this->expression(old_argument);

            if (new_argument != old_argument) {
                argument.reset(new_argument);
            }
        }

        return expr;
    }

    virtual Expression* double_literal_expression(DoubleLiteralExpression *expr) override {
        return this->process_node(expr);
    }

    virtual Expression* integer_literal_expression(IntegerLiteralExpression *expr) override {
        return this->process_node(expr);
    }

    virtual Expression* boolean_literal_expression(BooleanLiteralExpression *expr) override {
        return this->process_node(expr);
    }

    virtual Expression* string_literal_expression(StringLiteralExpression *expr) override {
        return this->process_node(expr);
    }

    virtual Expression* binary_operation_expression(BinaryOperationExpression *expr) override {
        auto new_expr = this->process_node(expr);
        if (new_expr->type() != BinaryOperationExpressionType) {
            return this->expression(new_expr);
        }
        expr = static_cast<BinaryOperationExpression*>(new_expr);

        auto old_left = expr->left.get();
        auto new_left = this->expression(old_left);
        if (new_left != old_left) {
            expr->left.reset(new_left);
        }

        auto old_right = expr->right.get();
        auto new_right = this->expression(old_right);
        if (new_right != old_right) {
            expr->right.reset(new_right);
        }

        return expr;
    }

    virtual Expression* unary_operation_expression(UnaryOperationExpression *expr) override {
        auto new_expr = this->process_node(expr);
        if (new_expr->type() != UnaryOperationExpressionType) {
            return this->expression(new_expr);
        }
        expr = static_cast<UnaryOperationExpression*>(new_expr);

        auto old_sub_expr = expr->expr.get();
        auto new_sub_expr = this->expression(old_sub_expr);
        if (new_sub_expr != old_sub_expr) {
            expr->expr.reset(new_sub_expr);
        }

        return expr;
    }

    virtual Expression* comparison_expression(ComparisonExpression *expr) override {
        auto new_expr = this->process_node(expr);
        if (new_expr->type() != ComparisonExpressionType) {
            return this->expression(new_expr);
        }
        expr = static_cast<ComparisonExpression*>(new_expr);

        auto old_left = expr->left.get();
        auto new_left = this->expression(old_left);
        if (new_left != old_left) {
            expr->left.reset(new_left);
        }

        auto old_right = expr->right.get();
        auto new_right = this->expression(old_right);
        if (new_right != old_right) {
            expr->right.reset(new_right);
        }

        return expr;
    }
};

} // namespace b2

#endif // AST_WALKER_HPP
