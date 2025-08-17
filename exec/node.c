#include "node.h"
#include <assert.h>

Node *node_create_literal_int(int val)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_LITERAL;
	n->literal.lit_type = LIT_INT;
	n->literal.i_val = val;
	return n;
}

Node *node_create_literal_float(double val)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_LITERAL;
	n->literal.lit_type = LIT_FLOAT;
	n->literal.f_val = val;
	return n;
}

Node *node_create_literal_string(char *val)
{
	assert(false && "strings not implemented yet");
}

Node *node_create_literal_bool(bool val)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_LITERAL;
	n->literal.lit_type = LIT_BOOL;
	n->literal.b_val = val;
	return n;
}

Node *node_create_variable(char *name, ParserEnv *env)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_VARIABLE;
	n->variable.env = env;
	n->variable.name = strdup(name);
	assert(n->variable.name && "Out of memory");
	return n;
}

Node *get_placeholder(void)
{
	static Node n;
	n.type = NODE_PLACEHOLDER;
	return &n;
}

Node *node_create_def(VarBinding *var)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_DEF;
	n->def.binding = var;
	return n;
}

Node *node_create_let(VarBindingArray *bindings,
					  NodeArray *body,
					  ParserEnv *env)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_LET;
	n->let.bindings = bindings;
	n->let.body = body;
	n->let.env = env;
	return n;
}

Node *node_create_function(StringArray *params,
						   NodeArray *body,
						   ParserEnv *closure_env)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_FUNCTION;
	n->function.closure_env = closure_env;
	n->function.body = body;
	n->function.param_names = params;
	return n;
}

Node *node_create_function_call(Node *fn, NodeArray *args)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_CALL;
	n->call.fn = fn;
	n->call.args = args;
	return n;
}

Node *node_create_if_expr(Node *condition,
						  Node *then_branch,
						  Node *else_branch)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_IF;
	n->if_expr.condition = condition;
	n->if_expr.then_branch = then_branch;
	n->if_expr.else_branch = else_branch;
	return n;
}

Node *node_create_quote(Node *quoted_expr)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_QUOTE;
	n->quote.quoted_expr = quoted_expr;
	return n;
}

static void *string_copy_func(const void *src, void *user_data)
{
	(void)user_data;

	return g_strdup((const char *)src);
}

Node *node_copy(const Node *original)
{
	if (!original)
		return NULL;

	if (original->type == NODE_PLACEHOLDER)
		return get_placeholder();

	Node *copy = malloc(sizeof(Node));
	if (!copy)
		return NULL;

	copy->type = original->type;

	switch (original->type)
	{
	case NODE_LITERAL:
		copy->literal = original->literal;
		if (original->literal.lit_type == LIT_STRING &&
			original->literal.s_val)
		{
			copy->literal.s_val = strdup(original->literal.s_val);
		}
		break;
	case NODE_VARIABLE:
		copy->variable.name = strdup(original->variable.name);
		copy->variable.env = original->variable.env;
		break;
	case NODE_FUNCTION:
		copy->function.param_names =
			string_array_copy(original->function.param_names);
		copy->function.body =
			node_array_copy(original->function.body);
		copy->function.closure_env = original->function.closure_env;
		break;
	case NODE_LET:
		copy->let.bindings =
			var_binding_array_copy(original->let.bindings);
		copy->let.body = node_array_copy(original->let.body);
		copy->let.env = original->let.env;
		break;
	case NODE_CALL:
		copy->call.fn = node_copy(original->call.fn);
		copy->call.args = node_array_copy(original->call.args);
		break;
	case NODE_DEF:
		copy->def.binding = var_binding_copy(original->def.binding);
		break;
	case NODE_IF:
		copy->if_expr.condition =
			node_copy(original->if_expr.condition);
		copy->if_expr.then_branch =
			node_copy(original->if_expr.then_branch);
		copy->if_expr.else_branch =
			node_copy(original->if_expr.else_branch);
		break;
	case NODE_QUOTE:
		copy->quote.quoted_expr =
			node_copy(original->quote.quoted_expr);
		break;
	case NODE_PLACEHOLDER:
		break;
	default:
		assert(false && "Unhandled node type");
	}

	return copy;
}

void node_free(Node *node)
{
	if (!node)
	{
		return;
	}
	switch (node->type)
	{
	case NODE_LITERAL:
		if (node->literal.lit_type == LIT_STRING)
		{
			free(node->literal.s_val);
		}
		break;
	case NODE_VARIABLE:
		free(node->variable.name);
		break;
	case NODE_FUNCTION:
		string_array_free(node->function.param_names);
		node_array_free(node->function.body);
		break;
	case NODE_CALL:
		node_array_free(node->call.args);
		node_free(node->call.fn);
		break;
	case NODE_IF:
		node_free(node->if_expr.condition);
		node_free(node->if_expr.then_branch);
		node_free(node->if_expr.else_branch);
		break;
	case NODE_DEF:
		var_binding_free(node->def.binding);
		break;
	case NODE_LET:
		var_binding_array_free(node->let.bindings);
		node_array_free(node->let.body);
		break;
	case NODE_QUOTE:
		node_free(node->quote.quoted_expr);
	case NODE_PLACEHOLDER:
		return;
	default:
		assert(false && "Unhandled node type");
	}

	free(node);
	node = NULL;
}

void node_free_v(void *data) { node_free((Node *)data); }
