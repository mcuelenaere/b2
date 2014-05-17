find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

bison_target(tokenizer syntax.y ${CMAKE_CURRENT_BINARY_DIR}/tokenizer.cpp)
flex_target(lexer syntax.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp COMPILE_FLAGS "-Psyntax_")
add_flex_bison_dependency(lexer tokenizer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-register")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(parser OBJECT
    parser.cpp
    ${BISON_tokenizer_OUTPUTS}
    ${FLEX_lexer_OUTPUTS}
)
