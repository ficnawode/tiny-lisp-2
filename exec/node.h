#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>

#include "parser_env.h"
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
typedef struct VarBinding VarBinding;
typedef struct VarBindingArray VarBindingArray;
typedef struct NodeArray NodeArray;
typedef struct StringArray StringArray;

typedef struct ParserEnv ParserEnv;

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
			ParserEnv *env;
		} variable;

		struct
		{
			StringArray *param_names;
			StringArray *free_var_names;
			NodeArray *body;
			ParserEnv *closure_env;
		} function;

		struct
		{
			Node *fn;
			NodeArray *args;
		} call;

		struct
		{
			Node *condition;
			Node *then_branch;
			Node *else_branch; // nullable
		} if_expr;

		struct
		{
			VarBinding *binding;
		} def;

		struct
		{
			VarBindingArray *bindings;
			NodeArray *body;
			ParserEnv *env;
		} let;

		struct
		{
			Node *quoted_expr;
		} quote;
	};
} Node;

Node *node_create_literal_int(int val);
Node *node_create_literal_float(double val);
Node *node_create_literal_string(char *val);
Node *node_create_literal_bool(bool val);
Node *node_create_variable(char *name, ParserEnv *env);
Node *node_create_def(VarBinding *var);

Node *node_create_let(VarBindingArray *bindings,
					  NodeArray *body,
					  ParserEnv *env);

Node *node_create_function(StringArray *params,
						   StringArray *free_vars,
						   NodeArray *body,
						   ParserEnv *closure_env);
Node *node_create_function_call(Node *fn, NodeArray *args);
Node *node_create_if_expr(Node *condition,
						  Node *then_branch,
						  Node *else_branch);
Node *node_create_quote(Node *quoted_expr);
Node *get_placeholder(void);
void node_free(Node *node);
void node_free_v(void *node);
Node *node_copy(const Node *node);
