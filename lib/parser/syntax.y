%{
  #include "ast/visitors.hpp"
  #include "parser/syntax.hpp"

  using namespace b2;

  static inline bool is_variant(Expression* expr)
  {
    return expr->valueType() == VariantType;
  }

  static inline bool is_numeric(Expression* expr)
  {
    auto type = expr->valueType();
    return type == IntegerType || type == DoubleType;
  }

  static inline bool is_boolean(Expression* expr)
  {
    return expr->valueType() == BooleanType;
  }

  static inline AST* from_statements_array(ASTList *list)
  {
    if (list->size() == 1) {
      AST* ast = list->front().release();
      delete list;
      return ast;
    } else {
      return new StatementsAST(list);
    }
  }

  static inline void add_ast_to_statements(ASTList* list, AST* ast)
  {
    if (ast->type() == StatementsASTType) {
      auto statements_ast = static_cast<StatementsAST*>(ast);
      for (auto &statement : *statements_ast->statements) {
        list->push_back(std::move(statement));
      }
      delete ast;
    } else {
      std::unique_ptr<AST> p_ast(ast);
      list->push_back(std::move(p_ast));
    }
  }

  static inline void add_to_stringexprmap(StringExpressionMap* map, const char* value)
  {
    (*map)[value] = std::move(std::unique_ptr<Expression>(new VariableReferenceExpression(value)));
    // don't free `value`, it is now owned by the above created VariableReferenceExpression
  }

  static inline void add_to_stringexprmap(StringExpressionMap* map, const char* key, Expression* value)
  {
    (*map)[key] = std::move(std::unique_ptr<Expression>(value));
    free((void*) key);
  }

  #define ASSERT_THAT(cond, msg) if (!(cond)) { yyerror(&yyloc, scanner, YY_(msg)); YYERROR; }
  #define ASSERT_NUMERIC(x) ASSERT_THAT(is_numeric(x) || is_variant(x), "numeric expression expected")
  #define ASSERT_BOOLEAN(x) ASSERT_THAT(is_boolean(x) || is_variant(x), "boolean expression expected")
  #define ASSERT_MAP(x) ASSERT_THAT(is_variant(x), "map expression expected")
%}

%require "3.0.2"
%name-prefix "syntax_"
%locations
%define api.pure full
%define api.value.type {SyntaxType}
%define parse.error verbose
%define parse.lac full
%parse-param {void* scanner}
%lex-param {yyscan_t* scanner}
%start root

%token<str> T_RAW
%token T_VARIABLE_START T_VARIABLE_END T_BLOCK_START T_BLOCK_END
%token T_KW_IF T_KW_ELSEIF T_KW_ELSE T_KW_ENDIF T_KW_FOR T_KW_ENDFOR T_KW_IN T_KW_INCLUDE T_KW_WITH T_KW_USING

%token<str> T_IDENTIFIER T_STRING_LITERAL
%token<b> T_BOOLEAN_LITERAL
%token<d> T_DOUBLE_LITERAL
%token<l> T_INTEGER_LITERAL

%destructor { free((void*)$$); } <str>

%token T_EQ T_NEQ T_GT T_GE T_LT T_LE T_AND T_OR T_NOT T_PLUS T_MINUS T_MUL T_DIV T_MOD T_OPEN_PAREN T_CLOSE_PAREN T_ATTRIBUTE_SEPARATOR T_COMMA T_ASSIGN

%right T_OR
%right T_AND
%right T_EQ T_NEQ
%right T_GT T_LT T_GE T_LE
%left T_PLUS T_MINUS
%left T_MUL T_DIV T_MOD
%left UMINUS UPLUS
%left T_NOT

%type<expr> expression var_ref_expression
%type<expr_arr> arguments
%destructor { delete $$; } <expr>
%destructor { delete $$; } <expr_arr>

%type<ast> statement print_block if_block else_block elseif_blocks for_statement elsefor_statement include_block
%type<ast_arr> statements raw_blocks
%destructor { delete $$; } <ast>
%destructor { delete $$; } <ast_arr>

%type<strexpr_map> include_variable_mapping
%destructor { delete $$; } <strexpr_map>

%%

root
  : %empty { syntax_set_extra(new StatementsAST(), scanner); }
  | statements { syntax_set_extra(from_statements_array($1), scanner); }
;

statements
  : statement {
      $$ = new ASTList();
      add_ast_to_statements($$, $1);
    }
  | statements statement {
      add_ast_to_statements($1, $2);
      $$ = $1;
    }
