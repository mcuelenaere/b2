#include "coalesce_rawblocks_pass.hpp"

#include <vector>
#include <iostream>

using namespace b2;

static const char* concat(std::vector<const char*> strings)
{
    size_t length = 0;

    // calculate total string size
    for (auto str : strings) {
        length += strlen(str);
    }

    // alloc new string
    const char* new_str = (const char*) malloc(length + 1);
    if (new_str == nullptr) {
        throw std::bad_alloc();
    }

    // copy strings into new string
    char* ptr = (char*) new_str;
    for (auto str : strings) {
        size_t str_len = strlen(str);
        memcpy(ptr, str, str_len);
        ptr += str_len;
    }

    // terminate new string
    *ptr = '\0';

    return new_str;
}

AST* CoalesceRawBlocksPass::process_node(StatementsAST* ast)
{
    auto statements_ast = static_cast<StatementsAST*>(ast);
    auto statements = statements_ast->statements.get();

    for (auto iter = statements->begin(); iter != statements->end(); ++iter) {
        auto statement = iter->get();

        if (statement->type() == RawBlockASTType) {
            // merge this and all following RawBlockASTs into one
            RawBlockAST* raw_block = static_cast<RawBlockAST*>(statement);
            std::vector<const char*> strings;

            // gather all strings of this raw block and all its succeeding raw blocks
            auto iter2 = iter;
            for (; iter2 != statements->end(); ++iter2) {
                if (iter2->get()->type() != RawBlockASTType) {
                    break;
                }

                RawBlockAST* raw_block2 = static_cast<RawBlockAST*>(iter2->get());
                strings.push_back(raw_block2->text.get());
            }

            if (strings.size() > 1) {
                // set this raw block to the concatenation of all strings
                raw_block->text.reset(concat(strings));

                // delete all succeeding raw blocks
                iter2 = iter;
                ++iter2;
                while (iter2 != statements->end()) {
                    if (iter2->get()->type() != RawBlockASTType) {
                        break;
                    }

                    iter2 = statements->erase(iter2);
                }
            }
        }
    }

    return ast;
}
