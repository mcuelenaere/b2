#include "fold_constant_expressions_pass.hpp"

using namespace b2;

static bool isLiteralExpression(Expression* expr)
{
    switch (expr->type()) {
        case DoubleLiteralExpressionType:
        case IntegerLiteralExpressionType:
        case BooleanLiteralExpressionType:
        case StringLiteralExpressionType:
            return true;
        default:
            return false;
    }
}

static bool isNumericLiteralExpression(Expression* expr)
{
    ExpressionType type = expr->type();
    return type == DoubleLiteralExpressionType || type == IntegerLiteralExpressionType;
}

static bool isBooleanLiteralExpression(Expression* expr)
{
    return expr->type() == BooleanLiteralExpressionType;
}

static bool isStringLiteralExpression(Expression* expr)
{
    return expr->type() == StringLiteralExpressionType;
}

#define NUMERIC_VALUE(x) (x->type() == IntegerLiteralExpressionType ? static_cast<IntegerLiteralExpression*>(x)->value : static_cast<DoubleLiteralExpression*>(x)->value)

Expression* FoldConstantExpressionsPass::process_node(BinaryOperationExpression *expression)
{
    auto left = expression->left.get();
    auto right = expression->right.get();

    if (!isNumericLiteralExpression(left)) {
        // try folding the left expression
        Expression* new_left = this->process(left);
        if (new_left != left) {
            expression->left.reset(new_left);
            left = new_left;
        }
    }

    if (!isNumericLiteralExpression(right)) {
        // try folding the right expression
        Expression* new_right = this->process(right);
        if (new_right != right) {
            expression->right.reset(new_right);
            right = new_right;
        }
    }

    if (!isNumericLiteralExpression(left) || !isNumericLiteralExpression(right)) {
        // we can't fold this expression, not all parts are constants
        return expression;
    }

    // fold this expression
    if (left->type() == DoubleLiteralExpressionType || right->type() == DoubleLiteralExpressionType) {
        // either one of the operands is double, so the result should be double
        switch (expression->op) {
            case Addition:
                return new DoubleLiteralExpression(NUMERIC_VALUE(left) + NUMERIC_VALUE(right));
            case Subtraction:
                return new DoubleLiteralExpression(NUMERIC_VALUE(left) - NUMERIC_VALUE(right));
            case Multiplication:
                return new DoubleLiteralExpression(NUMERIC_VALUE(left) * NUMERIC_VALUE(right));
            case Division:
                // TODO: check for 0 values
                return new DoubleLiteralExpression(NUMERIC_VALUE(left) / NUMERIC_VALUE(right));
            case Modulus:
                // TODO: should an exception be thrown?
                return expression;
        }
    } else {
        // both the operands are integer, so the result should also be integer
        auto leftValue = static_cast<IntegerLiteralExpression*>(left)->value;
        auto rightValue = static_cast<IntegerLiteralExpression*>(right)->value;

        switch (expression->op) {
            case Addition:
                return new IntegerLiteralExpression(leftValue + rightValue);
            case Subtraction:
                return new IntegerLiteralExpression(leftValue - rightValue);
            case Multiplication:
                return new IntegerLiteralExpression(leftValue * rightValue);
            case Division:
				if (rightValue == 0) {
					throw std::runtime_error("Division by zero");
				}
                return new IntegerLiteralExpression(leftValue / rightValue);
            case Modulus:
				if (rightValue == 0) {
					throw std::runtime_error("Modulo by zero");
				}
                return new IntegerLiteralExpression(leftValue % rightValue);
        }
    }
}

Expression* FoldConstantExpressionsPass::process_node(UnaryOperationExpression *expression)
{
    auto sub_expr = expression->expr.get();

    switch (expression->op) {
        case NumericPositive:
        case NumericNegative: {
            if (!isNumericLiteralExpression(sub_expr)) {
                // try folding sub_expr
                Expression* foldedExpression = this->process(sub_expr);
                if (foldedExpression != sub_expr) {
                    expression->expr.reset(foldedExpression);
                    sub_expr = foldedExpression;
                }
            }

            int multiplier = (expression->op == NumericNegative ? -1 : 1);
            if (sub_expr->type() == DoubleLiteralExpressionType) {
                auto oldValue = static_cast<DoubleLiteralExpression*>(sub_expr)->value;
                return new DoubleLiteralExpression(oldValue * multiplier);
            } else if (sub_expr->type() == IntegerLiteralExpressionType) {
                auto oldValue = static_cast<IntegerLiteralExpression*>(sub_expr)->value;
                return new IntegerLiteralExpression(oldValue * multiplier);
            }
            break;
        }
        case BooleanNegation: {
            if (!isBooleanLiteralExpression(sub_expr)) {
                // try folding sub_expr
                Expression* foldedExpression = this->process(sub_expr);
                if (foldedExpression != sub_expr) {
                    expression->expr.reset(foldedExpression);
                    sub_expr = foldedExpression;
                }
            }

            if (isBooleanLiteralExpression(sub_expr)) {
                auto oldValue = static_cast<BooleanLiteralExpression*>(sub_expr)->value;
                return new BooleanLiteralExpression(!oldValue);
            }
            break;
        }
    }

    return expression;
}

