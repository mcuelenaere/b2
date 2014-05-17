#ifndef CONVERT_LITERAL_PRINTBLOCK_TO_RAWBLOCK_PASS_HPP
#define CONVERT_LITERAL_PRINTBLOCK_TO_RAWBLOCK_PASS_HPP

#include "ast/passes/pass.hpp"

namespace b2 {

class ConvertLiteralPrintBlockToRawBlockPass : public ASTPass
{
protected:
    virtual AST* process_node(PrintBlockAST* ast) override;
};

} // namespace b2

#endif // CONVERT_LITERAL_PRINTBLOCK_TO_RAWBLOCK_PASS_HPP
