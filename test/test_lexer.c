#include <glib.h>
#include <stdio.h>

#include "lexer.h"

typedef struct TokenSequence
{
	enum TokenType *tts;
	int len;
} TokenSequence;

static void expect_token_sequence(char *code, TokenSequence seq)
{
	LexerContext lexer = lexer_create(code);
	for (int i = 0; i < seq.len; i++)
	{
		Token token = lexer_next(&lexer);
		print_token(&token);
		enum TokenType expected_type = seq.tts[i];
		printf("\nExpected token: %s",
			   token_type_to_string(expected_type));
		g_assert(token.type == expected_type);
	}
	Token token = lexer_next(&lexer);
	g_assert(token.type == TOKEN_EOF);
}

#define EXPECT_TOKENS(src, ...)                                      \
	do                                                               \
	{                                                                \
		enum TokenType _tts[] = {__VA_ARGS__};                       \
		TokenSequence _seq = {_tts, sizeof(_tts) / sizeof(_tts[0])}; \
		expect_token_sequence(src, _seq);                            \
	} while (0)

static void test_single_char(void)
{
	EXPECT_TOKENS("(", TOKEN_LPAREN, TOKEN_EOF);
	EXPECT_TOKENS(")", TOKEN_RPAREN, TOKEN_EOF);
	EXPECT_TOKENS("\'", TOKEN_QUOTE, TOKEN_EOF);
	EXPECT_TOKENS("", TOKEN_EOF);
}

static void test_number(void)
{
	EXPECT_TOKENS("1", TOKEN_NUMBER, TOKEN_EOF);
	EXPECT_TOKENS("+9", TOKEN_NUMBER, TOKEN_EOF);
	EXPECT_TOKENS("0", TOKEN_NUMBER, TOKEN_EOF);
	EXPECT_TOKENS("-3", TOKEN_NUMBER, TOKEN_EOF);
	EXPECT_TOKENS("-3.", TOKEN_NUMBER, TOKEN_EOF);
	EXPECT_TOKENS("+3.14159", TOKEN_NUMBER, TOKEN_EOF);
	EXPECT_TOKENS("+.14159", TOKEN_NUMBER, TOKEN_EOF);
	EXPECT_TOKENS(".14159", TOKEN_NUMBER, TOKEN_EOF);
}

static void test_label(void)
{
	EXPECT_TOKENS("a", TOKEN_SYMBOL, TOKEN_EOF);
	EXPECT_TOKENS("asdf", TOKEN_SYMBOL, TOKEN_EOF);
	EXPECT_TOKENS("1a", TOKEN_SYMBOL, TOKEN_EOF);
	EXPECT_TOKENS("a1", TOKEN_SYMBOL, TOKEN_EOF);
}

static void test_whitespace(void)
{
	EXPECT_TOKENS(" ", TOKEN_WHITESPACE, TOKEN_EOF);
	EXPECT_TOKENS("\n", TOKEN_WHITESPACE, TOKEN_EOF);
	EXPECT_TOKENS("\t", TOKEN_WHITESPACE, TOKEN_EOF);
	EXPECT_TOKENS("\r", TOKEN_WHITESPACE, TOKEN_EOF);

	EXPECT_TOKENS("a ", TOKEN_SYMBOL, TOKEN_WHITESPACE, TOKEN_EOF);
	EXPECT_TOKENS(" a", TOKEN_WHITESPACE, TOKEN_SYMBOL, TOKEN_EOF);
	EXPECT_TOKENS("a\n", TOKEN_SYMBOL, TOKEN_WHITESPACE, TOKEN_EOF);
	EXPECT_TOKENS(" \na", TOKEN_WHITESPACE, TOKEN_SYMBOL, TOKEN_EOF);
	EXPECT_TOKENS("a\t ", TOKEN_SYMBOL, TOKEN_WHITESPACE, TOKEN_EOF);
	EXPECT_TOKENS("\ta", TOKEN_WHITESPACE, TOKEN_SYMBOL, TOKEN_EOF);
}

static void test_comment(void)
{
	EXPECT_TOKENS("a ;", TOKEN_SYMBOL, TOKEN_WHITESPACE,
				  TOKEN_COMMENT, TOKEN_EOF);
	EXPECT_TOKENS("a;comment whatever", TOKEN_SYMBOL, TOKEN_COMMENT,
				  TOKEN_EOF);
	EXPECT_TOKENS("a;comment \nwhatever", TOKEN_SYMBOL, TOKEN_COMMENT,
				  TOKEN_SYMBOL, TOKEN_EOF);
}
static void test_error(void)
{
	EXPECT_TOKENS("α", TOKEN_ERROR, TOKEN_ERROR, TOKEN_EOF);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/lexer/single_chars", test_single_char);
	g_test_add_func("/lexer/numbers", test_number);
	g_test_add_func("/lexer/labels", test_label);
	g_test_add_func("/lexer/whitespace", test_whitespace);
	g_test_add_func("/lexer/comment", test_comment);
	g_test_add_func("/lexer/error", test_error);

	return g_test_run();
}