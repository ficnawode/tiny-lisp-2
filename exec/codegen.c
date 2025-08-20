#include "codegen.h"
#include "asm_emitter.h"
#include <assert.h>
#include <stdarg.h>

#include "lispvalue.h"

static CodeGenContext *
codegen_context_create(const char *output_prefix);
static void codegen_context_cleanup(CodeGenContext *ctx);
static void generate_node(CodeGenContext *ctx, Node *node);
static void generate_literal(CodeGenContext *ctx, Node *node);
static void generate_def(CodeGenContext *ctx, Node *node);
static void generate_variable(CodeGenContext *ctx, Node *node);
static void generate_if(CodeGenContext *ctx, Node *node);
static void generate_let(CodeGenContext *ctx, Node *node);
static void generate_call(CodeGenContext *ctx, Node *node);
static void generate_function_impl(CodeGenContext *ctx,
								   Node *node,
								   const char *self_name);
static void generate_function(CodeGenContext *ctx, Node *node);

static const enum Register ARGUMENT_REGS[] = {
	REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9};
static const char *ARGUMENT_REGISTERS[] = {"rdi", "rsi", "rdx",
										   "rcx", "r8",	 "r9"};
static const int NUM_ARGUMENT_REGISTERS = 6;

int get_next_label(void)
{
	static int label_number = 0;
	return ++label_number;
}

static inline StringToStringMap *
create_and_populate_builtin_func_map(void)
{
	StringToStringMap *map = string_to_string_map_new();
	string_to_string_map_insert(map, "print-debug", "lisp_print");
	string_to_string_map_insert(map, "+", "lisp_add");
	string_to_string_map_insert(map, "-", "lisp_subtract");
	string_to_string_map_insert(map, "*", "lisp_multiply");
	string_to_string_map_insert(map, "=", "lisp_equal");
	return map;
}

static CodeGenContext *
codegen_context_create(const char *output_prefix)
{
	CodeGenContext *ctx = malloc(sizeof(CodeGenContext));
	ctx->writer = asm_file_writer_create(output_prefix);
	if (!ctx->writer)
	{
		free(ctx);
		return NULL;
	}

	ctx->builtin_func_map = create_and_populate_builtin_func_map();
	ctx->env = codegen_env_create();
	codegen_env_enter_scope(ctx->env);
	return ctx;
}

static void codegen_context_cleanup(CodeGenContext *ctx)
{
	if (!ctx)
		return;
	codegen_env_cleanup(ctx->env);
	string_to_string_map_free(ctx->builtin_func_map);

	g_free(ctx);
}

static void emit_extern_for_builtin(gpointer key,
									gpointer value,
									gpointer user_data)
{
	(void)key;

	const char *c_function_label = (const char *)value;
	CodeGenContext *ctx = (CodeGenContext *)user_data;

	emit_extern(ctx->writer, value, "");
}

static inline void write_prologue(CodeGenContext *ctx)
{
	char *core_runtime_functions[] = {
		"lispvalue_create_int",	 "lispvalue_create_float",
		"lispvalue_create_bool", "lispvalue_create_closure",
		"lispcell_create",		 "lispvalue_create_cell",
		"lisp_is_truthy"};
	int num_elements = sizeof(core_runtime_functions) /
					   sizeof(core_runtime_functions[0]);

	emit_global(ctx->writer, "main", "");

	emit_comment(ctx->writer,
				 "; Core runtime functions declared extern");
	for (int i = 0; i < num_elements; i++)
	{
		emit_extern(ctx->writer, core_runtime_functions[i], "");
	}

	emit_comment(ctx->writer, "; Builtin functions declared extern");
	g_hash_table_foreach(ctx->builtin_func_map->_map,
						 emit_extern_for_builtin, ctx);
	emit_label(ctx->writer, "main", "");
	emit_push_reg(ctx->writer, REG_RBP, "");
	emit_mov_reg_reg(ctx->writer, REG_RBP, REG_RSP, "");
}

static inline void write_epilogue(CodeGenContext *ctx)
{
	emit_mov_reg_imm(ctx->writer, REG_RAX, 60, "arg1");
	emit_mov_reg_imm(ctx->writer, REG_RDI, 0, "arg2");
	emit_syscall(ctx->writer, "");
}

