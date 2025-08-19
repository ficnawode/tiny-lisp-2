#include "codegen.h"
#include <assert.h>
#include <stdarg.h>

#define EMIT_TEXT(ctx, ...)                                          \
	asm_file_writer_write_text((ctx)->writer, __VA_ARGS__)
#define EMIT_DATA(ctx, ...)                                          \
	asm_file_writer_write_data((ctx)->writer, __VA_ARGS__)

static CodeGenContext *
codegen_context_create(const char *output_prefix);
static void codegen_context_cleanup(CodeGenContext *ctx);
static void codegen_generate_node(CodeGenContext *ctx, Node *node);
static void codegen_generate_literal(CodeGenContext *ctx, Node *node);
static void codegen_generate_def(CodeGenContext *ctx, Node *node);
static void codegen_generate_variable(CodeGenContext *ctx,
									  Node *node);
static void codegen_generate_if(CodeGenContext *ctx, Node *node);
static void codegen_generate_let(CodeGenContext *ctx, Node *node);
static void codegen_generate_call(CodeGenContext *ctx, Node *node);
static void codegen_generate_function_impl(CodeGenContext *ctx,
										   Node *node,
										   const char *self_name);
static void codegen_generate_function(CodeGenContext *ctx,
									  Node *node);

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

	EMIT_TEXT(ctx, "extern %s", c_function_label);
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

	EMIT_TEXT(ctx, "global main");
	EMIT_TEXT(ctx, "; Core runtime functions declared extern");
	for (int i = 0; i < num_elements; i++)
	{
		EMIT_TEXT(ctx, "extern %s", core_runtime_functions[i]);
	}
	EMIT_TEXT(ctx, "; Builtin functions declared extern");
	g_hash_table_foreach(ctx->builtin_func_map->_map,
						 emit_extern_for_builtin, ctx);
	EMIT_TEXT(ctx, "main:");
	EMIT_TEXT(ctx, "push rbp");
	EMIT_TEXT(ctx, "mov rbp, rsp");
}

static inline void write_epilogue(CodeGenContext *ctx)
{
	EMIT_TEXT(ctx, "mov rax, 60");
	EMIT_TEXT(ctx, "mov rdi, 0");
	EMIT_TEXT(ctx, "syscall");
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
		codegen_generate_node(ctx, node);
	}
	write_epilogue(ctx);

	asm_file_writer_consolidate(ctx->writer);
	asm_file_writer_cleanup(ctx->writer);
	codegen_context_cleanup(ctx);

	return;
}

static void codegen_generate_node(CodeGenContext *ctx, Node *node)
{
	switch (node->type)
	{
	case NODE_LITERAL:
		codegen_generate_literal(ctx, node);
		break;
	case NODE_DEF:
		codegen_generate_def(ctx, node);
		break;
	case NODE_VARIABLE:
		codegen_generate_variable(ctx, node);
		break;
	case NODE_IF:
		codegen_generate_if(ctx, node);
		break;
	case NODE_LET:
		codegen_generate_let(ctx, node);
		break;
	case NODE_CALL:
		codegen_generate_call(ctx, node);
		break;
	case NODE_FUNCTION:
		codegen_generate_function(ctx, node);
		break;
	default:
		fprintf(stderr,
				"Codegen Error: Unimplemented AST node type %d\n",
				node->type);
		exit(1);
	}
}

static void codegen_generate_literal(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_LITERAL);
	switch (node->literal.lit_type)
	{
	case LIT_INT:
		EMIT_TEXT(ctx, "mov rdi, %d", node->literal.i_val);
		EMIT_TEXT(ctx, "call lispvalue_create_int");
		break;

	case LIT_FLOAT:
		char *label = g_strdup_printf("L_float_%d", get_next_label());
		EMIT_DATA(ctx, "%s: dq %f", label, node->literal.f_val);

		EMIT_TEXT(ctx, "movsd xmm0, [%s]", label);
		EMIT_TEXT(ctx, "call lispvalue_create_float");
		g_free(label);
		break;
	case LIT_BOOL:
		EMIT_TEXT(ctx, "mov rdi, %d", node->literal.b_val ? 1 : 0);
		EMIT_TEXT(ctx, "call lispvalue_create_bool");
		break;

	default:
		printf("Codegen Error: Unimplemented literal type %d\n",
			   node->literal.lit_type);
		exit(1);
	}
}

static void codegen_generate_def(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_DEF);
	VarBinding *binding = node->def.binding;
	const char *name = binding->name;
	Node *value_expr = binding->value_expr;

	if (value_expr->type == NODE_FUNCTION)
	{
		const char *label =
			codegen_env_add_global_variable(ctx->env, name);
		EMIT_DATA(ctx, "%s: dq 0", label);
		codegen_generate_function_impl(ctx, value_expr, name);
		EMIT_TEXT(ctx, "mov [%s], rax", label);
	}
	else
	{
		codegen_generate_node(ctx, value_expr);
		const char *label =
			codegen_env_add_global_variable(ctx->env, name);
		EMIT_DATA(ctx, "%s: dq 0", label);
		EMIT_TEXT(ctx, "mov [%s], rax", label);
	}
}

