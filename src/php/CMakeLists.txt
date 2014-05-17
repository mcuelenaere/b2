find_package(LLVM COMPONENTS jit REQUIRED)
find_package(PHP REQUIRED)

add_definitions(${LLVM_DEFINITIONS})
add_definitions(${PHP_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-register")

emit_llvm_target(${CMAKE_CURRENT_SOURCE_DIR}/php_bindings_functions.c ${CMAKE_CURRENT_BINARY_DIR}/php_bindings_functions.bc "${PHP_DEFINITIONS} -O3")
bin2c_target(php_bindings_functions ${CMAKE_CURRENT_BINARY_DIR}/php_bindings_functions.bc ${CMAKE_CURRENT_BINARY_DIR}/php_bindings_functions_arr.c)

add_library(b2_php SHARED
    php_b2.cpp
    php_bindings.cpp
    php_bindings_functions.c
    ${CMAKE_CURRENT_BINARY_DIR}/php_bindings_functions_arr.c
    $<TARGET_OBJECTS:ast_passes>
    $<TARGET_OBJECTS:backends_llvm>
    $<TARGET_OBJECTS:parser>
)
target_link_libraries(b2_php
    ${LLVM_LIBS}
)

install(TARGETS b2_php DESTINATION lib)
