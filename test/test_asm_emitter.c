#include "asm_emitter.h"
#include <glib.h>

static void test_reg2string(void)
{
	struct
	{
		enum Register reg;
		const char *expected;
	} test_cases[] = {
		{REG_RAX, "rax"}, {REG_RCX, "rcx"},	 {REG_RDX, "rdx"},
		{REG_RBX, "rbx"}, {REG_RSP, "rsp"},	 {REG_RBP, "rbp"},
		{REG_RSI, "rsi"}, {REG_RDI, "rdi"},	 {REG_R8, "r8"},
		{REG_R9, "r9"},	  {REG_R10, "r10"},	 {REG_R11, "r11"},
		{REG_R12, "r12"}, {REG_R13, "r13"},	 {REG_R14, "r14"},
		{REG_R15, "r15"}, {REG_XMM0, "xmm0"}};

	for (guint i = 0; i < G_N_ELEMENTS(test_cases); i++)
	{
		const char *actual = reg_to_string(test_cases[i].reg);
		g_test_message("Testing register: %s",
					   test_cases[i].expected);
		g_assert_cmpstr(actual, ==, test_cases[i].expected);
	}
}

typedef struct
{
	char *buffer;
	size_t size;
	FILE *stream;
} MockFile;

typedef struct
{
	MockFile text;
	MockFile data;
	AsmFileWriter *writer;
} TestEmitterFixture;

static void mock_file_init(MockFile *file)
{
	file->buffer = NULL;
	file->size = 0;
	file->stream = open_memstream(&file->buffer, &file->size);
	g_assert_nonnull(file->stream);
}

static void mock_file_cleanup(MockFile *file)
{
	if (file->stream)
	{
		fclose(file->stream);
	}
	free(file->buffer);
}

static void assert_text_emitted(TestEmitterFixture *fixture,
								const char *expected)
{
	fflush(fixture->text.stream);

	gchar *stripped_actual = g_strdup(fixture->text.buffer);
	gchar *stripped_expected = g_strdup(expected);

	g_strstrip(stripped_actual);
	g_strstrip(stripped_expected);

	g_assert_cmpstr(stripped_actual, ==, stripped_expected);

	g_free(stripped_actual);
	g_free(stripped_expected);

	mock_file_cleanup(&fixture->text);
	mock_file_init(&fixture->text);
	fixture->writer->text_file = fixture->text.stream;
}

static void assert_data_emitted(TestEmitterFixture *fixture,
								const char *expected)
{
	fflush(fixture->data.stream);

	gchar *stripped_actual = g_strdup(fixture->data.buffer);
	g_strchug(g_strchomp(stripped_actual));

	gchar *stripped_expected = g_strdup(expected);
	g_strchug(g_strchomp(stripped_expected));

	g_assert_cmpstr(stripped_actual, ==, stripped_expected);

	g_free(stripped_actual);
	g_free(stripped_expected);

	mock_file_cleanup(&fixture->data);
	mock_file_init(&fixture->data);
	fixture->writer->data_file = fixture->data.stream;
}

static void emitter_fixture_setup(TestEmitterFixture *fixture,
								  gconstpointer user_data)
{
	mock_file_init(&fixture->text);
	mock_file_init(&fixture->data);

	fixture->writer = asm_file_writer_create_mock(
		fixture->text.stream, fixture->data.stream);
	g_assert_nonnull(fixture->writer);
}

static void emitter_fixture_teardown(TestEmitterFixture *fixture,
									 gconstpointer user_data)
{
	mock_file_cleanup(&fixture->text);
	mock_file_cleanup(&fixture->data);
	fixture->writer->data_file = NULL;
	fixture->writer->text_file = NULL;
	asm_file_writer_cleanup(fixture->writer);
}

