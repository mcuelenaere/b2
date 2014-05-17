#ifndef __SYNTAX_H_
#define __SYNTAX_H_

#include <stdio.h>
#include "ast/ast.hpp"
#include "ast/expressions.hpp"
#include "ast/visitors.hpp"

#define YY_EXTRA_TYPE b2::AST*
#define YYSTYPE SyntaxType
#define YYLTYPE LexPosition

union SyntaxType {
  const char* str;
  long l;
  double d;
  bool b;
  b2::Expression* expr;
  b2::ExpressionList* expr_arr;
  b2::AST* ast;
  b2::ASTList* ast_arr;
  b2::StringExpressionMap* strexpr_map;
};

struct LexPosition
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

extern int syntax_parse(void* scanner);
extern int syntax_error(LexPosition*, const void* scanner, const char *s);
extern int syntax_lex(SyntaxType*, LexPosition*, void* scanner);
extern int syntax_lex_destroy(void* scanner);
extern int syntax_lex_init(void** scanner_ptr);
extern b2::AST* syntax_get_extra(void* scanner);
extern void syntax_set_extra(b2::AST*, void* scanner);
extern void syntax_set_in(FILE* file, void* scanner);
extern int syntax_get_lineno(void* scanner);

extern void syntax_set_debug(int bdebug, void* scanner);
extern int syntax_debug;

#endif /* __SYNTAX_H_ */