void codegen_compile_program(NodeArray *ast,
							 const char *output_prefix)
{
	CodeGenContext *ctx = codegen_context_create(output_prefix);
	if (!ctx)
		return;

	write_prologue(ctx);

	for (guint i = 0; i < ast->_array->len; i++)
	{
		codegen_env_reset_stack_offset(ctx->env, 0);
		Node *node = g_ptr_array_index(ast->_array, i);
		generate_node(ctx, node);
	}
	write_epilogue(ctx);

	asm_file_writer_consolidate(ctx->writer);
	asm_file_writer_cleanup(ctx->writer);
	codegen_context_cleanup(ctx);
	return;
}

static void generate_node(CodeGenContext *ctx, Node *node)
{
	switch (node->type)
	{
	case NODE_LITERAL:
		generate_literal(ctx, node);
		break;
	case NODE_DEF:
		generate_def(ctx, node);
		break;
	case NODE_VARIABLE:
		generate_variable(ctx, node);
		break;
	case NODE_IF:
		generate_if(ctx, node);
		break;
	case NODE_LET:
		generate_let(ctx, node);
		break;
	case NODE_CALL:
		generate_call(ctx, node);
		break;
	case NODE_FUNCTION:
		generate_function(ctx, node);
		break;
	default:
		fprintf(stderr,
				"Codegen Error: Unimplemented AST node type %d\n",
				node->type);
		exit(1);
	}
}

static inline void generate_literal_int(CodeGenContext *ctx,
										Node *node)
{
	emit_mov_reg_imm(ctx->writer, REG_RDI, node->literal.i_val,
					 "int literal");
	emit_call_label(ctx->writer, "lispvalue_create_int", "");
}
static inline void generate_literal_float(CodeGenContext *ctx,
										  Node *node)
{
	char *label = g_strdup_printf("L_float_%d", get_next_label());
	double f_val = node->literal.f_val;
	emit_data_dq_float(ctx->writer, label, f_val, "");
	emit_movsd_reg_global(ctx->writer, REG_XMM0, label, "");
	emit_call_label(ctx->writer, "lispvalue_create_float", "");
	g_free(label);
}
static inline void generate_literal_bool(CodeGenContext *ctx,
										 Node *node)
{
	int64_t b_val = node->literal.b_val ? 1 : 0;
	emit_mov_reg_imm(ctx->writer, REG_RDI, b_val, "");
	emit_call_label(ctx->writer, "lispvalue_create_bool", "");
}

static void generate_literal(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_LITERAL);
	switch (node->literal.lit_type)
	{
	case LIT_INT:
		generate_literal_int(ctx, node);
		break;
	case LIT_FLOAT:
		generate_literal_float(ctx, node);
		break;
	case LIT_BOOL:
		generate_literal_bool(ctx, node);
		break;
	default:
		printf("Codegen Error: Unimplemented literal type %d\n",
			   node->literal.lit_type);
		exit(1);
	}
}

static inline void generate_global_function(CodeGenContext *ctx,
											const char *name,
											Node *fn_node)
{
	assert(fn_node->type == NODE_FUNCTION &&
		   "Expected function node");
	const char *label =
		codegen_env_add_global_variable(ctx->env, name);
	emit_data_dq_imm(ctx->writer, label, 0, "");

	generate_function_impl(ctx, fn_node, name);
	emit_mov_global_reg(ctx->writer, label, REG_RAX, "");
}

static inline void generate_global_variable(CodeGenContext *ctx,
											const char *name,
											Node *val)
{
	generate_node(ctx, val);
	const char *label =
		codegen_env_add_global_variable(ctx->env, name);
	emit_data_dq_imm(ctx->writer, label, 0, "");
	emit_mov_global_reg(ctx->writer, label, REG_RAX, "");
}

static void generate_def(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_DEF);
	VarBinding *binding = node->def.binding;
	const char *name = binding->name;
	Node *value_expr = binding->value_expr;

	if (value_expr->type == NODE_FUNCTION)
	{
		generate_global_function(ctx, name, value_expr);
	}
	else
	{
		generate_global_variable(ctx, name, value_expr);
	}
}