static void test_emitter_mov_reg_reg(TestEmitterFixture *fixture,
									 gconstpointer user_data)
{
	emit_mov_reg_reg(fixture->writer, REG_RAX, REG_RCX, NULL);
	assert_text_emitted(fixture, "mov rax, rcx\n");
	assert_data_emitted(fixture, "");

	emit_mov_reg_reg(fixture->writer, REG_R10, REG_RBP,
					 "copy base pointer");
	assert_text_emitted(fixture,
						"mov r10, rbp ; copy base pointer\n");
	assert_data_emitted(fixture, "");

	const char *var_name = "counter";
	emit_mov_reg_reg(fixture->writer, REG_RDI, REG_RSI,
					 "copy value for var '%s'", var_name);
	assert_text_emitted(
		fixture, "mov rdi, rsi ; copy value for var 'counter'\n");
	assert_data_emitted(fixture, "");
}

static void test_emit_push_ops(TestEmitterFixture *fixture,
							   gconstpointer user_data)
{
	emit_push_reg(fixture->writer, REG_RAX, NULL);
	assert_text_emitted(fixture, "push rax");
	assert_data_emitted(fixture, "");

	emit_push_imm(fixture->writer, 42, "push integer literal");
	assert_text_emitted(fixture, "push 42 ; push integer literal");
	assert_data_emitted(fixture, "");

	emit_push_global(fixture->writer, "L_my_string",
					 "push string pointer for '%s'", "hello");
	assert_text_emitted(fixture, "push qword [L_my_string] ; push "
								 "string pointer for 'hello'");
	assert_data_emitted(fixture, "");
}

static void test_emit_pop_ops(TestEmitterFixture *fixture,
							  gconstpointer user_data)
{
	emit_pop_reg(fixture->writer, REG_RDI, "restore argument 1");
	assert_text_emitted(fixture, "pop rdi ; restore argument 1");
	assert_data_emitted(fixture, "");
}

static void test_emit_mov_ops(TestEmitterFixture *fixture,
							  gconstpointer user_data)
{
	emit_mov_reg_reg(fixture->writer, REG_RSP, REG_RBP,
					 "restore stack pointer");
	assert_text_emitted(fixture,
						"mov rsp, rbp ; restore stack pointer");
	assert_data_emitted(fixture, "");

	emit_mov_reg_imm(fixture->writer, REG_RAX, 60,
					 "exit syscall number");
	assert_text_emitted(fixture, "mov rax, 60 ; exit syscall number");
	assert_data_emitted(fixture, "");

	emit_mov_reg_global(fixture->writer, REG_R10, "my_global", NULL);
	assert_text_emitted(fixture, "mov r10, [my_global]");
	assert_data_emitted(fixture, "");

	emit_mov_reg_label(fixture->writer, REG_R10, "my_label", NULL);
	assert_text_emitted(fixture, "mov r10, my_label");
	assert_data_emitted(fixture, "");

	emit_mov_global_reg(fixture->writer, "my_global_result", REG_RAX,
						NULL);
	assert_text_emitted(fixture, "mov [my_global_result], rax");
	assert_data_emitted(fixture, "");

	emit_mov_reg_membase(fixture->writer, REG_RCX, REG_RBP, -16,
						 "load local var 'y'");
	assert_text_emitted(fixture,
						"mov rcx, [rbp + -16] ; load local var 'y'");
	assert_data_emitted(fixture, "");

	emit_mov_membase_reg(fixture->writer, REG_RBP, 8, REG_RDI,
						 "store arg 1 on stack");
	assert_text_emitted(fixture,
						"mov [rbp + 8], rdi ; store arg 1 on stack");
	assert_data_emitted(fixture, "");
}

