#ifndef PASS_MANAGER_HPP
#define PASS_MANAGER_HPP

#include "ast/passes/pass.hpp"

#include <memory>
#include <vector>

namespace b2 {

class PassManager
{
public:
    void addPass(ASTPass* pass);
    void addPass(ExpressionPass* pass);
    void removeAllPasses();

    /*
     * Runs the registered passes over the given AST.
     *
     * Calling this passes the ownership of the AST to PassManager.
     * The returned AST is no longer owned by PassManager (and could
     * or could not be the given AST).
     */
    AST* run(AST* ast);

private:
    std::vector<std::unique_ptr<ASTPass>> m_passes;
};

} // namespace b2

#endif // PASS_MANAGER_HPP
