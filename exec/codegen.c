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
	char *core_runtime_functions[] = //
		{"lispvalue_create_int", "lispvalue_create_float",
		 "lispvalue_create_bool", "lisp_is_truthy", "lisp_print"};
	int num_elements = sizeof(core_runtime_functions) /
					   sizeof(core_runtime_functions[0]);

	EMIT_TEXT(ctx, "global main");
	EMIT_TEXT(ctx, "; Primitive operator functions declared extern");
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
	EMIT_TEXT(ctx,
			  "mov rax, 60"); // syscall exit
	EMIT_TEXT(ctx,
			  "mov rdi, 0"); // exit code 0
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
	codegen_generate_node(ctx, binding->value_expr);
	const char *label =
		codegen_env_add_global_variable(ctx->env, binding->name);
	EMIT_DATA(ctx, "%s: dq 0", label);
	EMIT_TEXT(ctx, "mov [%s], rax", label);
}

static void codegen_generate_variable(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_VARIABLE);

	const VarLocation *loc =
		codegen_env_lookup(ctx->env, node->variable.name);

	assert(loc &&
		   "Undefined variable, should have been caught by parser?");

	switch (loc->type)
	{
	case VAR_LOCATION_GLOBAL:
		EMIT_TEXT(ctx, "mov rax, [%s]", loc->global_label);
		break;

	case VAR_LOCATION_STACK:
		EMIT_TEXT(ctx, "mov rax, [rbp + %d]", loc->stack_offset);
		break;
	case VAR_LOCATION_ENV:
		printf("Codegen Error: Unimplemented variable location type "
			   "for '%s'.\n",
			   node->variable.name);
		exit(1);
		break;
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

static inline int min(int a, int b) { return (a < b) ? a : b; }

/**
 * @brief as per the C ABI: evaluate all arguments, push them onto the
 * stack. Load all possible arguments off the stack into their
 * respective registers. Leave the surplus on the stack. Call
 * function.
 */
static void codegen_generate_call(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_CALL);
	assert(node->call.fn->type == NODE_VARIABLE &&
		   "Calling non-variables not yet supported");

	const char *operator_symbol = node->call.fn->variable.name;
	const char *impl_label = string_to_string_map_lookup(
		ctx->builtin_func_map, operator_symbol);

	if (!impl_label)
	{
		printf("Codegen Error: Call to non-primitive function '%s' "
			   "is not yet implemented.\n",
			   operator_symbol);
		exit(1);
	}
	NodeArray *args = node->call.args;
	guint num_args = args->_array->len;

	for (guint i = 0; i < num_args; i++)
	{
		Node *arg_node = g_ptr_array_index(args->_array, i);
		codegen_generate_node(ctx, arg_node);
		EMIT_TEXT(ctx, "push rax");
	}

	int regs_to_load = min(num_args, NUM_ARGUMENT_REGISTERS);
	for (int i = regs_to_load - 1; i >= 0; i--)
	{
		EMIT_TEXT(ctx, "pop %s", ARGUMENT_REGISTERS[i]);
	}

	EMIT_TEXT(ctx, "call %s", impl_label);

	if (num_args > NUM_ARGUMENT_REGISTERS)
	{
		int stack_args_bytes =
			(num_args - NUM_ARGUMENT_REGISTERS) * 8;
		EMIT_TEXT(ctx, "add rsp, %d", stack_args_bytes);
	}
}