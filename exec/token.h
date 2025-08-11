#pragma once

enum TokenType
{
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_QUOTE,
	TOKEN_SYMBOL,
	TOKEN_NUMBER,
	TOKEN_STRING,
	TOKEN_WHITESPACE,
	TOKEN_COMMENT,
	TOKEN_EOF,
	TOKEN_ERROR
};

typedef struct Position
{
	struct
	{
		int line;
		int col;
	} start;
	struct
	{
		int line;
		int col;
	} end;
} Position;

typedef struct Token
{
	enum TokenType type;
	char *lexeme;
	Position pos;
} Token;

struct Token
token_create(enum TokenType type, char *lexeme_buffer, Position pos);

struct Token token_create_error(const char *message, Position pos);
void token_cleanup(struct Token *token);

const char *token_type_to_string(enum TokenType type);
void print_token(struct Token *token);