static void generate_function_body(CodeGenContext *ctx,
								   Node *node,
								   const char *func_label,
								   const char *self_name)

{
	const char *comment_name = (self_name) ? self_name : "anonymous";
	emit_label(ctx->writer, func_label, "function %s", comment_name);
	emit_push_reg(ctx->writer, REG_RBP, "");
	emit_mov_reg_reg(ctx->writer, REG_RBP, REG_RSP, "");
	codegen_env_enter_scope(ctx->env);
	codegen_env_reset_stack_offset(ctx->env, 0);

	emit_push_reg(ctx->writer, REG_R12, "push the closure pointer");
	codegen_env_add_stack_space(ctx->env, 8);

	StringArray *params = node->function.param_names;
	guint num_params = params->_array->len;
	for (guint i = 0; i < num_params; i++)
	{
		const char *param_name = string_array_index(params, i);
		if (i < NUM_ARGUMENT_REGISTERS)
		{
			emit_push_reg(ctx->writer, ARGUMENT_REGS[i],
						  "arg %d '%s' from register", i, param_name);
			codegen_env_add_stack_variable(ctx->env, param_name);
		}
		else
		{
			const int fixed_prologue_offset =
				sizeof(intptr_t) + sizeof(intptr_t);
			const int argument_size = sizeof(LispValue *);
			int offset_from_rbp =
				fixed_prologue_offset +
				(i - NUM_ARGUMENT_REGISTERS) * argument_size;
			emit_mov_reg_membase(
				ctx->writer, REG_RAX, REG_RBP, offset_from_rbp,
				"load arg %d '%s' from caller stack", i, param_name);
			emit_push_reg(ctx->writer, REG_RAX,
						  "push arg %d '%s' to local stack", i,
						  param_name);
			codegen_env_add_stack_variable(ctx->env, param_name);
		}
	}

	StringArray *free_vars = node->function.free_var_names;
	for (guint i = 0; i < free_vars->_array->len; i++)
	{
		char *name = string_array_index(free_vars, i);
		codegen_env_add_free_variable(ctx->env, name, i);
	}

	NodeArray *body = node->function.body;
	for (guint i = 0; i < body->_array->len; i++)
	{
		generate_node(ctx, g_ptr_array_index(body->_array, i));
	}

	codegen_env_exit_scope(ctx->env);

	emit_pop_reg(ctx->writer, REG_R12, "Restore the closure pointer");
	emit_mov_reg_reg(ctx->writer, REG_RSP, REG_RBP, "");
	emit_pop_reg(ctx->writer, REG_RBP, "");
	emit_ret(ctx->writer, "");
}

