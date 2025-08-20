#pragma once
#include "node.h"

typedef struct Node Node;

typedef struct NodeArray
{
	GPtrArray *_array;
} NodeArray;

NodeArray *node_array_new(void);
void node_array_add(NodeArray *array, Node *element);
Node *node_array_index(NodeArray *list, int index);
NodeArray *node_array_copy(NodeArray *list);
void node_array_free(NodeArray *list);

typedef struct StringArray
{
	GPtrArray *_array;
} StringArray;

StringArray *string_array_new(void);
void string_array_add(StringArray *array, char *element);
char *string_array_index(StringArray *array, int index);
StringArray *string_array_copy(StringArray *array);
void string_array_sort(StringArray *array);
void string_array_free(StringArray *array);

typedef struct VarBinding
{
	char *name;
	Node *value_expr;
} VarBinding;

VarBinding *var_binding_create(char *name, Node *value_expr);
VarBinding *var_binding_copy(const VarBinding *original);
void var_binding_free(VarBinding *binding);

typedef struct VarBindingArray
{
	GPtrArray *_array;
} VarBindingArray;

VarBindingArray *var_binding_array_new(void);
void var_binding_array_add(VarBindingArray *array,
						   VarBinding *element);
VarBinding *var_binding_array_index(VarBindingArray *array,
									int index);
VarBindingArray *var_binding_array_copy(VarBindingArray *original);
void var_binding_array_free(VarBindingArray *array);

typedef struct StringToStringMap
{
	GHashTable *_map;
} StringToStringMap;

StringToStringMap *string_to_string_map_new(void);
void string_to_string_map_insert(StringToStringMap *map,
								 const char *key,
								 const char *value);
const char *string_to_string_map_lookup(StringToStringMap *map,
										const char *key);
void string_to_string_map_free(StringToStringMap *map);