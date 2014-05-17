#ifndef __FOLD_CONSTANT_EXPRESSIONS_PASS_HPP_
#define __FOLD_CONSTANT_EXPRESSIONS_PASS_HPP_

#include "ast/passes/pass.hpp"

namespace b2 {

class FoldConstantExpressionsPass : public ExpressionPass
{
protected:
    virtual Expression* process_node(BinaryOperationExpression *expr) override;
    virtual Expression* process_node(UnaryOperationExpression *expr) override;
    virtual Expression* process_node(ComparisonExpression *expr) override;
};

} // namespace b2

#endif /* __FOLD_CONSTANT_EXPRESSIONS_PASS_HPP_ */