static void codegen_generate_function_impl(CodeGenContext *ctx,
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

	EMIT_TEXT(ctx, "jmp %s", end_func_label);

	EMIT_TEXT(ctx, "%s: ;%s", func_label,
			  (self_name) ? self_name : "anonymous");
	EMIT_TEXT(ctx, "push rbp");
	EMIT_TEXT(ctx, "mov rbp, rsp");
	EMIT_TEXT(ctx, "push r10 ; push the closure pointer");

	codegen_env_enter_scope(ctx->env);
	codegen_env_reset_stack_offset(ctx->env, -8);

	StringArray *params = node->function.param_names;
	guint num_params = params->_array->len;
	for (guint i = 0; i < num_params; i++)
	{
		if (i < NUM_ARGUMENT_REGISTERS)
		{
			EMIT_TEXT(ctx, "push %s", ARGUMENT_REGISTERS[i]);
			codegen_env_add_stack_variable(
				ctx->env, string_array_index(params, i));
		}
	}

	StringArray *free_vars = node->function.free_var_names;
	for (guint i = 0; i < free_vars->_array->len; i++)
	{
		codegen_env_add_free_variable(
			ctx->env, string_array_index(free_vars, i), i);
	}

	NodeArray *body = node->function.body;
	for (guint i = 0; i < body->_array->len; i++)
	{
		codegen_generate_node(ctx,
							  g_ptr_array_index(body->_array, i));
	}

	codegen_env_exit_scope(ctx->env);

	EMIT_TEXT(ctx, "pop r10        ; Restore the closure pointer ");
	EMIT_TEXT(ctx, "mov rsp, rbp");
	EMIT_TEXT(ctx, "pop rbp");
	EMIT_TEXT(ctx, "ret");

	EMIT_TEXT(ctx, "%s:", end_func_label);

	guint num_free = free_vars->_array->len;
	for (int i = num_free - 1; i >= 0; i--)
	{
		const char *free_var_name = string_array_index(free_vars, i);

		if (self_name && strcmp(free_var_name, self_name) == 0)
		{
			EMIT_TEXT(ctx, "push 0 ; Self-reference for recursion");
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
				EMIT_TEXT(ctx, "push qword [%s]", loc->global_label);
				break;
			case VAR_LOCATION_STACK:
				EMIT_TEXT(ctx, "mov rdi, [rbp + %d]",
						  loc->stack_offset);
				EMIT_TEXT(ctx, "call lispcell_create");
				EMIT_TEXT(ctx, "mov rdi, rax");
				EMIT_TEXT(ctx, "call lispvalue_create_cell");
				EMIT_TEXT(ctx, "push rax");
				break;
			case VAR_LOCATION_ENV:
				EMIT_TEXT(ctx, "mov rax, [r10 + %d]",
						  32 + (loc->env_index * 8));
				EMIT_TEXT(ctx, "push rax");
				break;
			}
		}
	}

	EMIT_TEXT(ctx, "mov rdi, %s", func_label);
	EMIT_TEXT(ctx, "mov rsi, %d", num_params);
	EMIT_TEXT(ctx, "mov rdx, %d", num_free);
	const char *variadic_regs[] = {"rcx", "r8", "r9"};
	int num_variadic_regs =
		sizeof(variadic_regs) / sizeof(variadic_regs[0]);

	for (guint i = 0; i < num_free && i < num_variadic_regs; i++)
	{
		EMIT_TEXT(ctx, "mov %s, [rsp + %d]", variadic_regs[i], i * 8);
	}

	EMIT_TEXT(ctx, "xor rax, rax");

	bool need_align = (num_free % 2) == 1;

	EMIT_TEXT(ctx, "call lispvalue_create_closure");

	if (num_free > 0)
	{
		EMIT_TEXT(ctx, "add rsp, %d", (num_free) * 8);
	}

	codegen_env_set_stack_offset(ctx->env, original_stack_offset);
	g_free(func_label);
	g_free(end_func_label);
}

static void codegen_generate_variable(CodeGenContext *ctx, Node *node)
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
		EMIT_TEXT(ctx, "mov rax, [%s]", loc->global_label);
		break;

	case VAR_LOCATION_STACK:
		EMIT_TEXT(ctx, "mov rax, [rbp + %d]", loc->stack_offset);
		break;
	case VAR_LOCATION_ENV:
		EMIT_TEXT(ctx, "mov rax, [r10 + %d]",
				  32 + (loc->env_index * 8));
		EMIT_TEXT(ctx, "mov rax, [rax + 8]");
		EMIT_TEXT(ctx, "mov rax, [rax]");
		break;
		break;
	}
}

