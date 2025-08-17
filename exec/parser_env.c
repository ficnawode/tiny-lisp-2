
#include "parser_env.h"

void parser_env_cleanup_v(void *data)
{
	parser_env_cleanup((ParserEnv *)data);
}

ParserEnv *parser_env_create(ParserEnv *parent)
{
	ParserEnv *e = malloc(sizeof(ParserEnv));
	e->parent = parent;
	e->_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
									node_free_v);
	e->_children =
		g_ptr_array_new_with_free_func(parser_env_cleanup_v);
	if (e->parent)
	{
		g_ptr_array_add(e->parent->_children, e);
	}
	return e;
}

void parser_env_emplace(ParserEnv *env, char *name, Node *value)
{
	g_hash_table_insert(env->_map, strdup(name), node_copy(value));
}

Node *parser_env_lookup(ParserEnv *env, const char *name)
{
	Node *value = (Node *)g_hash_table_lookup(env->_map, name);
	if (value)
	{
		return value;
	}
	else if (env->parent)
	{
		Node *parent_value = parser_env_lookup(env->parent, name);
		if (parent_value)
		{
			parser_env_emplace(env, (char *)name, parent_value);
			return (Node *)g_hash_table_lookup(env->_map, name);
		}
	}
	return NULL;
}

void parser_env_cleanup(ParserEnv *env)
{
	if (!env)
	{
		return;
	}
	g_hash_table_destroy(env->_map);
	g_ptr_array_free(env->_children, TRUE);
	free(env);
	env = NULL;
}