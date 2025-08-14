#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>

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
typedef struct Env Env;

typedef struct LetBinding
{
	char *name;
	Node *value_expr;
} LetBinding;

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
			GPtrArray *param_names; // list of char*
			GPtrArray *body;		// list of Node*
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

		struct
		{
			char *name;
			Node *value;
		} def;

		struct
		{
			GPtrArray *bindings; // array of LetBinding*
			GPtrArray *body;	 // array of Node*
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
	GHashTable *map;
	struct Env *parent;

	// if we keep track of children, it will be easier to free memory
	GPtrArray *children;
} Env;

Node *make_literal_int(int val);
Node *make_literal_float(double val);
Node *make_literal_string(char *val);
Node *make_literal_bool(bool val);
Node *make_variable(char *name, Env *env);
Node *make_def(char *name, Node *value);

LetBinding *let_binding_create(char *name, Node *value_expr);
void let_binding_free_v(void *data);
Node *make_let(GPtrArray *bindings, GPtrArray *body, Env *env);

Node *
make_function(GPtrArray *params, GPtrArray *body, Env *closure_env);
Node *make_function_call(Node *fn, GPtrArray *args);
Node *
make_if_expr(Node *condition, Node *then_branch, Node *else_branch);
Node *make_quote(Node *quoted_expr);
Node *make_placeholder(void);
void node_free(Node *node);
void node_free_v(void *node);
Node *node_copy(const Node *node);

Env *env_create(Env *parent);
Env *env_copy(Env *parent);
void env_emplace(Env *env, char *name, Node *value);
Node *env_lookup(Env *env, const char *name);
void env_cleanup(Env *env);