static void test_emit_float_ops(TestEmitterFixture *fixture,
								gconstpointer user_data)
{
	emit_movsd_reg_global(fixture->writer, REG_XMM0, "L_my_float",
						  NULL);
	assert_text_emitted(fixture, "movsd xmm0, [L_my_float]");
	assert_data_emitted(fixture, "");

	emit_movsd_reg_global(fixture->writer, REG_XMM0, "L_pi_const",
						  "load float const '%s'", "pi");
	assert_text_emitted(
		fixture, "movsd xmm0, [L_pi_const] ; load float const 'pi'");
	assert_data_emitted(fixture, "");

	emit_movsd_membase_reg(fixture->writer, REG_RBP, -16, REG_XMM0,
						   NULL);
	assert_text_emitted(fixture, "movsd [rbp + -16], xmm0");
	assert_data_emitted(fixture, "");

	emit_movsd_membase_reg(fixture->writer, REG_RSP, 0, REG_XMM0,
						   "store float result");
	assert_text_emitted(fixture,
						"movsd [rsp + 0], xmm0 ; store float result");
	assert_data_emitted(fixture, "");
}

static void test_emit_call_ops(TestEmitterFixture *fixture,
							   gconstpointer user_data)
{
	emit_call_reg(fixture->writer, REG_RAX,
				  "indirect call via function pointer");
	assert_text_emitted(
		fixture, "call rax ; indirect call via function pointer");
	assert_data_emitted(fixture, "");

	emit_call_label(fixture->writer, "lisp_print_debug", NULL);
	assert_text_emitted(fixture, "call lisp_print_debug");
	assert_data_emitted(fixture, "");
}

static void test_emit_stack_adjust_ops(TestEmitterFixture *fixture,
									   gconstpointer user_data)
{
	emit_add_rsp(fixture->writer, 16,
				 "cleanup 2 variables from stack");
	assert_text_emitted(
		fixture, "add rsp, 16 ; cleanup 2 variables from stack");
	assert_data_emitted(fixture, "");

	emit_sub_rsp(fixture->writer, 32, "allocate space for 4 locals");
	assert_text_emitted(fixture,
						"sub rsp, 32 ; allocate space for 4 locals");
	assert_data_emitted(fixture, "");
}

static void test_emit_control_flow_ops(TestEmitterFixture *fixture,
									   gconstpointer user_data)
{
	emit_label(fixture->writer, "L_LOOP_START_1", "start of loop %d",
			   1);
	assert_text_emitted(fixture, "L_LOOP_START_1: ; start of loop 1");
	assert_data_emitted(fixture, "");

	emit_jmp(fixture->writer, "L_LOOP_START_1", NULL);
	assert_text_emitted(fixture, "jmp L_LOOP_START_1");
	assert_data_emitted(fixture, "");

	emit_je(fixture->writer, "L_ELSE_CLAUSE_1", NULL);
	assert_text_emitted(fixture, "je L_ELSE_CLAUSE_1");
	assert_data_emitted(fixture, "");
}

static void test_emit_misc_ops(TestEmitterFixture *fixture,
							   gconstpointer user_data)
{
	emit_ret(fixture->writer, NULL);
	assert_text_emitted(fixture, "ret");
	assert_data_emitted(fixture, "");

	emit_syscall(fixture->writer, "invoke kernel");
	assert_text_emitted(fixture, "syscall ; invoke kernel");
	assert_data_emitted(fixture, "");

	emit_cmp_reg_imm(fixture->writer, REG_RAX, 0,
					 "check if rax is zero");
	assert_text_emitted(fixture, "cmp rax, 0 ; check if rax is zero");
	assert_data_emitted(fixture, "");

	emit_xor_reg_reg(fixture->writer, REG_RAX, REG_RAX,
					 "zero out rax");
	assert_text_emitted(fixture, "xor rax, rax ; zero out rax");
	assert_data_emitted(fixture, "");

	emit_global(fixture->writer, "main", "global main function");
	assert_text_emitted(fixture,
						"global main ; global main function");
	assert_data_emitted(fixture, "");

	emit_extern(fixture->writer, "lisp_print",
				"debug print function");
	assert_text_emitted(fixture,
						"extern lisp_print ; debug print function");
	assert_data_emitted(fixture, "");
}

