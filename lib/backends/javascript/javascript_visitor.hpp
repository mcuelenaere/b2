#ifndef JAVASCRIPT_VISITOR_HPP
#define JAVASCRIPT_VISITOR_HPP

#include "ast/visitors.hpp"
#include "code_emitter.hpp"

#include <iostream>
#include <unordered_map>

namespace b2 {

class JavascriptVisitor : protected Visitor<void>, protected ExpressionVisitor<void>
{
public:
    JavascriptVisitor(CodeEmitter &output) : m_output(output), m_forCounter(0), m_undefinedCheck(false) {}

	inline void performUndefinedCheck(bool undefined_check) {
		m_undefinedCheck = undefined_check;
	}

	inline bool performsUndefinedCheck() {
		return m_undefinedCheck;
	}

	void visit(AST* ast);

protected:
    virtual void statements(StatementsAST* ast) override;
    virtual void raw(RawBlockAST* ast) override;
    virtual void print_block(PrintBlockAST* ast) override;
    virtual void if_block(IfBlockAST* ast) override { this->if_block(ast, false); }
    virtual void for_block(ForBlockAST* ast) override;
    virtual void include_block(IncludeBlockAST* ast) override;

    virtual void variable_reference_expression(VariableReferenceExpression *expr) override;
    virtual void get_attribute_expression(GetAttributeExpression *expr) override;
    virtual void method_call_expression(MethodCallExpression *expr) override;
    virtual void double_literal_expression(DoubleLiteralExpression *expr) override;
    virtual void integer_literal_expression(IntegerLiteralExpression *expr) override;
    virtual void boolean_literal_expression(BooleanLiteralExpression *expr) override;
    virtual void string_literal_expression(StringLiteralExpression *expr) override;
    virtual void binary_operation_expression(BinaryOperationExpression *expr) override;
    virtual void unary_operation_expression(UnaryOperationExpression *expr) override;
    virtual void comparison_expression(ComparisonExpression *expr) override;

private:
    void if_block(IfBlockAST* ast, bool is_elseif);

	void printHeader();
	void printFooter();

	inline CodeEmitter& expression(Expression* expr) {
		this->ExpressionVisitor<void>::expression(expr);
		return m_output;
	}

	CodeEmitter& m_output;
	bool m_undefinedCheck;
	int m_forCounter;
	std::unordered_map<std::string, std::string> m_shadowValues;
};

} // namespace b2

#endif // JAVASCRIPT_VISITOR_HPP
