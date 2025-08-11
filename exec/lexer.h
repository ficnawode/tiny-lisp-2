#pragma once

#include "token.h"
#include <glib.h>
#include <stddef.h>

typedef struct LexerContext
{
	struct
	{
		const char *data;
		int index;
	} buffer;

	Position cursor;
	GString *lexeme;
} LexerContext;

LexerContext lexer_create(const char *source_code);

// Parser will have to free token lexeme!
Token lexer_next(LexerContext *ctx);

void lexer_cleanup(LexerContext *ctx);
