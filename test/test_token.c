#include <glib.h>
#include <stdio.h>

#include "token.h"

static void test_alloc(void)
{
	Position p = {0, 0, 1, 1};
	Token t = token_create(TOKEN_SYMBOL, "aaa", p);
	g_assert(t.lexeme != NULL);
	g_assert(strlen(t.lexeme) == 3);
	token_cleanup(&t);
}

static void test_err(void)
{
	Position p = {0, 0, 1, 1};
	Token t = token_create_error("whatever", p);
	g_assert(t.lexeme != NULL);
	g_assert(strlen(t.lexeme) == 23);
	token_cleanup(&t);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/token/alloc", test_alloc);
	g_test_add_func("/token/error", test_err);

	return g_test_run();
}