;

statement
  : raw_blocks { $$ = from_statements_array($1); }
  | print_block
  | if_block
  | for_statement
  | include_block
;

raw_blocks
  : T_RAW {
      $$ = new ASTList();
      std::unique_ptr<AST> ast(new RawBlockAST($1));
      $$->push_back(std::move(ast));
    }
  | raw_blocks T_RAW {
      std::unique_ptr<AST> ast(new RawBlockAST($2));
      $1->push_back(std::move(ast));
      $$ = $1;
    }
;

print_block:
  T_VARIABLE_START expression T_VARIABLE_END { $$ = new PrintBlockAST($2); }
;

if_block:
  T_BLOCK_START T_KW_IF expression[cond] T_BLOCK_END
  statements[then_body]
  T_BLOCK_START
  elseif_blocks[elif_blocks]
  else_block[else_body]
  T_KW_ENDIF T_BLOCK_END
  {
    if ($elif_blocks) {
      IfBlockAST* elif_block = static_cast<IfBlockAST*>($elif_blocks);
      while (elif_block->elseBody) {
        elif_block = static_cast<IfBlockAST*>(elif_block->elseBody.get());
      }
      static_cast<IfBlockAST*>(elif_block)->elseBody.reset($else_body);
      $else_body = $elif_blocks;
    }
    $$ = new IfBlockAST($cond, from_statements_array($then_body), $else_body);
  }
;

elseif_blocks
  : %empty { $$ = NULL; }
  | elseif_blocks[prev_elif]
    T_KW_ELSEIF expression[cond] T_BLOCK_END
    statements[body]
    T_BLOCK_START
    {
      AST* elif = new IfBlockAST($cond, from_statements_array($body));
      if ($prev_elif) {
        static_cast<IfBlockAST*>($prev_elif)->elseBody.reset(elif);
        $$ = $prev_elif;
      } else {
        $$ = elif;
      }
    }
;

else_block
  : %empty { $$ = NULL; }
  | T_KW_ELSE T_BLOCK_END
    statements[body]
    T_BLOCK_START
    { $$ = from_statements_array($body); }
;

for_statement
  :
    T_BLOCK_START T_KW_FOR var_ref_expression[value] T_KW_IN var_ref_expression[iterable] T_BLOCK_END
    statements[body]
    T_BLOCK_START
    elsefor_statement[elseBody]
    T_KW_ENDFOR T_BLOCK_END
    {
      $$ = new ForBlockAST(
        NULL,
        static_cast<VariableReferenceExpression*>($value),
        static_cast<VariableReferenceExpression*>($iterable),
        from_statements_array($body),
        $elseBody
      );
    }
  |
    T_BLOCK_START T_KW_FOR var_ref_expression[key] T_COMMA var_ref_expression[value] T_KW_IN var_ref_expression[iterable] T_BLOCK_END
    statements[body]
    T_BLOCK_START
    elsefor_statement[elseBody]
    T_KW_ENDFOR T_BLOCK_END
    {
      $$ = new ForBlockAST(
        static_cast<VariableReferenceExpression*>($key),
        static_cast<VariableReferenceExpression*>($value),
        static_cast<VariableReferenceExpression*>($iterable),
        from_statements_array($body),
        $elseBody
      );
    }
;

elsefor_statement
  : %empty { $$ = NULL; }
  | T_KW_ELSE T_BLOCK_END
    statements[body]
    T_BLOCK_START
    {
      $$ = from_statements_array($body);
    }
;

include_block
  : T_BLOCK_START T_KW_INCLUDE T_STRING_LITERAL[templateName] T_BLOCK_END
    {
      $$ = new IncludeBlockAST($templateName);
    }
  | T_BLOCK_START T_KW_INCLUDE T_STRING_LITERAL[templateName] T_KW_USING expression[scope] T_BLOCK_END
    {
      ASSERT_MAP($scope);
      $$ = new IncludeBlockAST($templateName, $scope);
    }
  | T_BLOCK_START T_KW_INCLUDE T_STRING_LITERAL[templateName] T_KW_WITH include_variable_mapping[varMapping] T_BLOCK_END
    {
      $$ = new IncludeBlockAST($templateName);
      static_cast<IncludeBlockAST*>($$)->variableMapping = std::move(*$varMapping);
      delete $varMapping;
    }
;

