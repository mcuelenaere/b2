add_library(ast_passes OBJECT
    coalesce_rawblocks_pass.cpp
    convert_literal_printblock_to_rawblock_pass.cpp
    fold_constant_expressions_pass.cpp
    pass_manager.cpp
    resolve_includes_pass.cpp
)