Expression* FoldConstantExpressionsPass::process_node(ComparisonExpression *expression)
{
    auto left = expression->left.get();
    auto right = expression->right.get();

    if (!isLiteralExpression(left)) {
        // try folding the left expression
        Expression* new_left = this->process(left);
        if (new_left != left) {
            expression->left.reset(new_left);
            left = new_left;
        }
    }

    if (!isLiteralExpression(right)) {
        // try folding the right expression
        Expression* new_right = this->process(right);
        if (new_right != right) {
            expression->right.reset(new_right);
            right = new_right;
        }
    }

    if (!isLiteralExpression(left) || !isLiteralExpression(right)) {
        // we can't fold this expression, not all parts are constants
        return expression;
    }

    switch (expression->op) {
        case Equal:
        case NotEqual:
            if (isNumericLiteralExpression(left) && isNumericLiteralExpression(right)) {
                // numerical comparison
                switch (expression->op) {
                    case Equal: return new BooleanLiteralExpression(NUMERIC_VALUE(left) == NUMERIC_VALUE(right));
                    case NotEqual: return new BooleanLiteralExpression(NUMERIC_VALUE(left) != NUMERIC_VALUE(right));
                    default: break;
                }
            } else if (isBooleanLiteralExpression(left) && isBooleanLiteralExpression(right)) {
                // boolean comparison
                auto left_bool = static_cast<BooleanLiteralExpression*>(left)->value;
                auto right_bool = static_cast<BooleanLiteralExpression*>(right)->value;

                switch (expression->op) {
                    case Equal: return new BooleanLiteralExpression(left_bool == right_bool);
                    case NotEqual: return new BooleanLiteralExpression(left_bool != right_bool);
                    default: break;
                }
            } else if (isStringLiteralExpression(left) && isStringLiteralExpression(right)) {
                // string comparison
                auto left_string = static_cast<StringLiteralExpression*>(left)->value.get();
                auto right_string = static_cast<StringLiteralExpression*>(right)->value.get();

                switch (expression->op) {
                    case Equal: return new BooleanLiteralExpression(strcmp(left_string, right_string) == 0);
                    case NotEqual: return new BooleanLiteralExpression(strcmp(left_string, right_string) != 0);
                    default: break;
                }
            }
            break;

        case GreaterThan:
        case GreaterOrEqual:
        case LessThan:
        case LessOrEqual:
            if (!isNumericLiteralExpression(left) && !isNumericLiteralExpression(right)) {
                // these comparison operations only supports numeric values
                // TODO: throw error?
                break;
            }

            switch (expression->op) {
                case GreaterThan: return new BooleanLiteralExpression(NUMERIC_VALUE(left) > NUMERIC_VALUE(right));
                case GreaterOrEqual: return new BooleanLiteralExpression(NUMERIC_VALUE(left) >= NUMERIC_VALUE(right));
                case LessThan: return new BooleanLiteralExpression(NUMERIC_VALUE(left) < NUMERIC_VALUE(right));
                case LessOrEqual: return new BooleanLiteralExpression(NUMERIC_VALUE(left) <= NUMERIC_VALUE(right));
                default: break;
            }
            break;

        case Or:
        case And: {
            if (!isBooleanLiteralExpression(left) && !isBooleanLiteralExpression(right)) {
                // "&&" and "||" only supports boolean values
                // TODO: throw error?
                break;
            }

            auto left_bool = static_cast<BooleanLiteralExpression*>(left);
            auto right_bool = static_cast<BooleanLiteralExpression*>(right);

            switch (expression->op) {
                case Or: return new BooleanLiteralExpression(left_bool->value || right_bool->value);
                case And: return new BooleanLiteralExpression(left_bool->value && right_bool->value);
                default: break;
            }
            break;
        }
    }

    return expression;
}
