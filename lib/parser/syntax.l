%{
  #include "ast/visitors.hpp"
  #include "parser/syntax.hpp"
  #include "tokenizer.hpp"

  static char* process_string_literal(char* text, int len);
  static int eat_whitespace = 0; // TODO: eliminate global variable

  #define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno;
  // TODO: column
  #define ECHO // don't print to stdout on unknown rule
%}

%x IN_VARIABLE
%x IN_BLOCK
%x IN_COMMENT

%option reentrant bison-bridge bison-locations noyywrap debug yylineno

IDENTIFIER ([a-zA-Z])|(_+[a-zA-Z][a-zA-Z0-9_]*)|([a-zA-Z][a-zA-Z0-9_]*)
STRING_LITERAL \"([^\"]|\\\"|\\\\)*\"
STRING_LITERAL_SQ \'([^\']|\\\'|\\\\)*\'
INTEGER [0-9]+
DOUBLE (([0-9]+\.[0-9]+)|(\.[0-9]+)|([0-9]+))([Ee][\+\-]?[0-9]+)?

%%

<INITIAL>{
    "{{"                { eat_whitespace = 0; BEGIN(IN_VARIABLE); return T_VARIABLE_START; }
    "{%"                { eat_whitespace = 0; BEGIN(IN_BLOCK); return T_BLOCK_START; }
    "{-%"               { eat_whitespace = 0; BEGIN(IN_BLOCK); return T_BLOCK_START; }
    "{#"                { eat_whitespace = 0; BEGIN(IN_COMMENT); }
    [^{ \t\n\r]+        { eat_whitespace = 0; yylval->str = strdup(yytext); return T_RAW; }
    [ \t\n\r]+/"{-%"    // ignore
    [ \t\n\r]+          { if (!eat_whitespace) { yylval->str = strdup(yytext); return T_RAW; } }
    .                   { yylval->str = strdup(yytext); return T_RAW; }
}

<IN_COMMENT>{
    "#}"                { BEGIN(INITIAL); }
    [^#]+               // ignore
    .                   // ignore
}

<IN_VARIABLE>{
    "}}"                { BEGIN(INITIAL); return T_VARIABLE_END; }
    [ \t]+              // ignore
}

<IN_BLOCK>{
    "%}"                { BEGIN(INITIAL); return T_BLOCK_END; }
    "%-}"               { BEGIN(INITIAL); eat_whitespace = 1; return T_BLOCK_END; }
    [ \t]+              // ignore
    "if"                { return T_KW_IF; }
    "else if"           { return T_KW_ELSEIF; }
    "elseif"            { return T_KW_ELSEIF; }
    "else"              { return T_KW_ELSE; }
    "endif"             { return T_KW_ENDIF; }
    "for"               { return T_KW_FOR; }
    "in"                { return T_KW_IN; }
    "endfor"            { return T_KW_ENDFOR; }
    "include"           { return T_KW_INCLUDE; }
    "with"              { return T_KW_WITH; }
    "using"             { return T_KW_USING; }
}

<IN_BLOCK,IN_VARIABLE>{
    "=="                { return T_EQ; }
    "!="                { return T_NEQ; }
    ">"                 { return T_GT; }
    ">="                { return T_GE; }
    "<"                 { return T_LT; }
    "<="                { return T_LE; }
    "&&"                { return T_AND; }
    "and"               { return T_AND; }
    "AND"               { return T_AND; }
    "||"                { return T_OR; }
    "or"                { return T_OR; }
    "OR"                { return T_OR; }
    "!"                 { return T_NOT; }
    "not"               { return T_NOT; }
    "NOT"               { return T_NOT; }
    "+"                 { return T_PLUS; }
    "-"                 { return T_MINUS; }
    "*"                 { return T_MUL; }
    "/"                 { return T_DIV; }
    "%"                 { return T_MOD; }
    "("                 { return T_OPEN_PAREN; }
    ")"                 { return T_CLOSE_PAREN; }
    "."                 { return T_ATTRIBUTE_SEPARATOR; }
    ","                 { return T_COMMA; }
    "="                 { return T_ASSIGN; }
    {STRING_LITERAL}    { yylval->str = process_string_literal(yytext, yyleng); return T_STRING_LITERAL; }
    {STRING_LITERAL_SQ} { yylval->str = process_string_literal(yytext, yyleng); return T_STRING_LITERAL; }
    {INTEGER}           { yylval->l = atol(yytext); return T_INTEGER_LITERAL; }
    {DOUBLE}            { yylval->d = atof(yytext); return T_DOUBLE_LITERAL; }
    "true"              { yylval->b = true; return T_BOOLEAN_LITERAL; }
    "TRUE"              { yylval->b = true; return T_BOOLEAN_LITERAL; }
    "false"             { yylval->b = false; return T_BOOLEAN_LITERAL; }
    "FALSE"             { yylval->b = false; return T_BOOLEAN_LITERAL; }
    {IDENTIFIER}        { yylval->str = strdup(yytext); return T_IDENTIFIER; }
}

%%

static char* process_string_literal(char* text, int len)
{
    // don't include quotes at beginning and end of text
    char* ptr = &text[1];
    char* end_ptr = &text[len - 1];

    while (ptr < end_ptr) {
        if (*ptr == '\\') {
            // remove escape character from text
            memmove(ptr, ptr + 1, end_ptr - ptr - 1);
            end_ptr--;

            // NOTE: the next character being skipped is on purpose and not a bug;
            //       this allows "\\" to be reduced to "\" (and not "").
        }

        ptr++;
    }

    // terminate string (removes the quote at the end of text)
    *end_ptr = '\0';

    // return string starting at second character (gets rid of quote at the beginning of text)
    return strdup(&text[1]);
}
