#pragma once

// For Jack code and parsing
enum class Token { KEYWORD = 0, SYMBOL, IDENTIFIER, INT_CONST, STRING_CONST, NONE };
enum class Keyword { CLASS = 0, METHOD, FUNCTION, CONSTRUCTOR, INT, BOOLEAN, CHAR, VOID, VAR, STATIC, FIELD, LET, DO, IF, ELSE, WHILE, RETURN, TRUE, FALSE, K_NULL, THIS, NONE };
enum class Status { OK, NOT_FOUND, SYNTAX_ERROR, FAILURE };
enum class NonTerminal {
	CLASS, CLASS_VAR_DEC, SUBROUTINE_DEC, PARAMETER_LIST, SUBROUTINE_BODY, VAR_DEC, STATEMENTS, LET, IF, WHILE, DO, RETURN, EXPRESSION, TERM, EXPRESSION_LIST,
	IDENTIFIER, IDENTIFIER_CAT, IDENTIFIER_IDX, IDENTIFIER_BEING_DEFINED
};

// For VM code
enum class Kind { STATIC, FIELD, ARG, VAR };
enum class Segment { CONST, ARG, LOCAL, STATIC, THIS, THAT, POINTER, TEMP, NONE };
enum class Command { ADD, SUB, NEG, EQ, GT, LT, AND, OR, NOT, MULT, DIV, NONE };
