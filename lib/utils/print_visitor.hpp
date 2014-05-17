#ifndef __PRINT_VISITOR_H_
#define __PRINT_VISITOR_H_

#include "ast/visitors.hpp"

#include <iostream>
#include <ostream>

namespace b2 {

class PrintVisitor : private Visitor<void>, private ExpressionVisitor<void> {
public:
    PrintVisitor(std::ostream &output = std::cout) : m_indentation(0), m_output(output) {}

    void visit(AST* ast);

private:
    virtual void statements(StatementsAST* ast) override;
    virtual void raw(RawBlockAST* ast) override;
    virtual void print_block(PrintBlockAST* ast) override;
    virtual void if_block(IfBlockAST* ast) override;
    virtual void for_block(ForBlockAST* ast) override;
    virtual void include_block(IncludeBlockAST* ast) override;

    virtual void variable_reference_expression(VariableReferenceExpression *expr) override;
    virtual void get_attribute_expression(GetAttributeExpression* expr) override;
    virtual void method_call_expression(MethodCallExpression *expr) override;
    virtual void double_literal_expression(DoubleLiteralExpression *expr) override;
    virtual void integer_literal_expression(IntegerLiteralExpression *expr) override;
    virtual void boolean_literal_expression(BooleanLiteralExpression *expr) override;
    virtual void string_literal_expression(StringLiteralExpression *expr) override;
    virtual void binary_operation_expression(BinaryOperationExpression *expr) override;
    virtual void unary_operation_expression(UnaryOperationExpression *expr) override;
    virtual void comparison_expression(ComparisonExpression *expr) override;

	inline const std::string indentation() {
		return std::string(m_indentation, '\t');
	}

	inline std::string escape(const char* raw) {
		std::string text(raw);
		return escape(text);
	}

	inline const std::string& escape(std::string& raw) {
		for (auto i = 0; i < raw.size(); i++) {
			const char c = raw[i];
			switch (c) {
				case '\a':
					raw.replace(i, 1, "\\a");
					break;
				case '\b':
					raw.replace(i, 1, "\\b");
					break;
				case '\f':
					raw.replace(i, 1, "\\f");
					break;
				case '\n':
					raw.replace(i, 1, "\\n");
					break;
				case '\r':
					raw.replace(i, 1, "\\r");
					break;
				case '\t':
					raw.replace(i, 1, "\\t");
					break;
				case '\v':
					raw.replace(i, 1, "\\v");
					break;
				default:
					if ((c > 0x00 && c < 0x20) || c == 0x7F) {
						char buf[5];
						int buf_len = snprintf(buf, 5, "\\x%02d", c);
						raw.replace(i, 1, buf, buf_len);
					}
					break;
			}
		}
		return raw;
	}

    int m_indentation;
	std::ostream &m_output;
};

} // namespace b2

#endif /* __PRINT_VISITOR_H_ */