static void generate_closure_creation(CodeGenContext *ctx,
									  Node *node,
									  const char *func_label,
									  const char *self_name)
{
	StringArray *free_vars = node->function.free_var_names;
	guint num_free = free_vars->_array->len;

	int rsp_val = codegen_env_get_stack_offset(ctx->env);
	guint free_var_stack_space = num_free * sizeof(LispValue *);
	codegen_env_add_stack_space(ctx->env, free_var_stack_space);

	for (int i = num_free - 1; i >= 0; i--)
	{
		const char *free_var_name = string_array_index(free_vars, i);

		if (self_name && strcmp(free_var_name, self_name) == 0)
		{
			emit_push_imm(ctx->writer, 0,
						  "push self reference for recursion");
		}
		else
		{
			const VarLocation *loc =
				codegen_env_lookup(ctx->env, free_var_name);
			if (!loc)
			{
				printf("Free variable '%s' not found in codegen env",
					   free_var_name);
				exit(1);
			}

			switch (loc->type)
			{
			case VAR_LOCATION_GLOBAL:
				emit_push_global(
					ctx->writer, loc->global_label,
					"push global free var onto the stack");
				break;
			case VAR_LOCATION_STACK:
				emit_mov_reg_membase(ctx->writer, REG_RDI, REG_RBP,
									 loc->stack_offset, "");
				emit_call_label(ctx->writer, "lispcell_create", "");
				emit_mov_reg_reg(ctx->writer, REG_RDI, REG_RAX,
								 "load created lispcell as argument");
				emit_call_label(ctx->writer, "lispvalue_create_cell",
								"");
				emit_push_reg(ctx->writer, REG_RAX,
							  "push stack freevar onto the stack");
				break;
			case VAR_LOCATION_ENV:
				int64_t env_offset =
					sizeof(LispClosureObject) +
					loc->env_index * sizeof(LispValue *);
				emit_mov_reg_membase(
					ctx->writer, REG_RAX, REG_R12, env_offset,
					"load free variable from closure at r12, offset "
					"by LispClosureObject (size=%d) ",
					sizeof(LispClosureObject));
				emit_push_reg(ctx->writer, REG_RAX,
							  "push env free var onto the stack");
				break;
			}
		}
	}

	emit_mov_reg_label(ctx->writer, ARGUMENT_REGS[0], func_label,
					   "arg 1 : function pointer");
	StringArray *params = node->function.param_names;
	guint num_params = params->_array->len;
	emit_mov_reg_imm(ctx->writer, ARGUMENT_REGS[1], num_params,
					 "arg 2: num_params");
	emit_mov_reg_imm(ctx->writer, ARGUMENT_REGS[2], num_free,
					 "arg 3: num_free");

	const enum Register variadic_regs[] = {REG_RCX, REG_R8, REG_R9};
	int num_variadic_regs =
		sizeof(variadic_regs) / sizeof(variadic_regs[0]);

	const guint num_reg_args =
		(num_free < num_variadic_regs) ? num_free : num_variadic_regs;
	for (guint i = 0; i < num_reg_args; i++)
	{
		emit_pop_reg(ctx->writer, variadic_regs[i],
					 "arg %d (%s): pop free var from stack", i + 4,
					 reg_to_string(variadic_regs[i]));
	}

	guint num_stack_args =
		(num_free > num_reg_args) ? (num_free - num_reg_args) : 0;
	guint stack_space_to_clean = num_stack_args * 8;

	emit_xor_reg_reg(
		ctx->writer, REG_RAX, REG_RAX,
		"ABI: zero RAX for variadic call. Otherwise the C ABI "
		"interprets RAX as number of XMM registers used in call");

	emit_call_label(ctx->writer, "lispvalue_create_closure", "");
	if (stack_space_to_clean > 0)
	{
		emit_add_rsp(
			ctx->writer, stack_space_to_clean,
			"take free vars (and alignment padding) off the stack");
		codegen_env_remove_stack_space(ctx->env,
									   stack_space_to_clean);
	}
}

static void generate_function_impl(CodeGenContext *ctx,
								   Node *node,
								   const char *self_name)
{
	assert(node->type == NODE_FUNCTION);

	int original_stack_offset =
		codegen_env_get_stack_offset(ctx->env);
	int func_label_num = get_next_label();
	char *func_label = g_strdup_printf("L_func_%d", func_label_num);
	char *end_func_label =
		g_strdup_printf("L_func_end_%d", func_label_num);

	emit_jmp(ctx->writer, end_func_label, "");
	generate_function_body(ctx, node, func_label, self_name);

	emit_label(ctx->writer, end_func_label, "");
	generate_closure_creation(ctx, node, func_label, self_name);

	codegen_env_set_stack_offset(ctx->env, original_stack_offset);
	g_free(func_label);
	g_free(end_func_label);
}

static inline void
codegen_load_global_variable(CodeGenContext *ctx,
							 const VarLocation *loc)
{
	emit_mov_reg_global(ctx->writer, REG_RAX, loc->global_label,
						"load global variable");
}

static inline void codegen_load_stack_variable(CodeGenContext *ctx,
											   const VarLocation *loc)
{
	emit_mov_reg_membase(ctx->writer, REG_RAX, REG_RBP,
						 loc->stack_offset, "load stack variable");
}

static inline void codegen_load_free_variable(CodeGenContext *ctx,
											  const VarLocation *loc)
{
	int64_t free_var_offset = sizeof(LispClosureObject) +
							  sizeof(LispValue *) * loc->env_index;
	// The base register is R12, which is wrong.
	emit_mov_reg_membase(ctx->writer, REG_RAX, REG_R12,
						 free_var_offset, "load free (env) cell");
	emit_mov_reg_membase(ctx->writer, REG_RAX, REG_RAX,
						 sizeof(LispCell *),
						 "load lispvalue from cell");
	emit_mov_reg_membase(ctx->writer, REG_RAX, REG_RAX, 0, "");
}

