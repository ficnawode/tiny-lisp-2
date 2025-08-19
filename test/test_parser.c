#include <glib.h>

#include "parser.h"

#define SETUP_TEST(source, p_ctx, p_nodes)                           \
	p_ctx = parser_create(source);                                   \
	p_nodes = parser_parse(p_ctx);                                   \
	parser_print_errors(p_ctx)

#define CLEANUP_TEST(p_ctx, p_nodes)                                 \
	node_array_free(p_nodes);                                        \
	parser_cleanup(p_ctx)

static void test_literal_bool(void)
{
	char *source_code = "#t #f";
	ParserContext *parser;
	NodeArray *node_array;

	SETUP_TEST(source_code, parser, node_array);

	g_assert_cmpint(node_array->_array->len, ==, 2);

	Node *n1 = g_ptr_array_index(node_array->_array, 0);
	g_assert_nonnull(n1);
	g_assert_cmpint(n1->type, ==, NODE_LITERAL);
	g_assert_cmpint(n1->literal.lit_type, ==, LIT_BOOL);
	g_assert_true(n1->literal.b_val);

	Node *n2 = g_ptr_array_index(node_array->_array, 1);
	g_assert_nonnull(n2);
	g_assert_cmpint(n2->type, ==, NODE_LITERAL);
	g_assert_cmpint(n2->literal.lit_type, ==, LIT_BOOL);
	g_assert_false(n2->literal.b_val);

	CLEANUP_TEST(parser, node_array);
}

static void test_literal_number(void)
{
	char *source_code = "1 3.1415";
	ParserContext *parser;
	NodeArray *node_array;

	SETUP_TEST(source_code, parser, node_array);

	g_assert_cmpint(node_array->_array->len, ==, 2);

	Node *n1 = node_array_index(node_array, 0);
	g_assert_nonnull(n1);
	g_assert_cmpint(n1->type, ==, NODE_LITERAL);
	g_assert_cmpint(n1->literal.lit_type, ==, LIT_INT);
	g_assert_cmpint(n1->literal.i_val, ==, 1);

	Node *n2 = node_array_index(node_array, 1);
	g_assert_nonnull(n2);
	g_assert_cmpint(n2->type, ==, NODE_LITERAL);
	g_assert_cmpint(n2->literal.lit_type, ==, LIT_FLOAT);
	g_assert_cmpfloat(n2->literal.f_val, ==, 3.1415);

	CLEANUP_TEST(parser, node_array);
}

static void test_funcdef_no_params(void)
{
	char *source_code = "(lambda () 42)";
	ParserContext *parser;
	NodeArray *node_array;

	SETUP_TEST(source_code, parser, node_array);

	g_assert_cmpint(node_array->_array->len, ==, 1);

	Node *func_node = node_array_index(node_array, 0);
	g_assert_nonnull(func_node);
	g_assert_cmpint(func_node->type, ==, NODE_FUNCTION);

	g_assert_nonnull(func_node->function.param_names);
	g_assert_cmpint(func_node->function.param_names->_array->len, ==,
					0);

	NodeArray *body = func_node->function.body;
	g_assert_cmpint(body->_array->len, ==, 1);
	Node *body_node = node_array_index(body, 0);
	g_assert_nonnull(body_node);
	g_assert_cmpint(body_node->type, ==, NODE_LITERAL);
	g_assert_cmpint(body_node->literal.lit_type, ==, LIT_INT);
	g_assert_cmpint(body_node->literal.i_val, ==, 42);

	g_assert_nonnull(func_node->function.closure_env);
	CLEANUP_TEST(parser, node_array);
}

