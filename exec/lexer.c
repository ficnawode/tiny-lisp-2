#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

static char lexer_current_ch(struct LexerContext *ctx)
{
	if (ctx->buffer.data == NULL ||
		ctx->buffer.data[ctx->buffer.index] == '\0')
	{
		return '\0';
	}
	return ctx->buffer.data[ctx->buffer.index];
}

static void lexer_advance(struct LexerContext *ctx)
{
	if (ctx->buffer.data == NULL ||
		ctx->buffer.data[ctx->buffer.index] == '\0')
	{
		return;
	}
	char c = ctx->buffer.data[ctx->buffer.index];
	if (c == '\n')
	{
		ctx->cursor.line++;
		ctx->cursor.col = 1;
	}
	else
	{
		ctx->cursor.col++;
	}
	ctx->buffer.index++;
}

static Token lexer_handle_single_char(LexerContext *ctx,
									  enum TokenType token_type)
{
	g_string_truncate(ctx->lexeme, 0);
	Location location = {ctx->cursor, ctx->cursor};

	char c = lexer_current_ch(ctx);
	g_string_append_c(ctx->lexeme, c);
	lexer_advance(ctx);

	location.end = ctx->cursor;
	return token_create(token_type, ctx->lexeme->str, location);
}

static Token lexer_handle_whitespace(LexerContext *ctx)
{
	Location location = {ctx->cursor, ctx->cursor};

	while (isspace(lexer_current_ch(ctx)))
	{
		g_string_append_c(ctx->lexeme, lexer_current_ch(ctx));
		location.end = ctx->cursor;
		lexer_advance(ctx);
	}
	return token_create(TOKEN_WHITESPACE, ctx->lexeme->str, location);
}

static Token lexer_handle_comment(LexerContext *ctx)
{
	Location location = {ctx->cursor, ctx->cursor};
	while (lexer_current_ch(ctx) != '\n' &&
		   lexer_current_ch(ctx) != '\0')
	{
		g_string_append_c(ctx->lexeme, lexer_current_ch(ctx));
		lexer_advance(ctx);
	}
	location.end = ctx->cursor;
	lexer_advance(ctx);
	return token_create(TOKEN_COMMENT, ctx->lexeme->str, location);
}

static Token lexer_handle_str(LexerContext *ctx)
{
	Location location = {ctx->cursor, ctx->cursor};
	g_string_append_c(ctx->lexeme, lexer_current_ch(ctx));
	location.end = ctx->cursor;
	lexer_advance(ctx);
	while (lexer_current_ch(ctx) != '"' &&
		   lexer_current_ch(ctx) != '\0')
	{
		g_string_append_c(ctx->lexeme, lexer_current_ch(ctx));
		location.end = ctx->cursor;
		lexer_advance(ctx);
	}
	if (lexer_current_ch(ctx) == '\0')
	{
		return token_create_error("Unterminated string literal",
								  location);
	}
	g_string_append_c(ctx->lexeme, lexer_current_ch(ctx));
	location.end = ctx->cursor;
	lexer_advance(ctx);
	return token_create(TOKEN_STRING, ctx->lexeme->str, location);
}

static int is_symbol_char(char c)
{
	return isalnum(c) || strchr("#!$%&*+-./:<=>?@^_~", c) != NULL;
}

static Token lexer_handle_symbol(LexerContext *ctx)
{
	Location location = {ctx->cursor, ctx->cursor};

	while (lexer_current_ch(ctx) != '\0' &&
		   !isspace(lexer_current_ch(ctx)) &&
		   !strchr("()';\"", lexer_current_ch(ctx)))
	{
		char current = lexer_current_ch(ctx);
		if (!isdigit(current) && !strchr("+-", current) &&
			!is_symbol_char(current))
		{
			break;
		}
		g_string_append_c(ctx->lexeme, lexer_current_ch(ctx));
		location.end = ctx->cursor;
		lexer_advance(ctx);
	}

	char *endptr;
	strtod(ctx->lexeme->str, &endptr);

	if (*endptr == '\0' && (strcmp(ctx->lexeme->str, "+") != 0 &&
							strcmp(ctx->lexeme->str, "-") != 0))
	{
		bool contains_digit = false;
		for (int i = 0; ctx->lexeme->str[i] != '\0'; ++i)
		{
			if (isdigit(ctx->lexeme->str[i]))
			{
				contains_digit = true;
				break;
			}
		}

		if (contains_digit)
		{
			return token_create(TOKEN_NUMBER, ctx->lexeme->str,
								location);
		}
	}

	return token_create(TOKEN_SYMBOL, ctx->lexeme->str, location);
}

static Token lexer_handle_error(LexerContext *ctx)
{
	Location location = {ctx->cursor, ctx->cursor};
	g_string_append_c(ctx->lexeme, lexer_current_ch(ctx));
	lexer_advance(ctx);

	location.end = ctx->cursor;
	char error_msg[64];
	snprintf(error_msg, sizeof(error_msg), "Illegal character: '%s'",
			 ctx->lexeme->str);
	return token_create_error(error_msg, location);
}

LexerContext lexer_create(const char *source_code)
{

	LexerContext ctx;

	ctx.buffer.data = source_code;
	ctx.buffer.index = 0;
	ctx.cursor.line = 1;
	ctx.cursor.col = 1;

	ctx.lexeme = g_string_new(NULL);
	return ctx;
}

void lexer_cleanup(LexerContext *ctx)
{
	g_string_free(ctx->lexeme, TRUE);
	ctx->buffer.data = NULL;
}

Token lexer_next(LexerContext *ctx)
{
	g_string_truncate(ctx->lexeme, 0);
	char c = lexer_current_ch(ctx);

	if (c == '\0')
	{
		return lexer_handle_single_char(ctx, TOKEN_EOF);
	}
	if (isspace(c))
	{
		return lexer_handle_whitespace(ctx);
	}
	if (c == ';')
	{
		return lexer_handle_comment(ctx);
	}

	switch (c)
	{
	case '(':
		return lexer_handle_single_char(ctx, TOKEN_LPAREN);
	case ')':
		return lexer_handle_single_char(ctx, TOKEN_RPAREN);
	case '\'':
		return lexer_handle_single_char(ctx, TOKEN_QUOTE);
	}

	if (c == '"')
	{
		return lexer_handle_str(ctx);
	}
	if (isdigit(c) || c == '+' || c == '-' || is_symbol_char(c))
	{
		return lexer_handle_symbol(ctx);
	}

	return lexer_handle_error(ctx);
}