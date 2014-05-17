#ifndef __PARSER_H_
#define __PARSER_H_

#include <exception>
#include <string>

#include "ast/ast.hpp"

namespace b2 {

struct SyntaxError : std::exception {
	SyntaxError(const char* message, const int line_no) : m_message(message), m_line_no(line_no) {}

	virtual const char* what() const throw() {
		return m_message;
	}

	const int line_no() const throw() {
		return m_line_no;
	}

private:
	const char* m_message;
	const int m_line_no;
};

class Parser {
public:
	Parser();
	~Parser();

    b2::AST* parse(FILE* fd);
    b2::AST* parse(const std::string &filename);

private:
	void* lexer;
};

} // namespace b2

#endif /* __PARSER_H_ */
