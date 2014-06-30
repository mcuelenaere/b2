#include "print_visitor.hpp"

#include <map>

using namespace b2;

void PrintVisitor::visit(AST* ast)
{
	m_output << indentation() << "[SOF]" << std::endl;
    m_indentation++;
    this->ast(ast);
    m_indentation--;
	m_output << indentation() << "[EOF]" << std::endl;
}

void PrintVisitor::statements(StatementsAST* ast)
{
	m_output << indentation() << "[STATEMENTS]" << std::endl;
    m_indentation++;
    for (std::unique_ptr<AST> &statement : *ast->statements) {
        this->ast(statement.get());
    }
    m_indentation--;
	m_output << indentation() << "[END_STATEMENTS]" << std::endl;
}

void PrintVisitor::raw(RawBlockAST* ast)
{
	m_output << indentation() << "[RAW] \"" << escape(ast->text.get()) << "\"" << std::endl;
}

void PrintVisitor::print_block(PrintBlockAST* ast)
{
	m_output << indentation() << "[PRINT_BLOCK ";
    this->expression(ast->expr.get());
	m_output << "]" << std::endl;
}

void PrintVisitor::if_block(IfBlockAST* ast)
{
	m_output << indentation() << "[IF_BLOCK ";
    this->expression(ast->condition.get());
	m_output << "]" << std::endl;

    if (ast->thenBody) {
        m_indentation++;
        this->ast(ast->thenBody.get());
        m_indentation--;
    }
    if (ast->elseBody) {
		m_output << indentation() << "[ELSE_BLOCK]" << std::endl;
        m_indentation++;
        this->ast(ast->elseBody.get());
        m_indentation--;
    }

	m_output << indentation() << "[ENDIF_BLOCK]" << std::endl;
}

void PrintVisitor::for_block(ForBlockAST* ast)
{
	m_output << indentation() << "[FOR_BLOCK ";
    if (ast->keyVariable) {
		m_output << "keyVariable=";
        this->expression(ast->keyVariable.get());
		m_output << " ";
    }
    if (ast->valueVariable) {
		m_output << "valueVariable=";
        this->expression(ast->valueVariable.get());
		m_output << " ";
    }
    if (ast->iterable) {
		m_output << "iterable=";
        this->expression(ast->iterable.get());
    }
	m_output << "]" << std::endl;

    if (ast->body) {
        m_indentation++;
        this->ast(ast->body.get());
        m_indentation--;
    }
    if (ast->elseBody) {
		m_output << indentation() << "[ELSEFOR_BLOCK]" << std::endl;
        m_indentation++;
        this->ast(ast->elseBody.get());
        m_indentation--;
    }

	m_output << indentation() << "[ENDFOR_BLOCK]" << std::endl;
}

void PrintVisitor::include_block(IncludeBlockAST *ast)
{
	m_output << indentation() << "[INCLUDE_BLOCK includeName=\"" << ast->includeName.get() << "\"";
    if (ast->scope) {
		m_output << " scope=";
        this->expression(ast->scope.get());
    }
    if (ast->variableMapping.size() > 0) {
		std::map<std::string, Expression*> sortedMapping;
		for (auto &it : ast->variableMapping) {
			sortedMapping[it.first] = it.second.get();
		}

		m_output << " variableMapping={";
        for (auto iter = sortedMapping.begin(); iter != sortedMapping.end(); ++iter) {
            if (iter != sortedMapping.begin()) {
				m_output << ", ";
            }

			m_output << "\"" << iter->first << " => ";
            this->expression(iter->second);
        }
		m_output << "}";
    }
	m_output << "]" << std::endl;
}

void PrintVisitor::variable_reference_expression(VariableReferenceExpression *expr)
{
	m_output << "{VARIABLE name=\"" << expr->variableName.get() << "\"}";
}

void PrintVisitor::get_attribute_expression(GetAttributeExpression *expr)
{
	m_output << "{GET_ATTRIBUTE variable=";
    this->expression(expr->variable.get());
	m_output << " attributeName=\"" << expr->attributeName.get() << "\"}";
}

void PrintVisitor::method_call_expression(MethodCallExpression *expr)
{
	m_output << "{METHOD_CALL name=\"" << expr->methodName.get() << "\", args=[";
    auto arguments = expr->arguments.get();
    for (auto iter = arguments->begin(); iter != arguments->end(); ++iter) {
        if (iter != arguments->begin()) {
			m_output << ", ";
        }
        this->expression(iter->get());
    }
	m_output << "]}";
}

void PrintVisitor::double_literal_expression(DoubleLiteralExpression *expr)
{
	m_output << "{DOUBLE value=" << expr->value << "}";
}

void PrintVisitor::integer_literal_expression(IntegerLiteralExpression *expr)
{
	m_output << "{INT value=" << expr->value << "}";
}

void PrintVisitor::boolean_literal_expression(BooleanLiteralExpression *expr)
{
	m_output << "{BOOL value=" << (expr->value ? "true" : "false") << "}";
}

void PrintVisitor::string_literal_expression(StringLiteralExpression *expr)
{
	m_output << "{STRING value=\"" << escape(expr->value.get()) << "\"}";
}

void PrintVisitor::binary_operation_expression(BinaryOperationExpression *expr)
{
	m_output << "{BINOP left=";
    this->expression(expr->left.get());
	m_output << " right=";
    this->expression(expr->right.get());
	m_output << " op='" << (char) expr->op << "'}";
}

void PrintVisitor::unary_operation_expression(UnaryOperationExpression *expr)
{
	m_output << "{UNOP expr=";
    this->expression(expr->expr.get());
	m_output << " op='" << (char) expr->op << "'}";
}

void PrintVisitor::comparison_expression(ComparisonExpression *expr)
{
	m_output << "{CMP left=";
    this->expression(expr->left.get());
	m_output << " right=";
    this->expression(expr->right.get());
	m_output << " op=\"";
	if (expr->op >> 8) {
		m_output << (char) (expr->op >> 8);
	}
	if (expr->op & 0xFF) {
		m_output << (char) (expr->op & 0xFF);
	}
	m_output << "\"}";
}
