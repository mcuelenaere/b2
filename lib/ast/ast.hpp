#ifndef __AST_H_
#define __AST_H_

#include "expressions.hpp"
#include <list>
#include <memory>
#include <unordered_map>
#include <string>
#include "utils.hpp"

namespace b2 {

enum ASTType {
    StatementsASTType,
    RawBlockASTType,
    PrintBlockASTType,
    IfBlockASTType,
    ForBlockASTType,
    IncludeBlockASTType,
};

struct AST {
    virtual ~AST() {}
    virtual ASTType type() = 0;
};

template<ASTType T>
struct TypedAST : AST {
    virtual ASTType type() override { return T; }
};

typedef std::list<std::unique_ptr<AST>> ASTList;
typedef unique_ptr_with_free_deleter<const char> UniquePtrString;
typedef std::unordered_map<std::string, std::unique_ptr<Expression>> StringExpressionMap;

struct StatementsAST : TypedAST<StatementsASTType> {
    std::unique_ptr<ASTList> statements;

    StatementsAST() : statements(new ASTList()) {}
    StatementsAST(ASTList* statements) : statements(statements) {}
};

struct RawBlockAST : TypedAST<RawBlockASTType> {
    UniquePtrString text;

    RawBlockAST(const char* text) : text(make_unique_ptr_with_free_deleter(text)) {}
    RawBlockAST(UniquePtrString text) : text(std::move(text)) {}
};

struct PrintBlockAST : TypedAST<PrintBlockASTType> {
    std::unique_ptr<Expression> expr;

    PrintBlockAST(Expression *expr) : expr(expr) {}
};

struct IfBlockAST : TypedAST<IfBlockASTType> {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<AST> thenBody;
    std::unique_ptr<AST> elseBody;

    IfBlockAST(Expression* condition, AST* thenBody, AST* elseBody = nullptr) : condition(condition), thenBody(thenBody), elseBody(elseBody) {}
};

struct ForBlockAST : TypedAST<ForBlockASTType> {
    std::unique_ptr<VariableReferenceExpression> keyVariable;
    std::unique_ptr<VariableReferenceExpression> valueVariable;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<AST> body;
    std::unique_ptr<AST> elseBody;

    ForBlockAST(VariableReferenceExpression* keyVariable, VariableReferenceExpression* valueVariable, Expression* iterable, AST* body, AST* elseBody) : keyVariable(keyVariable), valueVariable(valueVariable), iterable(iterable), body(body), elseBody(elseBody) {}
};

struct IncludeBlockAST : TypedAST<IncludeBlockASTType> {
    UniquePtrString includeName;
    std::unique_ptr<Expression> scope;
    StringExpressionMap variableMapping;

    IncludeBlockAST(const char* includeName) : includeName(make_unique_ptr_with_free_deleter(includeName)) {}
    IncludeBlockAST(const char* includeName, Expression* scope) : includeName(make_unique_ptr_with_free_deleter(includeName)), scope(scope) {}
    IncludeBlockAST(UniquePtrString includeName, std::unique_ptr<Expression> scope, StringExpressionMap &variableMapping) : includeName(std::move(includeName)), scope(std::move(scope)), variableMapping(std::move(variableMapping)) {}
};

} // namespace b2

#endif /* __AST_H_ */
