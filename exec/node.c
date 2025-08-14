#include "node.h"
#include <assert.h>

Node *make_literal_int(int val)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_LITERAL;
	n->literal.lit_type = LIT_INT;
	n->literal.i_val = val;
	return n;
}

Node *make_literal_float(double val)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
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
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_LITERAL;
	n->literal.lit_type = LIT_BOOL;
	n->literal.b_val = val;
	return n;
}

Node *make_variable(char *name, Env *env)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_VARIABLE;
	n->variable.env = env;
	n->variable.name = strdup(name);
	assert(n->variable.name && "Out of memory");
	return n;
}

Node *make_placeholder(void)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_PLACEHOLDER;
	return n;
}

LetBinding *let_binding_create(char *name, Node *value_expr)
{
	LetBinding *binding = malloc(sizeof(LetBinding));
	assert(binding && "Out of memory");
	binding->name = strdup(name);
	assert(binding->name && "Out of memory");
	binding->value_expr = value_expr;
	return binding;
}

void let_binding_free(LetBinding *binding)
{
	if (!binding)
	{
		return;
	}
	free(binding->name);
	node_free(binding->value_expr);
	free(binding);
}

void let_binding_free_v(void *data)
{
	let_binding_free((LetBinding *)data);
}

Node *make_def(char *name, Node *value)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_DEF;
	n->def.name = strdup(name);
	assert(n->def.name && "Out of memory");
	n->def.value = value;
	return n;
}

Node *make_let(GPtrArray *bindings, GPtrArray *body, Env *env)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_LET;
	n->let.bindings = bindings;
	n->let.body = body;
	n->let.env = env;
	return n;
}

Node *
make_function(GPtrArray *params, GPtrArray *body, Env *closure_env)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_FUNCTION;
	n->function.closure_env = closure_env;
	n->function.body = body;
	n->function.param_names = params;
	return n;
}

Node *make_function_call(Node *fn, GPtrArray *args)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_CALL;
	n->call.fn = fn;
	n->call.args = args;
	return n;
}

Node *
make_if_expr(Node *condition, Node *then_branch, Node *else_branch)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_IF;
	n->if_expr.condition = condition;
	n->if_expr.then_branch = then_branch;
	n->if_expr.else_branch = else_branch;
	return n;
}

Node *make_quote(Node *quoted_expr)
{
	Node *n = malloc(sizeof(Node));
	assert(n && "Out of memory");
	n->type = NODE_QUOTE;
	n->quote.quoted_expr = quoted_expr;
	return n;
}

static GPtrArray *node_array_copy(const GPtrArray *original_array)
{
	if (!original_array)
		return NULL;
	GPtrArray *new_array =
		g_ptr_array_new_with_free_func(node_free_v);
	for (guint i = 0; i < original_array->len; ++i)
	{
		Node *original_node = g_ptr_array_index(original_array, i);
		g_ptr_array_add(new_array, node_copy(original_node));
	}
	return new_array;
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

	Node *copy = malloc(sizeof(Node));
	if (!copy)
		return NULL;

	copy->type = original->type;

	switch (original->type)
	{
	case NODE_LITERAL:
		copy->literal = original->literal;
		break;
	case NODE_VARIABLE:
		copy->variable.name = strdup(original->variable.name);
		copy->variable.env = original->variable.env;
		break;
	case NODE_FUNCTION:
		copy->function.param_names = g_ptr_array_copy(
			original->function.param_names, string_copy_func, NULL);
		copy->function.body =
			node_array_copy(original->function.body);
		copy->function.closure_env = original->function.closure_env;
		break;
	case NODE_LET:
		copy->let.bindings =
			g_ptr_array_new_with_free_func(node_free_v);
		for (guint i = 0; i < original->let.bindings->len; ++i)
		{
			LetBinding *orig_b =
				g_ptr_array_index(original->let.bindings, i);
			LetBinding *new_b = let_binding_create(
				strdup(orig_b->name), node_copy(orig_b->value_expr));
			g_ptr_array_add(copy->let.bindings, new_b);
		}
		copy->let.body = node_array_copy(original->let.body);
		copy->let.env = original->let.env;
		break;
	case NODE_CALL:
		copy->call.fn = node_copy(original->call.fn);
		copy->call.args = node_array_copy(original->call.args);
		break;
	case NODE_DEF:
		copy->def.name = strdup(original->def.name);
		copy->def.value = node_copy(original->def.value);
		break;
	case NODE_QUOTE:
		copy->quote.quoted_expr;
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
	case NODE_DEF:
		free(node->def.name);
		node_free(node->def.value);
		break;
	case NODE_LET:
		g_ptr_array_free(node->let.bindings, TRUE);
		g_ptr_array_free(node->let.body, TRUE);
		// env_cleanup(node->let.env);
		break;
	case NODE_FUNCTION:
		g_ptr_array_free(node->function.param_names, TRUE);
		g_ptr_array_free(node->function.body, TRUE);
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
	case NODE_QUOTE:
		node_free(node->quote.quoted_expr);
	default:
	}
	free(node);
	node = NULL;
}

void node_free_v(void *data) { node_free((Node *)data); }

void env_cleanup_v(void *data) { env_cleanup((Env *)data); }

Env *env_create(Env *parent)
{
	Env *e = malloc(sizeof(Env));
	e->parent = parent;
	e->map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
								   node_free_v);
	e->children = g_ptr_array_new_with_free_func(env_cleanup_v);
	return e;
}

static void
env_copy_entry(gpointer key, gpointer value, gpointer user_data)
{
	GHashTable *new_table = user_data;
	char *key_copy = g_strdup((char *)key);
	Node *value_copy = node_copy((Node *)value);
	g_hash_table_insert(new_table, key_copy, value_copy);
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
	e->parent = parent;
	e->map = hash_table_duplicate(parent->map);
	e->children = g_ptr_array_new_with_free_func(env_cleanup_v);
	if (e->parent)
	{
		g_ptr_array_add(e->parent->children, e);
	}
	return e;
}

void env_emplace(Env *env, char *name, Node *value)
{
	g_hash_table_insert(env->map, strdup(name), node_copy(value));
}
Node *env_lookup(Env *env, const char *name)
{
	return (Node *)g_hash_table_lookup(env->map, name);
}

void env_cleanup(Env *env)
{
	if (!env)
	{
		return;
	}
	g_hash_table_destroy(env->map);
	g_ptr_array_free(env->children, TRUE);
	free(env);
	env = NULL;
}