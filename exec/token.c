#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

struct Token
token_create(enum TokenType type, char *lexeme_buffer, Location location)
{
	struct Token token;
	token.type = type;
	token.lexeme = strdup(lexeme_buffer);
	assert(token.lexeme && "Lexer Error: Failed to duplicate lexeme string\n");
	token.location = location;
	return token;
}

struct Token token_create_error(const char *message, Location location)
{
	struct Token token;
	token.type = TOKEN_ERROR;
	asprintf(&token.lexeme, "Error at %d:%d - %s", location.start.line,
			 location.start.col, message);
	assert(token.lexeme &&
		   "Lexer Error: Failed to allocate error message string\n");
	token.location = location;
	return token;
}

void token_cleanup(struct Token *token)
{
	if (token->lexeme != NULL)
		free(token->lexeme);
}

const char *token_type_to_string(enum TokenType type)
{
	switch (type)
	{
	case TOKEN_LPAREN:
		return "TOKEN_LPAREN";
	case TOKEN_RPAREN:
		return "TOKEN_RPAREN";
	case TOKEN_QUOTE:
		return "TOKEN_QUOTE";
	case TOKEN_SYMBOL:
		return "TOKEN_SYMBOL";
	case TOKEN_NUMBER:
		return "TOKEN_NUMBER";
	case TOKEN_STRING:
		return "TOKEN_STRING";
	case TOKEN_WHITESPACE:
		return "TOKEN_WHITESPACE";
	case TOKEN_COMMENT:
		return "TOKEN_COMMENT";
	case TOKEN_EOF:
		return "TOKEN_EOF";
	case TOKEN_ERROR:
		return "TOKEN_ERROR";
	default:
		return "UNKNOWN_TOKEN";
	}
}

void print_token(struct Token *token)
{
	if (token->type == TOKEN_WHITESPACE)
		return;
	Position start = token->location.start;
	Position end = token->location.end;
	printf("Type: %-15s Lexeme: \"%s\" (Pos: %d:%d to %d:%d)\n",
		   token_type_to_string(token->type), token->lexeme, //
		   start.line, start.col, end.line, end.col);
}