include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/lib")

add_subdirectory(php)
#add_subdirectory(simple)
add_subdirectory(ast_print)
add_subdirectory(js_precompiler)
