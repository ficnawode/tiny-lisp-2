
// src/codegen_env.c
#include "codegen_env.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>

struct CodeGenEnv
{
	GPtrArray *scope_stack;
	int current_stack_offset;
};

static void free_var_location(VarLocation *location)
{
	if (location->type == VAR_LOCATION_GLOBAL)
	{
		free((char *)location->global_label);
	}
	free(location);
}

static void free_var_location_v(gpointer data)
{
	free_var_location((VarLocation *)data);
}

CodeGenEnv *codegen_env_create(void)
{
	CodeGenEnv *env = g_new(CodeGenEnv, 1);
	env->scope_stack = g_ptr_array_new_with_free_func(
		(GDestroyNotify)g_hash_table_destroy);
	codegen_env_enter_scope(env);
	codegen_env_reset_stack_offset(env, 0);
	return env;
}

void codegen_env_cleanup(CodeGenEnv *env)
{
	if (!env)
		return;
	g_ptr_array_free(env->scope_stack, TRUE);
	g_free(env);
}

void codegen_env_enter_scope(CodeGenEnv *env)
{
	GHashTable *new_scope = g_hash_table_new_full(
		g_str_hash, g_str_equal, g_free, free_var_location_v);
	g_ptr_array_add(env->scope_stack, new_scope);
}

void codegen_env_exit_scope(CodeGenEnv *env)
{
	assert(env->scope_stack->len > 1 &&
		   "Cannot exit the global scope!");
	g_ptr_array_remove_index(env->scope_stack,
							 env->scope_stack->len - 1);
}

void codegen_env_reset_stack_offset(CodeGenEnv *env,
									int initial_offset)
{
	env->current_stack_offset = initial_offset;
}

int codegen_env_add_stack_variable(CodeGenEnv *env, const char *name)
{
	env->current_stack_offset -= 8; // Decrement first, then assign

	VarLocation *loc = g_new(VarLocation, 1);
	loc->type = VAR_LOCATION_STACK;
	loc->stack_offset = env->current_stack_offset;

	GHashTable *current_scope = g_ptr_array_index(
		env->scope_stack, env->scope_stack->len - 1);
	g_hash_table_insert(current_scope, g_strdup(name), loc);

	return loc->stack_offset;
}

static char *sanitize_for_label(const char *input)
{
	if (!input)
		return NULL;

	size_t len = strlen(input);
	char *sanitized = malloc(len + 1);
	assert(sanitized && "Out of memory");

	for (size_t i = 0; i < len; i++)
	{
		char c = input[i];
		if (isalnum(c) || c == '_')
		{
			sanitized[i] = c;
		}
		else
		{
			sanitized[i] = '_';
		}
	}
	sanitized[len] = '\0';
	return sanitized;
}

const char *codegen_env_add_global_variable(CodeGenEnv *env,
											const char *name)
{
	VarLocation *loc = g_new(VarLocation, 1);
	loc->type = VAR_LOCATION_GLOBAL;
	char *sanitized_name = sanitize_for_label(name);
	loc->global_label =
		g_strdup_printf("global_var_%s", sanitized_name);
	free(sanitized_name);

	GHashTable *global_scope = g_ptr_array_index(env->scope_stack, 0);
	g_hash_table_insert(global_scope, g_strdup(name), loc);
	return loc->global_label;
}

void codegen_env_add_free_variable(CodeGenEnv *env,
								   const char *name,
								   int index)
{
	VarLocation *loc = g_new(VarLocation, 1);
	loc->type = VAR_LOCATION_ENV;
	loc->env_index = index;

	// A free variable is added to the *current* function's top-level
	// scope.
	GHashTable *current_scope = g_ptr_array_index(
		env->scope_stack, env->scope_stack->len - 1);
	g_hash_table_insert(current_scope, g_strdup(name), loc);
}

const VarLocation *codegen_env_lookup(CodeGenEnv *env,
									  const char *name)
{
	for (int i = env->scope_stack->len - 1; i >= 0; --i)
	{
		GHashTable *scope = g_ptr_array_index(env->scope_stack, i);
		const VarLocation *loc = g_hash_table_lookup(scope, name);
		if (loc)
		{
			return loc;
		}
	}
	return NULL;
}

int codegen_env_get_stack_offset(CodeGenEnv *env)
{
	return env->current_stack_offset;
}

void codegen_env_set_stack_offset(CodeGenEnv *env, int offset)
{
	env->current_stack_offset = offset;
}

void codegen_env_add_stack_space(CodeGenEnv *env, int bytes)
{
	assert(bytes % 8 == 0 && "stack space must be multiple of 8");
	env->current_stack_offset -= bytes;
}

void codegen_env_remove_stack_space(CodeGenEnv *env, int bytes)
{
	assert(bytes % 8 == 0 && "stack space must be multiple of 8");
	env->current_stack_offset += bytes;
}