add_executable(ast_print
    print.cpp
    $<TARGET_OBJECTS:ast_passes>
    $<TARGET_OBJECTS:parser>
    $<TARGET_OBJECTS:utils>
)
