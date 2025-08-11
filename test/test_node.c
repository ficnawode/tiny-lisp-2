#include <glib.h>
#include <stdio.h>

#include "node.h"

static void test_variable(void)
{
	Env *e = env_create(NULL);
	Node *n1 = make_variable("name1", e);
	g_assert_cmpstr("name1", ==, n1->variable.name);
	node_free(n1);
}
// test literals
// test function declaration
// test function call
// test if/else expr
// test env

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/node/variable", test_variable);

	return g_test_run();
}