#include "containers.h"

NodeList *node_list_create(void)
{
	// note: no need for malloc since address of list is address of
	// overall wrapper struct
	NodeList *list = malloc(sizeof(NodeList));
	list->_array = g_ptr_array_new_with_free_func(node_free_v);
	return list;
}

void node_list_emplace(NodeList *list, Node *element)
{
	g_ptr_array_add(list->_array, node_copy(element));
}

Node *node_list_index(NodeList *list, int index)
{
	return g_ptr_array_index(list->_array, index);
}

NodeList *node_list_copy(NodeList *original)
{
	NodeList *copy = node_list_create();
	if (!original || !copy)
	{
		return NULL;
	}
	for (int i = 0; i < original->_array->len; i++)
	{
		node_list_emplace(copy,
						  node_copy(node_list_index(original, i)));
	}
	return copy;
}

void node_list_cleanup(NodeList *list)
{
	g_ptr_array_free(list->_array, TRUE);
}

StringList *string_list_create(void)
{
	StringList *list = malloc(sizeof(StringList));
	list->_array = g_ptr_array_new_with_free_func(free);
	return list;
}

void string_list_emplace(StringList *list, char *element)
{
	g_ptr_array_add(list->_array, strdup(element));
}

char *string_list_index(StringList *list, int index)
{
	return g_ptr_array_index(list->_array, index);
}

StringList *string_list_copy(StringList *original)
{
	StringList *copy = string_list_create();
	if (!original || !copy)
	{
		return NULL;
	}
	for (int i = 0; i < original->_array->len; i++)
	{
		string_list_emplace(copy, string_list_index(original, i));
	}
	return copy;
}

void string_list_cleanup(StringList *list)
{
	g_ptr_array_free(list->_array, TRUE);
}

VarPair *var_pair_create(char *name, Node *value_expr)
{
	VarPair *pair = malloc(sizeof(VarPair));
	pair->name = strdup(name);
	pair->value_expr = node_copy(value_expr);
	return pair;
}

VarPair *var_pair_copy(const VarPair *original)
{
	return var_pair_create(original->name, original->value_expr);
}

void var_pair_free(VarPair *pair)
{
	free(pair->name);
	node_free(pair->value_expr);
}

static void var_pair_free_v(void *data)
{
	var_pair_free((VarPair *)data);
}

VarPairList *var_pair_list_create(void)
{

	VarPairList *list = malloc(sizeof(VarPairList));
	list->_array = g_ptr_array_new_with_free_func(var_pair_free_v);
	return list;
}

void var_pair_list_emplace(VarPairList *list, VarPair *element)
{
	return g_ptr_array_add(list->_array, var_pair_copy(element));
}

void var_pair_create_and_emplace(VarPairList *list,
								 char *name,
								 Node *val)
{
	return g_ptr_array_add(list->_array, var_pair_create(name, val));
}
VarPair *var_pair_list_index(VarPairList *list, int index)
{
	return g_ptr_array_index(list->_array, index);
}

VarPairList *var_pair_list_copy(VarPairList *original)
{
	VarPairList *copy = var_pair_list_create();
	if (!original || !copy)
	{
		return NULL;
	}
	for (int i = 0; i < original->_array->len; i++)
	{
		var_pair_list_emplace(copy, var_pair_list_index(original, i));
	}
	return copy;
}

void var_pair_list_cleanup(VarPairList *list)
{
	g_ptr_array_free(list->_array, TRUE);
}