#include <float.h>
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

static void _validate_float(double val)
{
	Node *n = make_literal_float(val);
	g_assert_cmpfloat(n->literal.f_val, ==, val);
	node_free(n);
}

static void _validate_int(int val)
{
	Node *n = make_literal_int(val);
	g_assert(n->literal.i_val == val);
	node_free(n);
}

static void test_literals(void)
{
	_validate_float(9.0);
	_validate_float(DBL_MAX);
	_validate_float(DBL_MIN);
	_validate_float(0);
	_validate_float(2.);
	_validate_float(.4);
	_validate_float(DBL_EPSILON);
	_validate_float(-0);

	_validate_int(9);
	_validate_int(INT_MAX);
	_validate_int(INT_MIN);
	_validate_int(0);
	_validate_int(2);
	_validate_int(-4);
	_validate_int(-0);
}

static void test_func(void)
{
	GPtrArray *params = g_ptr_array_new_with_free_func(g_free);

	char *p1 = strdup("foo");
	char *p2 = strdup("bar");
	char *p3 = strdup("baz");

	g_ptr_array_add(params, p1);
	g_ptr_array_add(params, p2);
	g_ptr_array_add(params, p3);

	Node *body = make_literal_float(3.14159);
	Env *env = env_create(NULL);
	Node *n = make_function(params, body, env);
	g_assert_cmpstr(p1, ==,
					g_ptr_array_index(n->function.param_names, 0));
	g_assert_cmpstr(p2, ==,
					g_ptr_array_index(n->function.param_names, 1));
	g_assert_cmpstr(p3, ==,
					g_ptr_array_index(n->function.param_names, 2));

	node_free(n);
}

static void test_ifexpr(void)
{
	Node *condition = make_literal_int(1);
	Node *then_branch = make_literal_int(2);
	Node *else_branch = make_literal_int(3);
	Node *n = make_if_expr(condition, then_branch, else_branch);

	g_assert(n->if_expr.condition->literal.i_val == 1);
	g_assert(n->if_expr.then_branch->literal.i_val == 2);
	g_assert(n->if_expr.else_branch->literal.i_val == 3);

	node_free(n);
}

static void test_env(void)
{
	Env *e = env_create(NULL);

	Node *p1 = make_literal_int(1);
	Node *p2 = make_literal_int(2);
	Node *p3 = make_literal_int(3);

	env_emplace(e, "p1", p1);
	env_emplace(e, "p2", p2);
	env_emplace(e, "p3", p3);

	Node *p1_lookup = env_lookup(e, "p1");
	Node *p2_lookup = env_lookup(e, "p2");
	Node *p3_lookup = env_lookup(e, "p3");

	g_assert(p1_lookup->literal.i_val == 1);
	g_assert(p2_lookup->literal.i_val == 2);
	g_assert(p3_lookup->literal.i_val == 3);

	env_cleanup(e);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	// Run these tests using valgrind to look for memory leaks

	g_test_add_func("/node/variable", test_variable);
	g_test_add_func("/node/literals", test_literals);
	g_test_add_func("/node/func", test_func);
	g_test_add_func("/node/ifexpr", test_ifexpr);
	g_test_add_func("/node/env", test_env);

	return g_test_run();
}