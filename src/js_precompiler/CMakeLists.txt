add_executable(b2-js-precompiler
    compiler.cpp
    $<TARGET_OBJECTS:ast_passes>
    $<TARGET_OBJECTS:backends_javascript>
    $<TARGET_OBJECTS:parser>
)

install(TARGETS b2-js-precompiler DESTINATION bin)
