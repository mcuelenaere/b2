#ifndef COALESCE_RAWBLOCKS_PASS_HPP
#define COALESCE_RAWBLOCKS_PASS_HPP

#include "ast/passes/pass.hpp"

namespace b2 {

class CoalesceRawBlocksPass : public ASTPass
{
protected:
    virtual AST* process_node(StatementsAST* ast) override;
};

} // namespace b2

#endif // COALESCE_RAWBLOCKS_PASS_HPP
