#include <stdexcept>

#include "parser/parser.hpp"
#include "parser/syntax.hpp"
#include "utils.hpp"

using namespace b2;

Parser::Parser() : lexer(NULL)
{
	if (syntax_lex_init(&this->lexer) != 0) {
		throw std::runtime_error("Couldn't init lexer");
	}
}

Parser::~Parser()
{
	if (this->lexer != NULL) {
		if (syntax_lex_destroy(this->lexer) != 0) {
			throw std::runtime_error("Couldn't destroy lexer");
		}
	}
}

static const char* err_message;

b2::AST* Parser::parse(FILE* fd)
{
	// TODO: removeme
    //syntax_set_debug(1, this->lexer);
    //syntax_debug = 1;

	err_message = NULL;

	syntax_set_in(fd, this->lexer);

    if (syntax_parse(this->lexer) != 0) {
		// TODO
	}

	if (err_message != NULL) {
		throw SyntaxError(err_message, syntax_get_lineno(this->lexer));
	}

    return syntax_get_extra(this->lexer);
}

b2::AST* Parser::parse(const std::string &filename)
{
    FileGuard file(filename.c_str(), "r");

    return this->parse(file.fd);
}

int syntax_error(LexPosition* position, const void* lexer, const char* message)
{
	err_message = strdup(message); // FIXME: this isn't thread-safe!
	return 0;
}