static void generate_variable(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_VARIABLE);

	const VarLocation *loc =
		codegen_env_lookup(ctx->env, node->variable.name);

	if (!loc)
	{
		printf("Undefined variable '%s', should have been caught by "
			   "parser?",
			   node->variable.name);
		exit(1);
	}

	switch (loc->type)
	{
	case VAR_LOCATION_GLOBAL:
		codegen_load_global_variable(ctx, loc);
		break;
	case VAR_LOCATION_STACK:
		codegen_load_stack_variable(ctx, loc);
		break;
	case VAR_LOCATION_ENV:
		codegen_load_free_variable(ctx, loc);
		break;
	default:
		printf("Undefined variable type '%d'", loc->type);
		exit(1);
	}
}

static void generate_let(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_LET);

	codegen_env_enter_scope(ctx->env);

	VarBindingArray *bindings = node->let.bindings;
	for (guint i = 0; i < bindings->_array->len; i++)
	{
		VarBinding *binding = g_ptr_array_index(bindings->_array, i);
		generate_node(ctx, binding->value_expr);
		emit_push_reg(ctx->writer, REG_RAX, "push stack variable %s",
					  binding->name);
		codegen_env_add_stack_variable(ctx->env, binding->name);
	}

	NodeArray *body = node->let.body;
	for (guint i = 0; i < body->_array->len; i++)
	{
		Node *body_expr = g_ptr_array_index(body->_array, i);
		generate_node(ctx, body_expr);
	}

	guint num_bindings = bindings->_array->len;
	if (num_bindings > 0)
	{
		guint space_to_reclaim = num_bindings * sizeof(LispValue *);
		emit_add_rsp(ctx->writer, space_to_reclaim,
					 "take let variables off the stack");
		codegen_env_remove_stack_space(ctx->env, space_to_reclaim);
	}

	codegen_env_exit_scope(ctx->env);
}

static void generate_function(CodeGenContext *ctx, Node *node)
{
	generate_function_impl(ctx, node, NULL);
}

static inline int min(int a, int b) { return (a < b) ? a : b; }

static void codegen_push_arguments(CodeGenContext *ctx,
								   NodeArray *args)
{
	guint num_args = args->_array->len;
	for (int i = num_args - 1; i >= 0; i--)
	{
		Node *arg_node = g_ptr_array_index(args->_array, i);
		generate_node(ctx, arg_node);
		emit_push_reg(ctx->writer, REG_RAX, "push arg %d", i);
		codegen_env_add_stack_space(ctx->env, 8);
	}
}

static void codegen_cleanup_stack_args(CodeGenContext *ctx,
									   int num_args)
{
	if (num_args > NUM_ARGUMENT_REGISTERS)
	{
		emit_add_rsp(ctx->writer,
					 (num_args - NUM_ARGUMENT_REGISTERS) *
						 sizeof(LispValue *),
					 "Remove stack-passed arguments");
		codegen_env_remove_stack_space(
			ctx->env, (num_args - NUM_ARGUMENT_REGISTERS) *
						  sizeof(LispValue *));
	}
}

static void generate_standard_builtin_call(
	CodeGenContext *ctx, Node *call_node, const char *builtin_c_label)
{
	NodeArray *args = call_node->call.args;
	int num_args = args->_array->len;

	codegen_push_arguments(ctx, args);

	int num_args_in_regs = min(num_args, NUM_ARGUMENT_REGISTERS);
	for (guint i = 0; i < num_args_in_regs; i++)
	{
		emit_pop_reg(ctx->writer, ARGUMENT_REGS[i],
					 "pop arg %d into register", i + 1);
		codegen_env_remove_stack_space(ctx->env, 8);
	}

	emit_call_label(ctx->writer, builtin_c_label, "");

	codegen_cleanup_stack_args(ctx, num_args);
}

