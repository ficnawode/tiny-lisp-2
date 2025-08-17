#include "codegen.h"
#include <assert.h>
#include <stdarg.h>

static CodeGenContext *
codegen_context_create(const char *output_prefix);
static void codegen_context_cleanup(CodeGenContext *ctx);
static void codegen_generate_node(CodeGenContext *ctx, Node *node);
static void codegen_generate_literal(CodeGenContext *ctx, Node *node);

int get_next_label(void)
{
	static int label_number = 0;
	return ++label_number;
}

static void codegen_env_push(CodeGenContext *ctx)
{
	// Key: char*, Value: VarLocation* (needs a value_destroy_func)
	GHashTable *new_scope = g_hash_table_new_full(
		g_str_hash, g_str_equal, g_free, g_free);
	g_ptr_array_add(ctx->env_stack, new_scope);
}

static void codegen_env_pop(CodeGenContext *ctx)
{
	g_ptr_array_remove_index(ctx->env_stack, ctx->env_stack->len - 1);
}

static GHashTable *codegen_env_peek(CodeGenContext *ctx)
{
	return g_ptr_array_index(ctx->env_stack, ctx->env_stack->len - 1);
}

void codegen_compile_program(NodeArray *ast,
							 const char *output_prefix)
{
	CodeGenContext *ctx = codegen_context_create(output_prefix);
	if (!ctx)
		return;

	asm_file_writer_write_text(ctx->writer, "global main");
	asm_file_writer_write_text(ctx->writer,
							   "extern lispvalue_create_int");
	asm_file_writer_write_text(ctx->writer,
							   "extern lispvalue_create_float");
	asm_file_writer_write_text(ctx->writer, "extern lisp_print");
	asm_file_writer_write_text(ctx->writer, "main:");
	asm_file_writer_write_text(ctx->writer, "push rbp");
	asm_file_writer_write_text(ctx->writer, "mov rbp, rsp");

	for (guint i = 0; i < ast->_array->len; i++)
	{
		Node *node = g_ptr_array_index(ast->_array, i);
		codegen_generate_node(ctx, node);
	}

	// Print whatever is left in RAX
	asm_file_writer_write_text(ctx->writer, "mov rdi, rax");
	asm_file_writer_write_text(ctx->writer, "call lisp_print");

	// Exit the program
	asm_file_writer_write_text(ctx->writer,
							   "mov rax, 60"); // syscall exit
	asm_file_writer_write_text(ctx->writer,
							   "mov rdi, 0"); // exit code 0
	asm_file_writer_write_text(ctx->writer, "syscall");

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
		// Load the arg (int) into RDI
		asm_file_writer_write_text(ctx->writer, "mov rdi, %d",
								   node->literal.i_val);
		asm_file_writer_write_text(ctx->writer,
								   "call lispvalue_create_int");
		// The result (LispValue*) is now in RAX
		break;

	case LIT_FLOAT:
	{
		char *label = g_strdup_printf("L_float_%d", get_next_label());
		asm_file_writer_write_data(ctx->writer, "%s: dq %f", label,
								   node->literal.f_val);

		// Load the arg (float) from data section into XMM0
		asm_file_writer_write_text(ctx->writer, "movsd xmm0, [%s]",
								   label);
		asm_file_writer_write_text(ctx->writer,
								   "call lispvalue_create_float");
		g_free(label);
	}
	break;

	default:
		fprintf(stderr,
				"Codegen Error: Unimplemented literal type %d\n",
				node->literal.lit_type);
		exit(1);
	}
}

static CodeGenContext *
codegen_context_create(const char *output_prefix)
{
	CodeGenContext *ctx = g_new(CodeGenContext, 1);
	ctx->writer = asm_file_writer_create(output_prefix);
	if (!ctx->writer)
	{
		g_free(ctx);
		return NULL;
	}

	ctx->env_stack = g_ptr_array_new_with_free_func(
		(GDestroyNotify)g_hash_table_destroy);
	codegen_env_push(ctx);

	ctx->stack_offset = 0;
	ctx->label_counter = 0;
	return ctx;
}

static void codegen_context_cleanup(CodeGenContext *ctx)
{
	if (!ctx)
		return;
	g_ptr_array_free(ctx->env_stack, TRUE);
	g_free(ctx);
}