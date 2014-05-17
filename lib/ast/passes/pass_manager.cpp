#include "pass_manager.hpp"

#include <memory>

using namespace b2;

void PassManager::addPass(ASTPass* pass)
{
    std::unique_ptr<ASTPass> p_pass(pass);
    m_passes.push_back(std::move(p_pass));
}

namespace {

class ExpressionPassWrapper : public ASTPass {
public:
    ExpressionPassWrapper(ExpressionPass* expressionPass) : m_expressionPass(expressionPass) {}

protected:
    virtual AST* process_node(PrintBlockAST* ast) override {
        auto old_expr = ast->expr.get();
        auto new_expr = this->m_expressionPass->process(old_expr);
        if (new_expr != old_expr) {
            ast->expr.reset(new_expr);
        }

        return ast;
    }

    virtual AST* process_node(IfBlockAST* ast) override {
        auto old_condition = ast->condition.get();
        auto new_condition = this->m_expressionPass->process(old_condition);
        if (new_condition != old_condition) {
            ast->condition.reset(new_condition);
        }

        return ast;
    }

    virtual AST* process_node(ForBlockAST* ast) override {
        auto old_iterable = ast->iterable.get();
        auto new_iterable = this->m_expressionPass->process(old_iterable);
        if (new_iterable != old_iterable) {
            ast->iterable.reset(new_iterable);
        }

        return ast;
    }

    virtual AST* process_node(IncludeBlockAST* ast) override {
        if (ast->scope) {
            auto old_scope = ast->scope.get();
            auto new_scope = this->m_expressionPass->process(old_scope);
            if (new_scope != old_scope) {
                ast->scope.reset(new_scope);
            }
        }

        for (auto &tuple : ast->variableMapping) {
            auto old_expr = tuple.second.get();
            auto new_expr = this->m_expressionPass->process(old_expr);
            if (new_expr != old_expr) {
                tuple.second.reset(new_expr);
            }
        }

        return ast;
    }

private:
    std::unique_ptr<ExpressionPass> m_expressionPass;
};

} /* anon namespace */

void PassManager::addPass(ExpressionPass* pass)
{
    std::unique_ptr<ASTPass> p_pass(new ExpressionPassWrapper(pass));
    m_passes.push_back(std::move(p_pass));
}

void PassManager::removeAllPasses()
{
    m_passes.clear();
}

AST* PassManager::run(AST* ast)
{
    for (auto &pass : m_passes) {
        try {
            auto new_ast = pass->process(ast);
            if (new_ast != ast) {
                delete ast;
            }
            ast = new_ast;
        } catch (...) {
            // something bad happened, free the AST as we own it
            delete ast;
            throw;
        }
    }
    return ast;
}
