#pragma once

#include "lexer.h"
#include "node.h"
#include <stdbool.h>

enum ParserErrorType
{
	PARSER_WARNING,
	PARSER_ERROR
};

typedef struct ParserError
{
	enum ParserErrorType type;
	Token token;
	char *error_msg;

} ParserError;

typedef struct ParserContext
{
	LexerContext *lexer;

	Token current_token;

	Env *global_env;

	GPtrArray *errors;
	bool panic_mode;
} ParserContext;

ParserContext *parser_create(char *source_code);

void parser_cleanup(ParserContext *ctx);

NodeArray *parser_parse(ParserContext *ctx);

void parser_print_errors(ParserContext *ctx);