static void test_emit_comment_only(TestEmitterFixture *fixture,
								   gconstpointer user_data)
{
	emit_comment(fixture->writer, "--- Section: Prologue ---");
	assert_text_emitted(fixture, "; --- Section: Prologue ---");
	assert_data_emitted(fixture, "");

	emit_comment(fixture->writer, "Processing item #%d", 5);
	assert_text_emitted(fixture, "; Processing item #5");
	assert_data_emitted(fixture, "");
}
static void test_emit_data_ops(TestEmitterFixture *fixture,
							   gconstpointer user_data)
{
	emit_data_label(fixture->writer, "L_my_global_int",
					"global integer storage");
	assert_data_emitted(fixture,
						"L_my_global_int: ; global integer storage");
	assert_text_emitted(fixture,
						""); // Ensure text section is untouched

	emit_data_dq_imm(fixture->writer, "L_int_const_42", 42, NULL);
	assert_data_emitted(fixture, "L_int_const_42: dq 42");
	assert_text_emitted(fixture, "");

	emit_data_dq_float(fixture->writer, "L_float_pi", 3.14159,
					   "value of pi");
	assert_data_emitted(
		fixture,
		"L_float_pi: dq 3.141590 ; value of pi"); // Note: default
												  // float formatting
	assert_text_emitted(fixture, "");

	emit_data_string(fixture->writer, "L_string_hello", "Hello",
					 "greeting string");
	// 'H' = 72, 'e' = 101, 'l' = 108, 'l' = 108, 'o' = 111
	const char *expected_str = "L_string_hello: db 72, 101, 108, "
							   "108, 111, 0 ; greeting string";
	assert_data_emitted(fixture, expected_str);
	assert_text_emitted(fixture, "");

	emit_data_string(fixture->writer, "L_empty_string", "", NULL);
	assert_data_emitted(fixture, "L_empty_string: db 0");
	assert_text_emitted(fixture, "");
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/emitter/test_reg_to_string", test_reg2string);

	g_test_add("/emitter/mov_reg_reg", TestEmitterFixture, NULL,
			   emitter_fixture_setup, test_emitter_mov_reg_reg,
			   emitter_fixture_teardown);
	g_test_add("/emitter/push_ops", TestEmitterFixture, NULL,
			   emitter_fixture_setup, test_emit_push_ops,
			   emitter_fixture_teardown);
	g_test_add("/emitter/pop_ops", TestEmitterFixture, NULL,
			   emitter_fixture_setup, test_emit_pop_ops,
			   emitter_fixture_teardown);
	g_test_add("/emitter/mov_ops", TestEmitterFixture, NULL,
			   emitter_fixture_setup, test_emit_mov_ops,
			   emitter_fixture_teardown);
	g_test_add("/emitter/call_ops", TestEmitterFixture, NULL,
			   emitter_fixture_setup, test_emit_call_ops,
			   emitter_fixture_teardown);
	g_test_add("/emitter/stack_adjust_ops", TestEmitterFixture, NULL,
			   emitter_fixture_setup, test_emit_stack_adjust_ops,
			   emitter_fixture_teardown);
	g_test_add("/emitter/control_flow_ops", TestEmitterFixture, NULL,
			   emitter_fixture_setup, test_emit_control_flow_ops,
			   emitter_fixture_teardown);
	g_test_add("/emitter/misc_ops", TestEmitterFixture, NULL,
			   emitter_fixture_setup, test_emit_misc_ops,
			   emitter_fixture_teardown);
	g_test_add("/emitter/comment_only", TestEmitterFixture, NULL,
			   emitter_fixture_setup, test_emit_comment_only,
			   emitter_fixture_teardown);
	g_test_add("/emitter/data_ops", TestEmitterFixture, NULL,
			   emitter_fixture_setup, test_emit_data_ops,
			   emitter_fixture_teardown);

	return g_test_run();
}