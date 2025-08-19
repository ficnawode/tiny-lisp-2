#pragma once

#include <glib.h>

typedef struct CodeGenEnv CodeGenEnv;

typedef struct
{
	enum VarLocationType
	{
		VAR_LOCATION_STACK,
		VAR_LOCATION_GLOBAL,
		VAR_LOCATION_ENV
	} type;
	union
	{
		int stack_offset;
		const char *global_label;
		int env_index;
	};
} VarLocation;

CodeGenEnv *codegen_env_create(void);
void codegen_env_cleanup(CodeGenEnv *env);
void codegen_env_enter_scope(CodeGenEnv *env);
void codegen_env_exit_scope(CodeGenEnv *env);

/**
 * @brief Adds a variable to the current scope, stored on the stack.
 * @return The assigned stack offset.
 */
int codegen_env_add_stack_variable(CodeGenEnv *env, const char *name);

/**
 * @brief Adds a variable to the global scope.
 * @return The generated global label for the variable. The caller
 * does NOT own this string.
 */
const char *codegen_env_add_global_variable(CodeGenEnv *env,
											const char *name);

/**
 * @brief Adds a variable that is known to be captured from a
 * closure's environment. This function is NOT responsible for adding
 * a lispvalue to a closure.
 * @param env The environment for the function body we are compiling.
 * @param name The name of the free variable.
 * @param index The index of the variable in the runtime environment
 * list.
 */
void codegen_env_add_free_variable(CodeGenEnv *env,
								   const char *name,
								   int index);

/**
 * @brief Looks up a variable's location, searching from the current
 * scope outwards.
 * @return A constant pointer to the VarLocation if found, otherwise
 * NULL.
 */
const VarLocation *codegen_env_lookup(CodeGenEnv *env,
									  const char *name);

/**
 * @brief Resets the stack offset counter. Call this at the start of a
 * new function body.
 */
void codegen_env_reset_stack_offset(CodeGenEnv *env,
									int initial_offset);

int codegen_env_get_stack_offset(CodeGenEnv *env);
void codegen_env_set_stack_offset(CodeGenEnv *env, int offset);
void codegen_env_add_stack_space(CodeGenEnv *env, int bytes);
void codegen_env_remove_stack_space(CodeGenEnv *env, int bytes);