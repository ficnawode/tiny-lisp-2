#pragma once

#include "node.h"
#include <glib.h>
typedef struct Node Node;

typedef struct ParserEnv
{
	struct ParserEnv *parent;
	GHashTable *_map;

	// A set of names of free variables identified within this scope.
	// Only relevant for function body environments.
	GHashTable *free_vars;

	// keep track of children -> only free parent environment, the
	// rest will be freed automatically
	GPtrArray *_children;
} ParserEnv;

ParserEnv *parser_env_create(ParserEnv *parent);

// Places the binding into a hashmap for lookup
// Deep copies both the name and the node
void parser_env_emplace(ParserEnv *env, char *name, Node *value);

Node *parser_env_lookup(ParserEnv *env, const char *name);
void parser_env_cleanup(ParserEnv *env);