#include <float.h>
#include <glib.h>
#include <stdio.h>

#include "node.h"

static void test_variable(void)
{
	Env *env = env_create(NULL);
	Node *node = make_variable("name1", env);
	g_assert_cmpstr("name1", ==, node->variable.name);
	node_free(node);
	env_cleanup(env);
}

static void _validate_float(double val)
{
	Node *node = make_literal_float(val);
	g_assert_cmpfloat(node->literal.f_val, ==, val);
	node_free(node);
}

static void _validate_int(int val)
{
	Node *node = make_literal_int(val);
	g_assert(node->literal.i_val == val);
	node_free(node);
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
	StringList *params = string_list_create();

	char *p1 = "foo";
	char *p2 = "bar";
	char *p3 = "baz";
	string_list_emplace(params, p1);
	string_list_emplace(params, p2);
	string_list_emplace(params, p3);

	Node *body_element = make_literal_float(3.14159);
	NodeList *body = node_list_create();
	node_list_emplace(body, body_element);
	Env *env = env_create(NULL);
	Node *node = make_function(params, body, env);
	g_assert_cmpstr(p1, ==,
					string_list_index(node->function.param_names, 0));
	g_assert_cmpstr(p2, ==,
					string_list_index(node->function.param_names, 1));
	g_assert_cmpstr(p3, ==,
					string_list_index(node->function.param_names, 2));

	node_free(node);
	env_cleanup(env);
}

static void test_ifexpr(void)
{
	Node *condition = make_literal_int(1);
	Node *then_branch = make_literal_int(2);
	Node *else_branch = make_literal_int(3);
	Node *node = make_if_expr(condition, then_branch, else_branch);

	g_assert(node->if_expr.condition->literal.i_val == 1);
	g_assert(node->if_expr.then_branch->literal.i_val == 2);
	g_assert(node->if_expr.else_branch->literal.i_val == 3);

	node_free(node);
}

static void test_env(void)
{
	Env *env = env_create(NULL);

	Node *p1 = make_literal_int(1);
	Node *p2 = make_literal_int(2);
	Node *p3 = make_literal_int(3);

	env_emplace(env, "p1", p1);
	env_emplace(env, "p2", p2);
	env_emplace(env, "p3", p3);

	// env emplace is a deep copy of both string and node
	node_free(p1);
	node_free(p2);
	node_free(p3);

	Node *p1_lookup = env_lookup(env, "p1");
	Node *p2_lookup = env_lookup(env, "p2");
	Node *p3_lookup = env_lookup(env, "p3");

	g_assert(p1_lookup->literal.i_val == 1);
	g_assert(p2_lookup->literal.i_val == 2);
	g_assert(p3_lookup->literal.i_val == 3);

	env_cleanup(env);
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