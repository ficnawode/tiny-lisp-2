
#include "parser_env.h"

static void *const DUMMY_SET_VALUE = GINT_TO_POINTER(1);

static void populate_env_with_builtins(ParserEnv *env)
{
	char *builtins[] = {
		"+",	  "-",	"/",   "*",		"=",
		"<",	  ">",	">=",  "<=",	"let",
		"lambda", "if", "def", "quote", "print-debug"};
	int num_elements = sizeof(builtins) / sizeof(builtins[0]);
	for (int i = 0; i < num_elements; i++)
	{
		// Use a placeholder to signify the name is valid.
		parser_env_emplace(env, builtins[i], get_placeholder());
	}
}

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
	populate_env_with_builtins(e);
	e->free_vars =
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
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
	ParserEnv *current_env = env;
	while (current_env != NULL)
	{
		Node *value =
			(Node *)g_hash_table_lookup(current_env->_map, name);
		if (value)
		{
			// If we found the variable in an ancestor scope...
			if (current_env != env)
			{
				// ...and that ancestor scope is NOT the global scope
				// (which has a NULL parent)...
				// ...then it's a true lexical free variable that
				// needs to be captured.
				if (current_env->parent != NULL)
				{
					g_hash_table_insert(env->free_vars,
										g_strdup(name),
										DUMMY_SET_VALUE);
				}
			}
			// We found what we were looking for. Return it directly.
			// No need to copy it into the local environment.
			return value;
		}
		current_env = current_env->parent;
	}
	return NULL; // Not found anywhere.
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