#include "javascript_visitor.hpp"

using namespace b2;

// TODO: move me to CodeEmitter
inline const std::string escape(const std::string& text) {
	// TODO: ensure this is correct
	std::string ret(text);
	for (auto i = 0; i < ret.size(); i++) {
		const char c = ret[i];
		switch (c) {
			case '\a':
				ret.replace(i, 1, "\\a");
				break;
			case '\b':
				ret.replace(i, 1, "\\b");
				break;
			case '\f':
				ret.replace(i, 1, "\\f");
				break;
			case '\n':
				ret.replace(i, 1, "\\n");
				break;
			case '\r':
				ret.replace(i, 1, "\\r");
				break;
			case '\t':
				ret.replace(i, 1, "\\t");
				break;
			case '\v':
				// IE < 9 treats "\v" as "v", so use "\x0B" instead
				ret.replace(i, 1, "\\x0B");
				break;
			case '\'':
				ret.replace(i, 1, "\\'");
				break;
			default:
				if ((c > 0x00 && c < 0x20) || c == 0x7F) {
					char buf[5];
					int buf_len = snprintf(buf, 5, "\\x%02d", c);
					ret.replace(i, 1, buf, buf_len);
				}
				break;
		}
	}
	return ret;
}

void JavascriptVisitor::visit(AST *ast)
{
	this->printHeader();
	this->ast(ast);
	this->printFooter();
}

void JavascriptVisitor::printHeader()
{
	m_output
		.line("function(helpers, data) {")
		.indent()
		.line("data = data || {};")
		.line("var buffer = '';")
		.blankline()
	;
}

void JavascriptVisitor::printFooter()
{
	m_output
		.blankline()
		.line("return buffer;")
		.outdent()
		.line("}")
	;
}

void JavascriptVisitor::statements(StatementsAST *ast)
{
	for (auto& statement : *ast->statements) {
		this->ast(statement.get());
	}
}

void JavascriptVisitor::raw(RawBlockAST *ast)
{
	m_output.start_line();
	m_output << "buffer += '" << escape(ast->text.get()) << "';";
	m_output.end_line();
}

void JavascriptVisitor::print_block(PrintBlockAST *ast)
{
	m_output.start_line();
	m_output << "buffer += " << this->expression(ast->expr.get());
	if (m_undefinedCheck) {
		m_output << " || ''";
	}
	m_output << ";";
	m_output.end_line();
}

void JavascriptVisitor::if_block(IfBlockAST *ast, bool is_elseif)
{
	if (!is_elseif) {
		m_output.blankline();
	}

	m_output.start_line();
	m_output << (is_elseif ? "} else " : "") << "if (" << this->expression(ast->condition.get()) << ") {";
	m_output.end_line();

	m_output.indent();
	this->ast(ast->thenBody.get());
	m_output.outdent();

	if (ast->elseBody) {
		if (ast->elseBody->type() == IfBlockASTType) {
			this->if_block(static_cast<IfBlockAST*>(ast->elseBody.get()), true);
		} else {
			m_output.line("} else {");
			
			m_output.indent();
			this->ast(ast->elseBody.get());
			m_output.outdent();

			m_output
				.line("}")
				.blankline()
			;
		}
	} else {
		m_output
			.line("}")
			.blankline()
		;
	}
}