static void test_funcdef_with_params(void)
{
	char *source_code = "(lambda (x y) (+ x y))";
	ParserContext *parser;
	NodeArray *node_array;

	SETUP_TEST(source_code, parser, node_array);

	if (parser->errors->len > 0)
	{
		parser_print_errors(parser);
	}
	g_assert_cmpint(parser->errors->len, ==, 0);

	g_assert_cmpint(node_array->_array->len, ==, 1);

	Node *func_node = node_array_index(node_array, 0);
	g_assert_nonnull(func_node);
	g_assert_cmpint(func_node->type, ==, NODE_FUNCTION);

	StringArray *params = func_node->function.param_names;
	g_assert_nonnull(params);
	g_assert_cmpint(params->_array->len, ==, 2);
	g_assert_cmpstr(string_array_index(params, 0), ==, "x");
	g_assert_cmpstr(string_array_index(params, 1), ==, "y");

	NodeArray *body_list = func_node->function.body;
	g_assert_nonnull(body_list);
	g_assert_cmpint(body_list->_array->len, ==, 1);

	Node *body_expr_node = node_array_index(body_list, 0);
	g_assert_nonnull(body_expr_node);
	g_assert_cmpint(body_expr_node->type, ==, NODE_CALL);

	Node *callable = body_expr_node->call.fn;
	g_assert_nonnull(callable);
	g_assert_cmpint(callable->type, ==, NODE_VARIABLE);
	g_assert_cmpstr(callable->variable.name, ==, "+");

	NodeArray *args = body_expr_node->call.args;
	g_assert_nonnull(args);
	g_assert_cmpint(args->_array->len, ==, 2);

	Node *arg1 = node_array_index(args, 0);
	g_assert_nonnull(arg1);
	g_assert_cmpint(arg1->type, ==, NODE_VARIABLE);
	g_assert_cmpstr(arg1->variable.name, ==, "x");

	Node *arg2 = node_array_index(args, 1);
	g_assert_nonnull(arg2);
	g_assert_cmpint(arg2->type, ==, NODE_VARIABLE);
	g_assert_cmpstr(arg2->variable.name, ==, "y");

	CLEANUP_TEST(parser, node_array);
}

static void test_let_multiple_body_exprs(void)
{
	char *source_code = "(let ((x 10)) (def y 20) (+ x y))";
	ParserContext *parser;
	NodeArray *node_array;

	SETUP_TEST(source_code, parser, node_array);

	if (parser->errors->len > 0)
		parser_print_errors(parser);
	g_assert_cmpint(parser->errors->len, ==, 0);
	g_assert_cmpint(node_array->_array->len, ==, 1);

	Node *let_node = node_array_index(node_array, 0);
	g_assert_nonnull(let_node);
	g_assert_cmpint(let_node->type, ==, NODE_LET);

	g_assert_cmpint(let_node->let.bindings->_array->len, ==, 1);

	NodeArray *body = let_node->let.body;
	g_assert_nonnull(body);
	g_assert_cmpint(body->_array->len, ==, 2);

	Node *def_expr = node_array_index(body, 0);
	g_assert_nonnull(def_expr);
	g_assert_cmpint(def_expr->type, ==, NODE_DEF);
	g_assert_cmpstr(def_expr->def.binding->name, ==, "y");

	Node *call_expr = node_array_index(body, 1);
	g_assert_nonnull(call_expr);
	g_assert_cmpint(call_expr->type, ==, NODE_CALL);

	CLEANUP_TEST(parser, node_array);
}

static void test_def(void)
{
	char *source_code = "(def my-var 123) my-var";
	ParserContext *parser;
	NodeArray *node_array;

	SETUP_TEST(source_code, parser, node_array);

	if (parser->errors->len > 0)
		parser_print_errors(parser);
	g_assert_cmpint(parser->errors->len, ==, 0);

	g_assert_cmpint(node_array->_array->len, ==, 2);

	Node *def_node = node_array_index(node_array, 0);
	g_assert_nonnull(def_node);
	g_assert_cmpint(def_node->type, ==, NODE_DEF);

	g_assert_cmpstr(def_node->def.binding->name, ==, "my-var");

	Node *value_node = def_node->def.binding->value_expr;
	g_assert_nonnull(value_node);
	g_assert_cmpint(value_node->type, ==, NODE_LITERAL);
	g_assert_cmpint(value_node->literal.lit_type, ==, LIT_INT);
	g_assert_cmpint(value_node->literal.i_val, ==, 123);

	Node *var_node = node_array_index(node_array, 1);
	g_assert_nonnull(var_node);
	g_assert_cmpint(var_node->type, ==, NODE_VARIABLE);
	g_assert_cmpstr(var_node->variable.name, ==, "my-var");

	CLEANUP_TEST(parser, node_array);
}

static void test_ifexpr(void)
{
	char *source_code = "(if #t 10 20)";
	ParserContext *parser;
	NodeArray *node_array;

	SETUP_TEST(source_code, parser, node_array);

	if (parser->errors->len > 0)
	{
		parser_print_errors(parser);
	}
	g_assert_cmpint(parser->errors->len, ==, 0);
	g_assert_cmpint(node_array->_array->len, ==, 1);

	Node *if_node = node_array_index(node_array, 0);
	g_assert_nonnull(if_node);
	g_assert_cmpint(if_node->type, ==, NODE_IF);

	Node *cond = if_node->if_expr.condition;
	g_assert_cmpint(cond->type, ==, NODE_LITERAL);
	g_assert_cmpint(cond->literal.lit_type, ==, LIT_BOOL);
	g_assert_true(cond->literal.b_val);

	Node *then_b = if_node->if_expr.then_branch;
	g_assert_cmpint(then_b->type, ==, NODE_LITERAL);
	g_assert_cmpint(then_b->literal.lit_type, ==, LIT_INT);
	g_assert_cmpint(then_b->literal.i_val, ==, 10);

	Node *else_b = if_node->if_expr.else_branch;
	g_assert_cmpint(else_b->type, ==, NODE_LITERAL);
	g_assert_cmpint(else_b->literal.lit_type, ==, LIT_INT);
	g_assert_cmpint(else_b->literal.i_val, ==, 20);

	CLEANUP_TEST(parser, node_array);
}

