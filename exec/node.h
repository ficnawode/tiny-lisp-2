#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>

#include "util/containers.h"

typedef enum NodeType
{
	NODE_LITERAL,
	NODE_LET,
	NODE_DEF,
	NODE_VARIABLE,
	NODE_FUNCTION,
	NODE_CALL,
	NODE_IF,
	NODE_QUOTE,
	NODE_PLACEHOLDER
} NodeType;

typedef enum LiteralType
{
	LIT_INT,
	LIT_FLOAT,
	LIT_STRING,
	LIT_BOOL
} LiteralType;

typedef struct Node Node;
typedef struct VarPair VarPair;
typedef struct VarPairList VarPairList;
typedef struct NodeList NodeList;
typedef struct StringList StringList;

typedef struct Env Env;

typedef struct Node
{
	NodeType type;
	union
	{
		struct
		{
			LiteralType lit_type;
			union
			{
				int i_val;
				double f_val;
				char *s_val;
				bool b_val;
			};
		} literal;

		struct
		{
			char *name;
			Env *env;
		} variable;

		struct
		{
			StringList *param_names;
			NodeList *body;
			Env *closure_env;
		} function;

		struct
		{
			Node *fn;
			NodeList *args;
		} call;

		struct
		{
			Node *condition;
			Node *then_branch;
			Node *else_branch; // nullable
		} if_expr;

		struct
		{
			VarPair *binding;
		} def;

		struct
		{
			VarPairList *bindings;
			NodeList *body;
			Env *env;
		} let;

		struct
		{
			Node *quoted_expr;
		} quote;
	};
} Node;

typedef struct Env
{
	struct Env *parent;
	GHashTable *_map;

	// if we keep track of children, it will be easier to free memory
	GPtrArray *_children;
} Env;

Node *make_literal_int(int val);
Node *make_literal_float(double val);
Node *make_literal_string(char *val);
Node *make_literal_bool(bool val);
Node *make_variable(char *name, Env *env);
Node *make_def(VarPair *var);

Node *make_let(VarPairList *bindings, NodeList *body, Env *env);

Node *
make_function(StringList *params, NodeList *body, Env *closure_env);
Node *make_function_call(Node *fn, NodeList *args);
Node *
make_if_expr(Node *condition, Node *then_branch, Node *else_branch);
Node *make_quote(Node *quoted_expr);
Node *get_placeholder(void);
void node_free(Node *node);
void node_free_v(void *node);
Node *node_copy(const Node *node);

Env *env_create(Env *parent);
Env *env_copy(Env *parent);
void env_emplace(Env *env, char *name, Node *value);
Node *env_lookup(Env *env, const char *name);
void env_cleanup(Env *env);
