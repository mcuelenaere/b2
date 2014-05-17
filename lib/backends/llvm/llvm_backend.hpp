#ifndef __LLVM_BACKEND_HPP_
#define __LLVM_BACKEND_HPP_

#include "ast/ast.hpp"
#include "backends/llvm/llvm_bindings.h"
#include "backends/llvm/llvm_visitor.hpp"

#include <memory>
#include <string>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

namespace b2 {

class LLVMBackend
{
public:
    LLVMBackend(llvm::IRBuilder<> &irBuilder, LLVMBindings &bindings);

    void* createFunction(const std::string &name, AST* ast);
	llvm::Module* getModule() { return m_module; }
protected:
	llvm::IRBuilder<> &m_irBuilder;
	LLVMBindings &m_bindings;
	llvm::Module* m_module;
	std::unique_ptr<LLVMVisitor> m_visitor;
	std::unique_ptr<llvm::ExecutionEngine> m_engine;
	std::unordered_map<std::string, llvm::Function*> m_functions;
};

} // namespace b2

#endif // __LLVM_BACKEND_HPP_
