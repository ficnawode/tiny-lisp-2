#pragma once
#include "node.h"

typedef struct Node Node;

typedef struct NodeList
{
	GPtrArray *_array;
} NodeList;

NodeList *node_list_create(void);
void node_list_emplace(NodeList *list, Node *element);
Node *node_list_index(NodeList *list, int index);
NodeList *node_list_copy(NodeList *list);
void node_list_cleanup(NodeList *list);

typedef struct StringList
{
	GPtrArray *_array;
} StringList;

StringList *string_list_create(void);
void string_list_emplace(StringList *list, char *element);
char *string_list_index(StringList *list, int index);
StringList *string_list_copy(StringList *list);
void string_list_cleanup(StringList *list);

typedef struct VarPair
{
	char *name;
	Node *value_expr;
} VarPair;

VarPair *var_pair_create(char *name, Node *value_expr);
VarPair *var_pair_copy(const VarPair *original);
void var_pair_free(VarPair *data);

typedef struct VarPairList
{
	GPtrArray *_array;
} VarPairList;

VarPairList *var_pair_list_create(void);
void var_pair_list_emplace(VarPairList *list, VarPair *element);
void var_pair_create_and_emplace(VarPairList *list,
								 char *key,
								 Node *val);
VarPair *var_pair_list_index(VarPairList *list, int index);
VarPairList *var_pair_list_copy(VarPairList *original);
void var_pair_list_cleanup(VarPairList *list);