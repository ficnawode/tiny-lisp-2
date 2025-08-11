#include "node.h"
#include <assert.h>

Node *make_literal_int(int val)
{
	Node *n = malloc(sizeof(Node));
	n->type = NODE_LITERAL;
	n->literal.lit_type = LIT_INT;
	n->literal.i_val = val;
	return n;
}

Node *make_literal_float(double val)
{
	Node *n = malloc(sizeof(Node));
	n->type = NODE_LITERAL;
	n->literal.lit_type = LIT_FLOAT;
	n->literal.f_val = val;
	return n;
}

Node *make_literal_string(char *val)
{
	assert(false && "strings not implemented yet");
}
Node *make_literal_bool(bool val)
{
	assert(false && "booleans not implemented yet");
}

Node *make_variable(char *name, Env *env)
{
	Node *n = malloc(sizeof(Node));
	n->type = NODE_VARIABLE;
	n->variable.env = env;
	n->variable.name = strdup(name);
	assert(n->variable.name && "Out of memory");
	return n;
}

Node *make_function(GPtrArray *params, Node *body, Env *closure_env)
{
	Node *n = malloc(sizeof(Node));
	n->type = NODE_FUNCTION;
	n->function.closure_env = closure_env;
	n->function.body = body;
	n->function.param_names = params;
	return n;
}
Node *make_function_call(Node *fn, GPtrArray *args)
{
	Node *n = malloc(sizeof(Node));
	n->type = NODE_CALL;
	n->call.fn = fn;
	n->call.args = args;
	return n;
}

Node *
make_if_expr(Node *condition, Node *then_branch, Node *else_branch)
{
	Node *n = malloc(sizeof(Node));
	n->type = NODE_IF;
	n->if_expr.condition = condition;
	n->if_expr.then_branch = then_branch;
	n->if_expr.else_branch = else_branch;
	return n;
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
		g_ptr_array_free(node->function.param_names, TRUE);
		free(node->function.body);
		break;
	case NODE_CALL:
		g_ptr_array_free(node->call.args, TRUE);
		node_free(node->call.fn);
		break;
	case NODE_IF:
		node_free(node->if_expr.condition);
		node_free(node->if_expr.then_branch);
		node_free(node->if_expr.else_branch);
		break;
	default:
	}
	node = NULL;
}

static void node_free_v(void *node) { node_free((Node *)node); }

Env *env_create(Env *parent)
{
	Env *e = malloc(sizeof(Env));
	GHashTable *map = g_hash_table_new_full(g_str_hash, g_str_equal,
											g_free, node_free_v);
}

static void
env_copy_entry(gpointer key, gpointer value, gpointer user_data)
{
	GHashTable *new_table = user_data;

	char *key_copy = g_strdup((char *)key);

	g_hash_table_insert(new_table, key_copy, value);
}

static GHashTable *hash_table_duplicate(GHashTable *original)
{
	GHashTable *new_table = g_hash_table_new_full(
		g_str_hash, g_str_equal, g_free, node_free_v);
	g_hash_table_foreach(original, env_copy_entry, new_table);
	return new_table;
}

Env *env_copy(Env *parent)
{
	Env *e = malloc(sizeof(Env));
	GHashTable *map = hash_table_duplicate(parent->map);
}

void env_emplace(Env *env, char *name, Node *value)
{
	g_hash_table_insert(env->map, name, value);
}

Node *env_lookup(Env *env, const char *name)
{
	return (Node *)g_hash_table_lookup(env->map, name);
}

void env_cleanup(Env *env) { g_hash_table_destroy(env->map); }