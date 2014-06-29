#include "backends/llvm/llvm_backend.hpp"
#include "backends/llvm/llvm_bindings.h"
#include "backends/llvm/llvm_visitor.hpp"

#include <mutex>
#include <unordered_map>
#include <vector>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Linker.h>
#include <llvm/Analysis/Verifier.h>

// passes
#include <llvm/Analysis/Passes.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>

using namespace b2;

static std::once_flag llvmInitialized;

LLVMBackend::LLVMBackend(llvm::IRBuilder<> &irBuilder, LLVMBindings &bindings) :
	m_irBuilder(irBuilder),
	m_bindings(bindings),
	m_visitor(nullptr),
	m_engine(nullptr)
{
	// TODO: should the module name be unique?
	m_module = new llvm::Module("template", m_irBuilder.getContext());

	// create visitor
	m_visitor.reset(new LLVMVisitor(m_irBuilder, m_module, m_bindings));

	// setup execution engine preconditions
	std::call_once(llvmInitialized, []() {
		LLVMLinkInJIT();
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
	});

	llvm::TargetOptions targetOptions;
	targetOptions.JITEmitDebugInfo = true;
	targetOptions.NoFramePointerElim = true;
	
	// create execution engine
	std::string error;
	m_engine.reset(
       llvm::EngineBuilder(m_module)
       .setErrorStr(&error)
       .setEngineKind(llvm::EngineKind::JIT)
       .setTargetOptions(targetOptions)
       //.setUseMCJIT(true)
       .create()
	);
	if (!m_engine) {
		delete m_module;
		throw std::runtime_error("Error occured during initing JIT engine: " + error);
	}
}

void* LLVMBackend::createFunction(const std::string &name, AST *ast)
{
    auto llvmFunc = m_functions[name];

    if (llvmFunc == nullptr) {
        // create LLVM IR
        llvmFunc = m_visitor->visit(ast);
        //llvmFunc->dump();

        auto module = llvmFunc->getParent();
    #if 0
        std::string error;
        llvm::raw_fd_ostream strm("output.bc", error);
        llvm::WriteBitcodeToFile(module, strm);
    #endif

        // verify IR
        if (llvm::verifyFunction(*llvmFunc, llvm::PrintMessageAction) || llvm::verifyModule(*module, llvm::PrintMessageAction)) {
            throw std::runtime_error("Error occured during verification of IR!");
        }

        // optimize IR
        llvm::legacy::FunctionPassManager p(module);
        p.add(llvm::createBasicAliasAnalysisPass());
        p.add(llvm::createInstructionCombiningPass());
        p.add(llvm::createReassociatePass());
        p.add(llvm::createGVNPass());
        p.add(llvm::createCFGSimplificationPass());
        //p.add(llvm::createFunctionInliningPass());
        //p.add(llvm::createAlwaysInlinerPass());

        p.doInitialization();
        p.run(*llvmFunc);
        p.doFinalization();

        //llvmFunc->dump();

        m_functions[name] = llvmFunc;
    }

    // create machine code
    return m_engine->getPointerToFunction(llvmFunc);
}
