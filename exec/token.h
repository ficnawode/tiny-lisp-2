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
	int line;
	int col;
} Position;

typedef struct Location
{
	Position start;
	Position end;
} Location;

typedef struct Token
{
	enum TokenType type;
	char *lexeme;
	Location location;
} Token;

struct Token
token_create(enum TokenType type, char *lexeme_buffer, Location location);
struct Token token_create_error(const char *message, Location location);
void token_cleanup(struct Token *token);

const char *token_type_to_string(enum TokenType type);
void print_token(struct Token *token);