static void codegen_generate_let(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_LET);

	codegen_env_enter_scope(ctx->env);

	VarBindingArray *bindings = node->let.bindings;
	for (guint i = 0; i < bindings->_array->len; i++)
	{
		VarBinding *binding = g_ptr_array_index(bindings->_array, i);
		codegen_generate_node(ctx, binding->value_expr);
		EMIT_TEXT(ctx, "push rax");
		codegen_env_add_stack_variable(ctx->env, binding->name);
	}

	NodeArray *body = node->let.body;
	for (guint i = 0; i < body->_array->len; i++)
	{
		Node *body_expr = g_ptr_array_index(body->_array, i);
		codegen_generate_node(ctx, body_expr);
	}

	guint num_bindings = bindings->_array->len;
	if (num_bindings > 0)
	{
		EMIT_TEXT(ctx, "add rsp, %d", num_bindings * 8);
	}

	codegen_env_exit_scope(ctx->env);
}

static void codegen_generate_function(CodeGenContext *ctx, Node *node)
{
	codegen_generate_function_impl(ctx, node, NULL);
}

static inline int min(int a, int b) { return (a < b) ? a : b; }

static void codegen_generate_call(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_CALL);

	NodeArray *args = node->call.args;
	guint num_args = args->_array->len;

	for (int i = num_args - 1; i >= 0; i--)
	{
		Node *arg_node = g_ptr_array_index(args->_array, i);
		codegen_generate_node(ctx, arg_node);
		EMIT_TEXT(ctx, "push rax");
	}

	const char *builtin_c_label = NULL;
	if (node->call.fn->type == NODE_VARIABLE)
	{
		builtin_c_label = string_to_string_map_lookup(
			ctx->builtin_func_map, node->call.fn->variable.name);
	}

	if (builtin_c_label)
	{
		const char *op_name = node->call.fn->variable.name;
		bool is_variadic_op =
			(strcmp(op_name, "+") == 0 || strcmp(op_name, "*") == 0 ||
			 strcmp(op_name, "-") == 0);

		if (is_variadic_op && num_args > 2)
		{
			EMIT_TEXT(ctx, "pop rdi");
			EMIT_TEXT(ctx, "pop rsi");
			EMIT_TEXT(ctx, "call %s", builtin_c_label);

			for (guint i = 2; i < num_args; i++)
			{
				EMIT_TEXT(ctx, "mov rdi, rax");
				EMIT_TEXT(ctx, "pop rsi");
				EMIT_TEXT(ctx, "call %s", builtin_c_label);
			}
		}
		else
		{
			int num_args_in_regs =
				min(num_args, NUM_ARGUMENT_REGISTERS);
			for (guint i = 0; i < num_args_in_regs; i++)
			{
				EMIT_TEXT(ctx, "pop %s", ARGUMENT_REGISTERS[i]);
			}

			EMIT_TEXT(ctx, "call %s", builtin_c_label);
		}
	}
	else
	{

		codegen_generate_node(ctx, node->call.fn);

		EMIT_TEXT(ctx, "mov r10, rax ; r10 now holds the LispValue* "
					   "for the closure");

		int num_args_in_regs = min(num_args, NUM_ARGUMENT_REGISTERS);
		for (guint i = 0; i < num_args_in_regs; i++)
		{
			EMIT_TEXT(ctx, "pop %s", ARGUMENT_REGISTERS[i]);
		}

		EMIT_TEXT(ctx, "mov rax, [r10 + 8] ; Get code_ptr from the "
					   "closure object");
		EMIT_TEXT(ctx, "call rax");
	}

	if (num_args > NUM_ARGUMENT_REGISTERS)
	{
		EMIT_TEXT(ctx, "add rsp, %d",
				  (num_args - NUM_ARGUMENT_REGISTERS) * 8);
	}
}

static void codegen_generate_if(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_IF);
	int label_num = get_next_label();
	char *else_label = g_strdup_printf("L_else_%d", label_num);
	char *end_label = g_strdup_printf("L_end_if_%d", label_num);

	codegen_generate_node(ctx, node->if_expr.condition);

	EMIT_TEXT(ctx, "mov rdi, rax");
	EMIT_TEXT(ctx, "call lisp_is_truthy");
	EMIT_TEXT(ctx, "cmp rax, 0");

	EMIT_TEXT(ctx, "je %s", else_label);
	codegen_generate_node(ctx, node->if_expr.then_branch);
	EMIT_TEXT(ctx, "jmp %s", end_label);

	EMIT_TEXT(ctx, "%s:", else_label);

	if (node->if_expr.else_branch)
	{
		codegen_generate_node(ctx, node->if_expr.else_branch);
	}
	else
	{
		EMIT_TEXT(ctx, "xor rax, rax");
	}

	EMIT_TEXT(ctx, "%s:", end_label);
	free(else_label);
	free(end_label);
}