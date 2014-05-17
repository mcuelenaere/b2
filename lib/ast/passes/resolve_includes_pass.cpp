#include <memory>
#include <unordered_set>

#include "ast/passes/resolve_includes_pass.hpp"
#include "ast/passes/pass_manager.hpp"
#include "parser/parser.hpp"

using namespace b2;

namespace {

class ReplaceVariableReferencesPass : public ExpressionPass {
public:
    ReplaceVariableReferencesPass(const std::string &includeFilename, StringExpressionMap& replacements) :
        m_replacements(replacements), m_includeFilename(includeFilename) {}
protected:
    virtual Expression* process_node(VariableReferenceExpression *expr) override;
private:
    StringExpressionMap &m_replacements;
    const std::string &m_includeFilename;
};

Expression* ReplaceVariableReferencesPass::process_node(VariableReferenceExpression *expr)
{
    std::string variableName = expr->variableName.get();

    // look for replacement
    auto replacement = m_replacements.find(variableName);
    if (replacement == m_replacements.end()) {
        // not found, throw error
        throw MissingVariableReferenceError(m_includeFilename, variableName);
    }

    return replacement->second->clone();
}

class PrependScopePass : public ExpressionPass {
public:
    PrependScopePass(Expression* scope) : m_scope(scope) {}
protected:
    virtual Expression* process_node(VariableReferenceExpression *expr) override;
private:
    Expression* m_scope;
};

Expression* PrependScopePass::process_node(VariableReferenceExpression *expr)
{
    return new GetAttributeExpression(m_scope->clone(), strdup(expr->variableName.get()));
}

} /* anon namespace */

std::string ResolveIncludesPass::resolvePath(const std::string &path)
{
    // TODO: this is UNIX-specific
    if (path[0] == '/') {
        return path;
    }

    return m_includeBasePath + "/" + path;
}

AST* ResolveIncludesPass::replaceVariableReferences(AST* ast, const std::string &includeFilename, StringExpressionMap &replacements)
{
    m_passManager.removeAllPasses();
    m_passManager.addPass(new ReplaceVariableReferencesPass(includeFilename, replacements));
    return m_passManager.run(ast);
}

AST* ResolveIncludesPass::prependScope(AST *ast, Expression *scope)
{
    m_passManager.removeAllPasses();
    m_passManager.addPass(new PrependScopePass(scope));
    return m_passManager.run(ast);
}

AST* ResolveIncludesPass::process_node(IncludeBlockAST *ast)
{
    auto includeFilename = this->resolvePath(ast->includeName.get());
    std::unique_ptr<AST> includeAST(m_parser.parse(includeFilename.c_str()));

	// (recursively) process this node, it could contain 'include' blocks itself
	auto processedIncludeAST = this->process(includeAST.get());
	if (processedIncludeAST != includeAST.get()) {
		includeAST.reset(processedIncludeAST);
	}

    AST* newIncludeAST;
    if (ast->scope) {
        newIncludeAST = prependScope(includeAST.get(), ast->scope.get());
    } else {
        newIncludeAST = replaceVariableReferences(includeAST.get(), includeFilename, ast->variableMapping);
    }

    // replace includeAST, if necessary
    if (newIncludeAST != includeAST.get()) {
        includeAST.reset(newIncludeAST);
    }

    return includeAST.release();
}
