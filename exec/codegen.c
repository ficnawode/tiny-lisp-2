#include "codegen.h"
#include <assert.h>
#include <stdarg.h>

static CodeGenContext *
codegen_context_create(const char *output_prefix);
static void codegen_context_cleanup(CodeGenContext *ctx);
static void codegen_generate_node(CodeGenContext *ctx, Node *node);
static void codegen_generate_literal(CodeGenContext *ctx, Node *node);
static void codegen_generate_def(CodeGenContext *ctx, Node *node);
static void codegen_generate_variable(CodeGenContext *ctx,
									  Node *node);

int get_next_label(void)
{
	static int label_number = 0;
	return ++label_number;
}

static inline void write_prologue(CodeGenContext *ctx)
{
	char *function_names[] = //
		{"lispvalue_create_int", "lispvalue_create_float",
		 "lisp_print"};
	int num_elements =
		sizeof(function_names) / sizeof(function_names[0]);

	asm_file_writer_write_text(ctx->writer, "global main");
	for (int i = 0; i < num_elements; i++)
	{
		asm_file_writer_write_text(ctx->writer, "extern %s",
								   function_names[i]);
	}
	asm_file_writer_write_text(ctx->writer, "extern lisp_print");
	asm_file_writer_write_text(ctx->writer, "main:");
	asm_file_writer_write_text(ctx->writer, "push rbp");
	asm_file_writer_write_text(ctx->writer, "mov rbp, rsp");
}

static inline void write_epilogue(CodeGenContext *ctx)
{
	// Print whatever is left in RAX
	asm_file_writer_write_text(ctx->writer, "mov rdi, rax");
	asm_file_writer_write_text(ctx->writer, "call lisp_print");

	// Exit the program
	asm_file_writer_write_text(ctx->writer,
							   "mov rax, 60"); // syscall exit
	asm_file_writer_write_text(ctx->writer,
							   "mov rdi, 0"); // exit code 0
	asm_file_writer_write_text(ctx->writer, "syscall");
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

static void codegen_generate_def(CodeGenContext *ctx, Node *node)
{
	assert(node->type == NODE_DEF);
	VarBinding *binding = node->def.binding;
	codegen_generate_node(ctx, binding->value_expr);
	const char *label =
		codegen_env_add_global_variable(ctx->env, binding->name);
	asm_file_writer_write_data(ctx->writer, "%s: dq 0", label);
	asm_file_writer_write_text(ctx->writer, "mov [%s], rax", label);
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
		asm_file_writer_write_text(ctx->writer, "mov rax, [%s]",
								   loc->global_label);
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

static CodeGenContext *
codegen_context_create(const char *output_prefix)
{
	CodeGenContext *ctx = malloc(sizeof(CodeGenContext));
	ctx->writer = asm_file_writer_create(output_prefix);
	if (!ctx->writer)
	{
		g_free(ctx);
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