static void test_def_named_function_recursive(void)
{
	char *source_code = "(def (factorial n) (if (= n 0) 1 (* n "
						"(factorial (- n 1)))))";
	ParserContext *parser;
	NodeArray *node_array;

	SETUP_TEST(source_code, parser, node_array);

	if (parser->errors->len > 0)
	{
		parser_print_errors(parser);
	}
	g_assert_cmpint(parser->errors->len, ==, 0);
	g_assert_cmpint(node_array->_array->len, ==, 1);

	Node *def_node = node_array_index(node_array, 0);
	g_assert_nonnull(def_node);
	g_assert_cmpint(def_node->type, ==, NODE_DEF);
	g_assert_cmpstr(def_node->def.binding->name, ==, "factorial");

	Node *func_node = def_node->def.binding->value_expr;
	g_assert_nonnull(func_node);
	g_assert_cmpint(func_node->type, ==, NODE_FUNCTION);

	StringArray *params = func_node->function.param_names;
	g_assert_nonnull(params);
	g_assert_cmpint(params->_array->len, ==, 1);
	g_assert_cmpstr(string_array_index(params, 0), ==, "n");

	NodeArray *body_list = func_node->function.body;
	g_assert_nonnull(body_list);
	g_assert_cmpint(body_list->_array->len, ==, 1);
	Node *if_node = node_array_index(body_list, 0);
	g_assert_nonnull(if_node);
	g_assert_cmpint(if_node->type, ==, NODE_IF);

	Node *else_branch = if_node->if_expr.else_branch;
	g_assert_nonnull(else_branch);
	g_assert_cmpint(else_branch->type, ==, NODE_CALL);
	g_assert_cmpstr(else_branch->call.fn->variable.name, ==, "*");

	Node *recursive_call =
		node_array_index(else_branch->call.args, 1);
	g_assert_nonnull(recursive_call);
	g_assert_cmpint(recursive_call->type, ==, NODE_CALL);

	Node *recursive_fn = recursive_call->call.fn;
	g_assert_nonnull(recursive_fn);
	g_assert_cmpint(recursive_fn->type, ==, NODE_VARIABLE);
	g_assert_cmpstr(recursive_fn->variable.name, ==, "factorial");

	CLEANUP_TEST(parser, node_array);
}

static void test_closure_free_var_capture(void)
{
	// z is global so it will not be captured
	// y is local so must be captured
	char *source_code =
		"(def z 1) (let ((x 10)) (lambda (y) (+ x y z)))";
	ParserContext *parser;
	NodeArray *node_array;

	SETUP_TEST(source_code, parser, node_array);

	if (parser->errors->len > 0)
		parser_print_errors(parser);
	g_assert_cmpint(parser->errors->len, ==, 0);

	g_assert_cmpint(node_array->_array->len, ==, 2);
	Node *let_node = node_array_index(node_array, 1);
	g_assert_cmpint(let_node->type, ==, NODE_LET);

	Node *func_node = node_array_index(let_node->let.body, 0);
	g_assert_cmpint(func_node->type, ==, NODE_FUNCTION);

	StringArray *free_vars = func_node->function.free_var_names;
	g_assert_nonnull(free_vars);
	g_assert_cmpint(free_vars->_array->len, ==, 1);
	g_assert_cmpstr(string_array_index(free_vars, 0), ==, "x");

	CLEANUP_TEST(parser, node_array);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);

	// Run these tests using valgrind to look for memory leaks

	g_test_add_func("/parser/bool", test_literal_bool);
	g_test_add_func("/parser/number", test_literal_number);
	g_test_add_func("/parser/funcdef/no_params",
					test_funcdef_no_params);
	g_test_add_func("/parser/funcdef/with_params",
					test_funcdef_with_params);
	g_test_add_func("/parser/closure/free_var_capture",
					test_closure_free_var_capture);
	g_test_add_func("/parser/let", test_let_multiple_body_exprs);
	g_test_add_func("/parser/def", test_def);
	g_test_add_func("/parser/deffunc",
					test_def_named_function_recursive);
	g_test_add_func("/parser/if", test_ifexpr);

	return g_test_run();
}