static void generate_variadic_builtin_call(
	CodeGenContext *ctx, Node *call_node, const char *builtin_c_label)
{
	NodeArray *args = call_node->call.args;
	int num_args = args->_array->len;

	codegen_push_arguments(ctx, args);

	emit_pop_reg(ctx->writer, REG_RDI, "pop arg 1 off the stack");
	codegen_env_remove_stack_space(ctx->env, 8);

	emit_pop_reg(ctx->writer, REG_RSI, "pop arg 2 off the stack");
	codegen_env_remove_stack_space(ctx->env, 8);

	emit_call_label(ctx->writer, builtin_c_label, "");

	for (guint i = 2; i < num_args; i++)
	{
		emit_comment(ctx->writer, "chaining variadic call");
		emit_mov_reg_reg(ctx->writer, REG_RDI, REG_RAX,
						 "result becomes arg 1");
		emit_pop_reg(ctx->writer, REG_RSI, "pop arg %d off the stack",
					 i + 1);
		codegen_env_remove_stack_space(ctx->env, 8);
		emit_call_label(ctx->writer, builtin_c_label, "");
	}
}

static void generate_lisp_closure_call(CodeGenContext *ctx,
									   Node *call_node)
{
	NodeArray *args = call_node->call.args;
	int num_args = args->_array->len;

	codegen_push_arguments(ctx, args);

	generate_node(ctx, call_node->call.fn);

	emit_mov_reg_reg(ctx->writer, REG_R12, REG_RAX,
					 "save closure pointer in R12");

	int num_args_in_regs = min(num_args, NUM_ARGUMENT_REGISTERS);
	for (guint i = 0; i < num_args_in_regs; i++)
	{
		emit_pop_reg(ctx->writer, ARGUMENT_REGS[i],
					 "pop arg %d into register", i + 1);
		codegen_env_remove_stack_space(ctx->env, 8);
	}

	emit_mov_reg_membase(ctx->writer, REG_RAX, REG_R12,
						 sizeof(LispValue *),
						 "get code ptr from closure");
	emit_call_reg(ctx->writer, REG_RAX, "call closure");

	codegen_cleanup_stack_args(ctx, num_args);
}

static void generate_builtin_func_call(CodeGenContext *ctx,
									   Node *call_node,
									   const char *builtin_c_label)
{
	const char *op_name = call_node->call.fn->variable.name;
	const int num_args = call_node->call.args->_array->len;
	bool is_variadic_op =
		(strcmp(op_name, "+") == 0 || strcmp(op_name, "*") == 0 ||
		 strcmp(op_name, "-") == 0);

	if (is_variadic_op && num_args > 2)
	{
		generate_variadic_builtin_call(ctx, call_node,
									   builtin_c_label);
	}
	else
	{
		generate_standard_builtin_call(ctx, call_node,
									   builtin_c_label);
	}
}

static void generate_call(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_CALL);

	const char *builtin_c_label = NULL;
	if (node->call.fn->type == NODE_VARIABLE)
	{
		builtin_c_label = string_to_string_map_lookup(
			ctx->builtin_func_map, node->call.fn->variable.name);
	}

	if (builtin_c_label)
	{
		generate_builtin_func_call(ctx, node, builtin_c_label);
	}
	else
	{
		generate_lisp_closure_call(ctx, node);
	}
}

static void generate_if(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_IF);

	int label_num = get_next_label();
	char *else_label = g_strdup_printf("L_else_%d", label_num);
	char *end_label = g_strdup_printf("L_end_if_%d", label_num);

	generate_node(ctx, node->if_expr.condition);
	emit_mov_reg_reg(ctx->writer, REG_RDI, REG_RAX, "load arg 1");
	emit_call_label(ctx->writer, "lisp_is_truthy", "");
	emit_cmp_reg_imm(ctx->writer, REG_RAX, 0, "");
	emit_je(ctx->writer, else_label, "");

	generate_node(ctx, node->if_expr.then_branch);
	emit_jmp(ctx->writer, end_label, "");

	emit_label(ctx->writer, else_label, "");
	if (node->if_expr.else_branch)
	{
		generate_node(ctx, node->if_expr.else_branch);
	}
	else
	{
		emit_xor_reg_reg(ctx->writer, REG_RAX, REG_RAX,
						 "No else branch: zero RAX");
	}

	emit_label(ctx->writer, end_label, "");
	free(else_label);
	free(end_label);
}