#include "containers.h"

NodeArray *node_array_new(void)
{
	NodeArray *array = malloc(sizeof(NodeArray));
	array->_array = g_ptr_array_new_with_free_func(node_free_v);
	return array;
}

void node_array_add(NodeArray *array, Node *element)
{
	g_ptr_array_add(array->_array, element);
}

Node *node_array_index(NodeArray *array, int index)
{
	return g_ptr_array_index(array->_array, index);
}

NodeArray *node_array_copy(NodeArray *original)
{
	NodeArray *copy = node_array_new();
	if (!original || !copy)
	{
		return NULL;
	}
	for (int i = 0; i < original->_array->len; i++)
	{
		node_array_add(copy,
					   node_copy(node_array_index(original, i)));
	}
	return copy;
}

void node_array_free(NodeArray *array)
{
	g_ptr_array_free(array->_array, TRUE);
	free(array);
}

StringArray *string_array_new(void)
{
	StringArray *array = malloc(sizeof(StringArray));
	array->_array = g_ptr_array_new_with_free_func(free);
	return array;
}

void string_array_add(StringArray *array, char *element)
{
	g_ptr_array_add(array->_array, strdup(element));
}

char *string_array_index(StringArray *array, int index)
{
	return g_ptr_array_index(array->_array, index);
}

StringArray *string_array_copy(StringArray *original)
{
	StringArray *copy = string_array_new();
	if (!original || !copy)
	{
		return NULL;
	}
	for (int i = 0; i < original->_array->len; i++)
	{
		string_array_add(copy, string_array_index(original, i));
	}
	return copy;
}

void string_array_free(StringArray *array)
{
	g_ptr_array_free(array->_array, TRUE);
	free(array);
}

VarBinding *var_binding_create(char *name, Node *value_expr)
{
	VarBinding *pair = malloc(sizeof(VarBinding));
	pair->name = strdup(name);
	pair->value_expr = node_copy(value_expr);
	return pair;
}

VarBinding *var_binding_copy(const VarBinding *original)
{
	return var_binding_create(original->name, original->value_expr);
}

void var_binding_free(VarBinding *pair)
{
	free(pair->name);
	node_free(pair->value_expr);
	free(pair);
}

static void var_binding_free_v(void *data)
{
	var_binding_free((VarBinding *)data);
}

VarBindingArray *var_binding_array_new(void)
{

	VarBindingArray *array = malloc(sizeof(VarBindingArray));
	array->_array =
		g_ptr_array_new_with_free_func(var_binding_free_v);
	return array;
}

void var_binding_array_add(VarBindingArray *array,
						   VarBinding *element)
{
	return g_ptr_array_add(array->_array, element);
}

void var_binding_create_and_emplace(VarBindingArray *array,
									char *name,
									Node *val)
{
	return g_ptr_array_add(array->_array,
						   var_binding_create(name, val));
}
VarBinding *var_binding_array_index(VarBindingArray *array, int index)
{
	return g_ptr_array_index(array->_array, index);
}

VarBindingArray *var_binding_array_copy(VarBindingArray *original)
{
	VarBindingArray *copy = var_binding_array_new();
	if (!original || !copy)
	{
		return NULL;
	}
	for (int i = 0; i < original->_array->len; i++)
	{
		var_binding_array_add(
			copy,
			var_binding_copy(var_binding_array_index(original, i)));
	}
	return copy;
}

void var_binding_array_free(VarBindingArray *array)
{
	g_ptr_array_free(array->_array, TRUE);
	free(array);
}