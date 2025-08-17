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

int get_next_label(void)
{
	static int label_number = 0;
	return ++label_number;
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

	ctx->env = codegen_env_create();
	codegen_env_enter_scope(ctx->env);
	return ctx;
}

static void codegen_context_cleanup(CodeGenContext *ctx)
{
	if (!ctx)
		return;
	codegen_env_cleanup(ctx->env);
	g_free(ctx);
}

static inline void write_prologue(CodeGenContext *ctx)
{
	char *function_names[] = //
		{"lispvalue_create_int", "lispvalue_create_float",
		 "lispvalue_create_bool", "lisp_is_truthy", "lisp_print"};
	int num_elements =
		sizeof(function_names) / sizeof(function_names[0]);

	EMIT_TEXT(ctx, "global main");
	for (int i = 0; i < num_elements; i++)
	{
		EMIT_TEXT(ctx, "extern %s", function_names[i]);
	}
	EMIT_TEXT(ctx, "extern lisp_print");
	EMIT_TEXT(ctx, "main:");
	EMIT_TEXT(ctx, "push rbp");
	EMIT_TEXT(ctx, "mov rbp, rsp");
}

static inline void write_epilogue(CodeGenContext *ctx)
{
	// Print whatever is left in RAX
	EMIT_TEXT(ctx, "mov rdi, rax");
	EMIT_TEXT(ctx, "call lisp_print");

	// Exit the program
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