include_variable_mapping
  : T_IDENTIFIER[val] { $$ = new StringExpressionMap(); add_to_stringexprmap($$, $val); }
  | T_IDENTIFIER[key] T_ASSIGN expression[val] { $$ = new StringExpressionMap(); add_to_stringexprmap($$, $key, $val); }
  | include_variable_mapping[mapping] T_COMMA T_IDENTIFIER[val] { $$ = $mapping; add_to_stringexprmap($$, $val); }
  | include_variable_mapping[mapping] T_COMMA T_IDENTIFIER[key] T_ASSIGN expression[val] { $$ = $mapping; add_to_stringexprmap($$, $key, $val); }
;

var_ref_expression
  : T_IDENTIFIER { $$ = new VariableReferenceExpression($1); }
  | var_ref_expression[variable] T_ATTRIBUTE_SEPARATOR T_IDENTIFIER[attribute] { $$ = new GetAttributeExpression($variable, $attribute); }
;

arguments
  : expression {
      $$ = new ExpressionList();
      std::unique_ptr<Expression> expr($1);
      $$->push_back(std::move(expr));
    }
  | arguments[other] T_COMMA expression[expr] {
      std::unique_ptr<Expression> expr($expr);
      $other->push_back(std::move(expr));
      $$ = $other;
    }
;

expression
  : var_ref_expression
  | T_STRING_LITERAL { $$ = new StringLiteralExpression($1); }
  | T_DOUBLE_LITERAL { $$ = new DoubleLiteralExpression($1); }
  | T_INTEGER_LITERAL { $$ = new IntegerLiteralExpression($1); }
  | T_BOOLEAN_LITERAL { $$ = new BooleanLiteralExpression($1); }
  | T_MINUS expression %prec UMINUS { ASSERT_NUMERIC($2); $$ = new UnaryOperationExpression($2, '-'); }
  | T_PLUS expression %prec UPLUS { ASSERT_NUMERIC($2); $$ = new UnaryOperationExpression($2, '+'); }
  | expression T_PLUS expression { ASSERT_NUMERIC($1); ASSERT_NUMERIC($3); $$ = new BinaryOperationExpression($1, $3, '+'); }
  | expression T_MINUS expression { ASSERT_NUMERIC($1); ASSERT_NUMERIC($3); $$ = new BinaryOperationExpression($1, $3, '-'); }
  | expression T_MUL expression { ASSERT_NUMERIC($1); ASSERT_NUMERIC($3); $$ = new BinaryOperationExpression($1, $3, '*'); }
  | expression T_DIV expression { ASSERT_NUMERIC($1); ASSERT_NUMERIC($3); $$ = new BinaryOperationExpression($1, $3, '/'); }
  | expression T_MOD expression { ASSERT_NUMERIC($1); ASSERT_NUMERIC($3); $$ = new BinaryOperationExpression($1, $3, '%'); }
  | expression T_EQ expression { $$ = new ComparisonExpression($1, $3, "=="); }
  | expression T_NEQ expression { $$ = new ComparisonExpression($1, $3, "!="); }
  | expression T_GT expression { ASSERT_NUMERIC($1); ASSERT_NUMERIC($3); $$ = new ComparisonExpression($1, $3, ">"); }
  | expression T_GE expression { ASSERT_NUMERIC($1); ASSERT_NUMERIC($3); $$ = new ComparisonExpression($1, $3, ">="); }
  | expression T_LT expression { ASSERT_NUMERIC($1); ASSERT_NUMERIC($3); $$ = new ComparisonExpression($1, $3, "<"); }
  | expression T_LE expression { ASSERT_NUMERIC($1); ASSERT_NUMERIC($3); $$ = new ComparisonExpression($1, $3, "<="); }
  | expression T_OR expression { ASSERT_BOOLEAN($1); ASSERT_BOOLEAN($3); $$ = new ComparisonExpression($1, $3, "||"); }
  | expression T_AND expression { ASSERT_BOOLEAN($1); ASSERT_BOOLEAN($3); $$ = new ComparisonExpression($1, $3, "&&"); }
  | T_NOT expression { ASSERT_BOOLEAN($2); $$ = new UnaryOperationExpression($2, '!'); }
  | T_OPEN_PAREN expression T_CLOSE_PAREN { $$ = $2; }
  | T_IDENTIFIER[method] T_OPEN_PAREN arguments[args] T_CLOSE_PAREN { $$ = new MethodCallExpression($method, $args); }
;
