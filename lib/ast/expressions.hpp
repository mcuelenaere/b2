#ifndef __EXPRESSIONS_H_
#define __EXPRESSIONS_H_

#include <cstring>
#include <cstdlib>
#include <list>
#include <memory>
#include "utils.hpp"

namespace b2 {

enum ExpressionType {
    VariableReferenceExpressionType,
    GetAttributeExpressionType,
    MethodCallExpressionType,
    DoubleLiteralExpressionType,
    IntegerLiteralExpressionType,
    BooleanLiteralExpressionType,
    StringLiteralExpressionType,
    BinaryOperationExpressionType,
    UnaryOperationExpressionType,
    ComparisonExpressionType,
};

enum ExpressionValueType {
    DoubleType,
    IntegerType,
    BooleanType,
    StringType,
    VariantType,
};

struct Expression {
    virtual ~Expression() {}
    virtual Expression* clone() = 0;
    virtual enum ExpressionType type() = 0;
    virtual enum ExpressionValueType valueType() = 0;
};

template<ExpressionType T, ExpressionValueType V = VariantType>
struct TypedExpression : Expression {
    virtual enum ExpressionType type() override {
        return T;
    }

    virtual enum ExpressionValueType valueType() override {
        return V;
    }
};

struct VariableReferenceExpression : TypedExpression<VariableReferenceExpressionType, VariantType> {
    unique_ptr_with_free_deleter<const char> variableName;

    VariableReferenceExpression(const char* variableName) : variableName(make_unique_ptr_with_free_deleter(variableName)) {}
    virtual Expression* clone() override {
        return new VariableReferenceExpression(strdup(variableName.get()));
    }
};

struct GetAttributeExpression : TypedExpression<GetAttributeExpressionType, VariantType> {
    std::unique_ptr<Expression> variable;
    unique_ptr_with_free_deleter<const char> attributeName;

    GetAttributeExpression(Expression* variable, const char* attributeName) : variable(variable), attributeName(make_unique_ptr_with_free_deleter(attributeName)) {}
    virtual Expression* clone() override {
        return new GetAttributeExpression(variable->clone(), strdup(attributeName.get()));
    }
};

typedef std::list<std::unique_ptr<Expression>> ExpressionList;

static inline ExpressionList* cloneExpressionList(ExpressionList* exprList)
{
    ExpressionList* clonedExprList = new ExpressionList(exprList->size());
    for (auto &expr : *exprList) {
        clonedExprList->push_back(std::move(std::unique_ptr<Expression>(expr->clone())));
    }
    return clonedExprList;
}

struct MethodCallExpression : TypedExpression<MethodCallExpressionType, VariantType> {
    unique_ptr_with_free_deleter<const char> methodName;
    std::unique_ptr<ExpressionList> arguments;

    MethodCallExpression(const char* methodName, ExpressionList* arguments) : methodName(make_unique_ptr_with_free_deleter(methodName)), arguments(arguments) {}
    virtual Expression* clone() override {
        return new MethodCallExpression(strdup(methodName.get()), cloneExpressionList(arguments.get()));
    }
};

template<typename T, ExpressionType exprType, ExpressionValueType valueType>
struct LiteralExpression : TypedExpression<exprType, valueType> {
    T value;

    LiteralExpression(T value) : value(value) {}
    virtual Expression* clone() override {
        return new LiteralExpression<T, exprType, valueType>(value);
    }
};

using DoubleLiteralExpression = LiteralExpression<double, DoubleLiteralExpressionType, DoubleType>;
using IntegerLiteralExpression = LiteralExpression<long, IntegerLiteralExpressionType, IntegerType>;
using BooleanLiteralExpression = LiteralExpression<bool, BooleanLiteralExpressionType, BooleanType>;

struct StringLiteralExpression : TypedExpression<StringLiteralExpressionType, StringType> {
    unique_ptr_with_free_deleter<const char> value;

    StringLiteralExpression(const char *v) : value(make_unique_ptr_with_free_deleter(v)) {}
    virtual Expression* clone() override {
        return new StringLiteralExpression(strdup(value.get()));
    }
};

enum BinaryOperation {
    Addition = '+',
    Subtraction = '-',
    Multiplication = '*',
    Division = '/',
    Modulus = '%',
};

struct BinaryOperationExpression : TypedExpression<BinaryOperationExpressionType> {
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    const BinaryOperation op;

    BinaryOperationExpression(Expression* left, Expression* right, BinaryOperation op) : left(left), right(right), op(op) {}
    BinaryOperationExpression(Expression* left, Expression* right, const char op) : left(left), right(right), op((BinaryOperation)op) {}
    virtual Expression* clone() override {
        return new BinaryOperationExpression(left->clone(), right->clone(), op);
    }

    virtual enum ExpressionValueType valueType() override {
        auto left_vtype = left->valueType();
        auto right_vtype = right->valueType();

        if (left_vtype == VariantType || right_vtype == VariantType) {
            return VariantType;
        } else if (left_vtype == DoubleType || right_vtype == DoubleType) {
            return DoubleType;
        } else if (left_vtype == IntegerType && right_vtype == IntegerType) {
            return IntegerType;
        } else {
            // note: this isn't entirely correct, a string could also be converted to an integer
            return DoubleType;
        }
    }
};

enum UnaryOperation {
    NumericPositive = '+',
    NumericNegative = '-',
    BooleanNegation = '!',
};

struct UnaryOperationExpression : TypedExpression<UnaryOperationExpressionType> {
    std::unique_ptr<Expression> expr;
    const UnaryOperation op;

    UnaryOperationExpression(Expression* expr, UnaryOperation op) : expr(expr), op(op) {}
    UnaryOperationExpression(Expression* expr, const char op) : expr(expr), op((UnaryOperation) op) {}
    virtual Expression* clone() override {
        return new UnaryOperationExpression(expr->clone(), op);
    }

    virtual enum ExpressionValueType valueType() override {
        switch (op) {
            case BooleanNegation:
                return BooleanType;
            case NumericPositive:
            case NumericNegative:
                auto type = expr->valueType();
                switch (type) {
                    case VariantType:
                    case DoubleType:
                    case IntegerType:
                        return type;
                    default:
                        // note: this isn't entirely correct, a string could also be converted to an integer
                        return DoubleType;
                }
        }
    }
};

#define PACK_UINT16(a,b) ((a << 8) | b)
enum ComparisonOperation {
	Equal = PACK_UINT16('=', '='),
	NotEqual = PACK_UINT16('!', '='),
	GreaterThan = PACK_UINT16('>', '\0'),
	GreaterOrEqual = PACK_UINT16('>', '='),
	LessThan = PACK_UINT16('<', '\0'),
	LessOrEqual = PACK_UINT16('<', '='),
	Or = PACK_UINT16('|', '|'),
	And = PACK_UINT16('&', '&'),
};
#undef PACK_UINT16

struct ComparisonExpression : TypedExpression<ComparisonExpressionType, BooleanType> {
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    const ComparisonOperation op;

    ComparisonExpression(Expression* left, Expression* right, ComparisonOperation op) : left(left), right(right), op(op) {}
    ComparisonExpression(Expression* left, Expression* right, const char* op) : left(left), right(right), op((ComparisonOperation)(op[0] << 8 | op[1])) {}
    virtual Expression* clone() override {
        return new ComparisonExpression(left->clone(), right->clone(), op);
    }
};

} // namespace b2

#endif /* __EXPRESSIONS_H_ */
