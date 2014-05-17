#ifndef __RESOLVE_INCLUDES_PASS_HPP_
#define __RESOLVE_INCLUDES_PASS_HPP_

#include "ast/passes/pass.hpp"
#include "ast/passes/pass_manager.hpp"
#include "parser/parser.hpp"

#include <stdexcept>
#include <string>

namespace b2 {

class MissingVariableReferenceError : public std::runtime_error
{
public:
    MissingVariableReferenceError(const std::string &includeFilename, const std::string &variableName) :
        std::runtime_error("No value found for variable '" + variableName + "', referenced in '" + includeFilename + "'"),
        m_includeFilename(includeFilename),
        m_variableName(variableName)
    {
    }

    const std::string& includeFilename() const noexcept { return m_includeFilename; }
    const std::string& variableName() const noexcept { return m_variableName; }
private:
    const std::string m_includeFilename;
    const std::string m_variableName;
};

class ResolveIncludesPass : public ASTPass
{
public:
    ResolveIncludesPass(const std::string &includeBasePath) : m_includeBasePath(includeBasePath) {}
protected:
    virtual AST* process_node(IncludeBlockAST *ast) override;
private:
    std::string resolvePath(const std::string &path);
    AST* prependScope(AST* ast, Expression* scope);
    AST* replaceVariableReferences(AST* ast, const std::string &includeFilename, StringExpressionMap &replacements);

    const std::string m_includeBasePath;
    PassManager m_passManager;
    Parser m_parser;
};

} // namespace b2

#endif /* __RESOLVE_INCLUDES_PASS_HPP_ */
