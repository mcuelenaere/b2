find_package(LLVM COMPONENTS jit REQUIRED)

add_definitions(${LLVM_DEFINITIONS})

add_library(backends_llvm OBJECT
    llvm_backend.cpp
    llvm_visitor.cpp
)