void JavascriptVisitor::for_block(ForBlockAST *ast)
{
	m_forCounter++;

	// variables
	bool has_else_body = (ast->elseBody != nullptr);
	bool has_key_variable = (ast->keyVariable != nullptr);
	bool has_value_variable = (ast->valueVariable != nullptr);

	std::string key_variable_name = has_key_variable ? ast->keyVariable->variableName.get() : std::string();
	std::string value_variable_name = has_value_variable ? ast->valueVariable->variableName.get() : std::string();

	std::string iterable_id = "iterable_" + std::to_string(m_forCounter);
	std::string is_empty_id = "is_empty_" + std::to_string(m_forCounter);
	std::string key_id = "key_" + std::to_string(m_forCounter);
	std::string value_id = "value_" + std::to_string(m_forCounter);

	// header
	m_output.blankline();
	m_output.start_line();
	m_output << "var " << iterable_id << " = " << this->expression(ast->iterable.get()) << ";";
	m_output.end_line();

	if (has_else_body) {
		m_output.start_line();
		m_output << "var " << is_empty_id << " = true;";
		m_output.end_line();
	}

	// start of loop body
	m_output.start_line();
	m_output << "for (var " << key_id << " in " << iterable_id << ") {";
	m_output.end_line();
	m_output.indent();

	m_output.start_line();
	m_output << "if (!" << iterable_id << ".hasOwnProperty(" << key_id << ")) continue;";
	m_output.end_line();

	if (has_value_variable) {
		m_output.start_line();
		m_output << "var " << value_id << " = " << iterable_id << "[" << key_id << "];";
		m_output.end_line();
	}
	m_output.blankline();

	// cache old values
	auto oldKey = m_shadowValues.find(key_variable_name) != m_shadowValues.end() ? m_shadowValues[key_variable_name] : std::string();
	auto oldValue = m_shadowValues.find(value_variable_name) != m_shadowValues.end() ? m_shadowValues[value_variable_name] : std::string();

	// set shadow values
	if (has_key_variable) {
		m_shadowValues[key_variable_name] = key_id;
	}
	if (has_value_variable) {
		m_shadowValues[value_variable_name] = value_id;
	}

	// walk body
	this->ast(ast->body.get());

	// restore old shadow values
	if (has_key_variable) {
		if (!oldKey.empty()) {
			m_shadowValues[key_variable_name] = oldKey;
		} else {
			m_shadowValues.erase(key_variable_name);
		}
	}
	if (has_value_variable) {
		if (!oldValue.empty()) {
			m_shadowValues[value_variable_name] = oldValue;
		} else {
			m_shadowValues.erase(value_variable_name);
		}
	}

	// update is_empty variable
	if (has_else_body) {
		m_output.start_line();
		m_output  << is_empty_id << " = false;";
		m_output.end_line();
	}
	m_output.outdent();

	// end of loop body
	m_output
		.line("}")
		.blankline()
	;

	// walk else body
	if (has_else_body) {
		m_output.start_line();
		m_output << "if (" << is_empty_id << ") {";
		m_output.end_line();

		m_output.indent();
		this->ast(ast->elseBody.get());
		m_output.outdent();

		m_output
			.line("}")
			.blankline()
		;
	}
}

void JavascriptVisitor::include_block(IncludeBlockAST *ast)
{
	throw std::runtime_error("Unsupported");
}

void JavascriptVisitor::variable_reference_expression(VariableReferenceExpression *expr)
{
	std::string variableName = expr->variableName.get();

	// check shadow values
	auto shadowValue = m_shadowValues.find(variableName);
	if (shadowValue != m_shadowValues.end()) {
		m_output << shadowValue->second;
		return;
	}

	m_output << "data['" << escape(expr->variableName.get()) << "']";
}

void JavascriptVisitor::get_attribute_expression(GetAttributeExpression *expr)
{
	m_output << this->expression(expr->variable.get()) << "['" << escape(expr->attributeName.get()) << "']";
}

void JavascriptVisitor::method_call_expression(MethodCallExpression *expr)
{
	m_output << "helpers['" << expr->methodName.get() << "'](";

	bool first = true;
	for (auto &argument : *expr->arguments) {
		if (!first) {
			m_output << ", ";
		}

		m_output << this->expression(argument.get());
		first = false;
	}

	m_output << ")";
}

void JavascriptVisitor::double_literal_expression(DoubleLiteralExpression *expr)
{
	m_output << std::to_string(expr->value);
}

void JavascriptVisitor::integer_literal_expression(IntegerLiteralExpression *expr)
{
	m_output << std::to_string(expr->value);
}

void JavascriptVisitor::boolean_literal_expression(BooleanLiteralExpression *expr)
{
	m_output << (expr->value ? "true" : "false");
}

void JavascriptVisitor::string_literal_expression(StringLiteralExpression *expr)
{
	m_output << "'" << escape(expr->value.get()) << "'";
}

void JavascriptVisitor::binary_operation_expression(BinaryOperationExpression *expr)
{
	std::string op = {(char) (expr->op >> 8), (char) (expr->op & 0xFF)};

	m_output << this->expression(expr->left.get()) << " " << op << " " << this->expression(expr->right.get());
}

void JavascriptVisitor::unary_operation_expression(UnaryOperationExpression *expr)
{
	std::string op = {(char) expr->op};

	m_output << op << this->expression(expr->expr.get());
}

void JavascriptVisitor::comparison_expression(ComparisonExpression *expr)
{
	std::string op = {(char) (expr->op >> 8), (char) (expr->op & 0xFF)};

	m_output << this->expression(expr->left.get()) << " " << op << " " << this->expression(expr->right.get());
}
