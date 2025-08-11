#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum NodeType
{
	NODE_LITERAL,
	NODE_VARIABLE,
	NODE_FUNCTION,
	NODE_CALL,
	NODE_IF
} NodeType;

typedef enum LiteralType
{
	LIT_INT,
	LIT_FLOAT,
	LIT_STRING,
	LIT_BOOL
} LiteralType;

typedef struct Node Node;
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

		// note: functions have no names, they are values and can be
		// assigned to variables.
		struct
		{
			GPtrArray *param_names; // list of char*
			Node *body;
			Env *closure_env;
		} function;

		struct
		{
			Node *fn;
			GPtrArray *args; // Node*
		} call;

		struct
		{
			Node *condition;
			Node *then_branch;
			Node *else_branch; // nullable
		} if_expr;
	};
} Node;

typedef struct Env
{
	GHashTable *map;
	struct Env *parent;
} Env;

Node *make_literal_int(int val);
Node *make_literal_float(double val);
Node *make_literal_string(char *val);
Node *make_literal_bool(bool val);
Node *make_variable(char *name, Env *env);
Node *make_function(GPtrArray *params, Node *body, Env *closure_env);
Node *make_function_call(Node *fn, GPtrArray *args);
Node *
make_if_expr(Node *condition, Node *then_branch, Node *else_branch);
void node_free(Node *node);

// Env
Env *env_create(Env *parent);
Env *env_copy(Env *parent);
void env_emplace(Env *env, char *name, Node *value);
Node *env_lookup(Env *env, const char *name);