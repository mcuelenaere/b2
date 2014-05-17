#include "convert_literal_printblock_to_rawblock_pass.hpp"

#include <string.h>

using namespace b2;

template<typename T>
static RawBlockAST* createFormattedRawBlock(T value)
{
	std::stringstream ss;
	ss << value;

	const char* str = strdup(ss.str().c_str());
    if (str == nullptr) {
        throw std::bad_alloc();
    }

    return new RawBlockAST(str);
}

AST* ConvertLiteralPrintBlockToRawBlockPass::process_node(PrintBlockAST* ast)
{
    auto expr = ast->expr.get();
    switch (expr->type()) {
        case DoubleLiteralExpressionType: {
            auto literalExpr = static_cast<DoubleLiteralExpression*>(expr);
            return createFormattedRawBlock(literalExpr->value);
        }
        case IntegerLiteralExpressionType: {
            auto literalExpr = static_cast<IntegerLiteralExpression*>(expr);
            return createFormattedRawBlock(literalExpr->value);
        }
        case BooleanLiteralExpressionType: {
            auto literalExpr = static_cast<BooleanLiteralExpression*>(expr);
            return createFormattedRawBlock(literalExpr->value ? "true" : "false");
        }
        case StringLiteralExpressionType: {
            auto literalExpr = static_cast<StringLiteralExpression*>(expr);
            return new RawBlockAST(std::move(literalExpr->value));
        }
        default:
            return ast;
    }
}
