#include <iostream>
#include <memory>

#include <libgen.h>
#include <getopt.h>

#include "parser/parser.hpp"
#include "utils/print_visitor.hpp"

#include "ast/passes/coalesce_rawblocks_pass.hpp"
#include "ast/passes/convert_literal_printblock_to_rawblock_pass.hpp"
#include "ast/passes/fold_constant_expressions_pass.hpp"
#include "ast/passes/pass_manager.hpp"
#include "ast/passes/resolve_includes_pass.hpp"

using namespace b2;

static void printAST(AST* ast)
{
	PrintVisitor visitor;

    visitor.visit(ast);
}

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

static void usage(char* binary)
{
	static const char* passes[] = {
		"resolve-includes-pass",
		"constant-folding-pass",
		"literal-print-to-raw-conversion-pass",
		"raw-block-coalescing-pass"
	};

	std::cerr << "USAGE: " << binary << " [options] <template>" << std::endl;
	std::cerr << "OPTIONS:" << std::endl;
	std::cerr << "  --enable-all-passes                        Enable all passes [default]" << std::endl;
	std::cerr << "  --disable-all-passes                       Disable all passes" << std::endl;
	std::cerr << "  --enable-resolve-includes-pass" << std::endl;
	for (auto pass : passes) {
		std::cerr << "  --enable-" << pass << std::endl;
		std::cerr << "  --disable-" << pass << std::endl;
	}
	std::cerr << "  --template-basepath | -t                   Template basepath" << std::endl;
	std::cerr << "  --help | -h                                Display this message" << std::endl;
}

static struct option long_options[] = {
	{"disable-all-passes", no_argument, nullptr, 'd'},
	{"enable-all-passes", no_argument, nullptr, 'e'},
	{"enable-resolve-includes-pass", no_argument, &enable_resolve_includes_pass, 1},
	{"disable-resolve-includes-pass", no_argument, &enable_resolve_includes_pass, 0},
	{"enable-constant-folding-pass", no_argument, &enable_constant_folding_pass, 1},
	{"disable-constant-folding-pass", no_argument, &enable_constant_folding_pass, 0},
	{"enable-literal-print-to-raw-conversion-pass", no_argument, &enable_literal_print_block_to_raw_block_conversion_pass, 1},
	{"disable-literal-print-to-raw-conversion-pass", no_argument, &enable_literal_print_block_to_raw_block_conversion_pass, 0},
	{"enable-raw-block-coalescing-pass", no_argument, &enable_raw_block_coalescing_pass, 1},
	{"disable-raw-block-coalescing-pass", no_argument, &enable_raw_block_coalescing_pass, 0},
	{"template-basepath", required_argument, nullptr, 't'},
	{"help", no_argument, nullptr, 'h'},
	{nullptr, 0, nullptr, 0},
};

int main(int argc, char* argv[])
{
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
				enable_resolve_includes_pass = 0;
				enable_constant_folding_pass = 0;
				enable_literal_print_block_to_raw_block_conversion_pass = 0;
				enable_raw_block_coalescing_pass = 0;
				break;
			case 'e':
				enable_resolve_includes_pass = 1;
				enable_constant_folding_pass = 1;
				enable_literal_print_block_to_raw_block_conversion_pass = 1;
				enable_raw_block_coalescing_pass = 1;
				break;
			case 't':
				basepath = optarg;
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

	try {
		ast.reset(optimizeAST(ast.release(), basepath));
	} catch (std::runtime_error& e) {
		std::cerr << "Runtime error: " << e.what() << std::endl;
		return 1;
	}

    printAST(ast.get());

    return 0;
}
