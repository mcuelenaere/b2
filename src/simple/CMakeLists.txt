find_package(LLVM COMPONENTS jit REQUIRED)

add_definitions(${LLVM_DEFINITIONS})

emit_llvm_target(${CMAKE_CURRENT_SOURCE_DIR}/simple_bindings_functions.c ${CMAKE_CURRENT_BINARY_DIR}/simple_bindings_functions.bc "-O3")
bin2c_target(simple_bindings_functions ${CMAKE_CURRENT_BINARY_DIR}/simple_bindings_functions.bc ${CMAKE_CURRENT_BINARY_DIR}/simple_bindings_functions_arr.c)

add_executable(test
    simple_bindings.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/simple_bindings_functions_arr.c
)
target_link_libraries(test
    ast_passes
    backends_llvm
    parser
    ${LLVM_LIBS}
)
