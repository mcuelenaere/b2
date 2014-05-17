#include <cstring>
#include <iostream>
#include <memory>

#include <libgen.h>
#include <getopt.h>

#include "parser/parser.hpp"
#include "backends/javascript/javascript_visitor.hpp"

#include "ast/passes/coalesce_rawblocks_pass.hpp"
#include "ast/passes/convert_literal_printblock_to_rawblock_pass.hpp"
#include "ast/passes/fold_constant_expressions_pass.hpp"
#include "ast/passes/pass_manager.hpp"
#include "ast/passes/resolve_includes_pass.hpp"

using namespace b2;

static AST* parseAST(const std::string &path)
{
	Parser parser;

    try {
        return parser.parse(path);
    } catch (SyntaxError err) {
        std::cerr << "syntax error at line " << err.line_no() << ": " << err.what() << std::endl;
        return nullptr;
    }
}

static int enable_resolve_includes_pass = 1;
static int enable_constant_folding_pass = 1;
static int enable_literal_print_block_to_raw_block_conversion_pass = 1;
static int enable_raw_block_coalescing_pass = 1;

static AST* optimizeAST(AST* ast, std::string basepath)
{
    PassManager passManager;

	if (enable_resolve_includes_pass) {
		passManager.addPass(new ResolveIncludesPass(basepath));
	}
	if (enable_constant_folding_pass) {
	    passManager.addPass(new FoldConstantExpressionsPass());
	}
	if (enable_literal_print_block_to_raw_block_conversion_pass) {
	    passManager.addPass(new ConvertLiteralPrintBlockToRawBlockPass());
	}
	if (enable_raw_block_coalescing_pass) {
		passManager.addPass(new CoalesceRawBlocksPass());
	}
    return passManager.run(ast);
}

static struct option long_options[] = {
	{"disable-pass", required_argument, nullptr, 'd'},
	{"enable-pass", required_argument, nullptr, 'e'},
	{"list-passes", no_argument, nullptr, 'l'},
	{"template-basepath", required_argument, nullptr, 't'},
	{"help", no_argument, nullptr, 'h'},
	{"enable-undefined-check", no_argument, nullptr, 'u'},
	{nullptr, 0, nullptr, 0},
};

static struct {
	const char* name;
	int* enabled;
} passes[] = {
	{"resolve-includes-pass", &enable_resolve_includes_pass},
	{"constant-folding-pass", &enable_constant_folding_pass},
	{"literal-print-to-raw-conversion-pass", &enable_literal_print_block_to_raw_block_conversion_pass},
	{"raw-block-coalescing-pass", &enable_raw_block_coalescing_pass},
};

static void list_passes()
{
	std::cerr << "Available passes:" << std::endl;
	for (auto &pass : passes) {
		std::cerr << "  " << pass.name << std::endl;
	}
}

static void usage(char* binary)
{
	std::cerr << "USAGE: " << binary << " [options] <template>" << std::endl;
	std::cerr << "OPTIONS:" << std::endl;
	std::cerr << "  --enable-pass=<pass>                       Enable pass (all are enabled by default)" << std::endl;
	std::cerr << "  --disable-pass=<pass>                      Disable pass" << std::endl;
	std::cerr << "  --list-passes                              Lists all passes" << std::endl;
	std::cerr << "  --template-basepath | -t                   Template basepath" << std::endl;
	std::cerr << "  --enable-undefined-check                   Checks whether a value is undefined and replaces it with an empty string" << std::endl;
	std::cerr << "  --help | -h                                Display this message" << std::endl;
}

int main(int argc, char* argv[])
{
	CodeEmitter code_emitter(std::cout);
	JavascriptVisitor js_visitor(code_emitter);

	char* binary = argv[0];
	std::string basepath;
	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "ht:", long_options, &option_index);
		if (c == -1) {
			break;
		}

		if (c == 0) {
			if (long_options[option_index].flag != nullptr) {
				continue;
			}

			c = long_options[option_index].val;
		}

		switch (c) {
			case 'd':
				for (auto &pass : passes) {
					if (!strcmp(pass.name, optarg)) {
						*pass.enabled = 0;
						break;
					}
				}
				break;
			case 'e':
				for (auto &pass : passes) {
					if (!strcmp(pass.name, optarg)) {
						*pass.enabled = 1;
						break;
					}
				}
				break;
			case 'l':
				list_passes();
				return 0;
			case 't':
				basepath = optarg;
				break;
			case 'u':
				js_visitor.performUndefinedCheck(true);
				break;
			case 'h':
			case '?':
				usage(binary);
				return 0;
			default:
				abort();
		}
	}
	argc -= optind;
	argv += optind;

    if (argc != 1) {
		usage(binary);
        return 1;
    }

	if (basepath.empty()) {
		basepath = dirname(argv[0]);
	}

	std::unique_ptr<AST> ast;
    ast.reset(parseAST(argv[0]));
    if (!ast) {
        return 1;
    }

	ast.reset(optimizeAST(ast.release(), basepath));

	js_visitor.visit(ast.get());

    return 0;
}
