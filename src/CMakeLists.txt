include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/lib")

if(WITH_PHP_BINDINGS)
	add_subdirectory(php)
endif()

#add_subdirectory(simple)
add_subdirectory(ast_print)
add_subdirectory(js_precompiler)
