#include "asm_emitter.h"
#include "glib.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static const char *REGISTER_NAMES[] = {
	"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8",
	"r9",  "r10", "r11", "r12", "r13", "r14", "r15", "xmm0"};

const char *reg_to_string(enum Register reg)
{
	assert(reg < REG_COUNT && "Invalid register enum value.");
	return REGISTER_NAMES[reg];
}

static void format_and_emit_text(AsmFileWriter *writer,
								 const char *instruction,
								 const char *comment_fmt,
								 va_list args)
{
	if (comment_fmt)
	{
		char comment_buffer[256];
		vsnprintf(comment_buffer, sizeof(comment_buffer), comment_fmt,
				  args);
		asm_file_writer_write_text(writer, "%s ; %s", instruction,
								   comment_buffer);
	}
	else
	{
		asm_file_writer_write_text(writer, "%s", instruction);
	}
}

static void format_and_emit_data(AsmFileWriter *writer,
								 const char *instruction,
								 const char *comment_fmt,
								 va_list args)
{
	if (comment_fmt)
	{
		char comment_buffer[256];
		vsnprintf(comment_buffer, sizeof(comment_buffer), comment_fmt,
				  args);
		asm_file_writer_write_data(writer, "%s ; %s", instruction,
								   comment_buffer);
	}
	else
	{
		asm_file_writer_write_data(writer, "%s", instruction);
	}
}

#define INSTRUCTION_BUFFER_SIZE 128

// I know they say 'don't be clever' but I ain't writing all that
#define IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, instr_fmt, ...)  \
	do                                                               \
	{                                                                \
		char instruction_buffer[INSTRUCTION_BUFFER_SIZE];            \
		snprintf(instruction_buffer, sizeof(instruction_buffer),     \
				 instr_fmt, ##__VA_ARGS__);                          \
                                                                     \
		va_list comment_args;                                        \
		va_start(comment_args, comment_fmt);                         \
		format_and_emit_text(writer, instruction_buffer,             \
							 comment_fmt, comment_args);             \
		va_end(comment_args);                                        \
	} while (0)

#define IMPLEMENT_DATA_EMITTER(writer, comment_fmt, instr_fmt, ...)  \
	do                                                               \
	{                                                                \
		char instruction_buffer[INSTRUCTION_BUFFER_SIZE];            \
		snprintf(instruction_buffer, sizeof(instruction_buffer),     \
				 instr_fmt, ##__VA_ARGS__);                          \
                                                                     \
		va_list comment_args;                                        \
		va_start(comment_args, comment_fmt);                         \
		/* Pass the data file stream */                              \
		format_and_emit_data(writer, instruction_buffer,             \
							 comment_fmt, comment_args);             \
		va_end(comment_args);                                        \
	} while (0)

void emit_push_reg(AsmFileWriter *writer,
				   enum Register reg,
				   const char *comment_fmt,
				   ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "push %s",
						   reg_to_string(reg));
}

void emit_push_imm(AsmFileWriter *writer,
				   int64_t imm,
				   const char *comment_fmt,
				   ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "push %lld", imm);
}

void emit_push_global(AsmFileWriter *writer,
					  const char *label,
					  const char *comment_fmt,
					  ...)
{
	// "qword" is specified to remove size ambiguity for the assembler
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "push qword [%s]",
						   label);
}

void emit_pop_reg(AsmFileWriter *writer,
				  enum Register reg,
				  const char *comment_fmt,
				  ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "pop %s",
						   reg_to_string(reg));
}

void emit_mov_reg_reg(AsmFileWriter *writer,
					  enum Register dest,
					  enum Register src,
					  const char *comment_fmt,
					  ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "mov %s, %s",
						   reg_to_string(dest), reg_to_string(src));
}

void emit_mov_reg_imm(AsmFileWriter *writer,
					  enum Register dest,
					  int64_t immediate,
					  const char *comment_fmt,
					  ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "mov %s, %lld",
						   reg_to_string(dest), immediate);
}

void emit_mov_reg_global(AsmFileWriter *writer,
						 enum Register dest,
						 const char *label,
						 const char *comment_fmt,
						 ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "mov %s, [%s]",
						   reg_to_string(dest), label);
}

void emit_mov_reg_label(AsmFileWriter *writer,
						enum Register dest,
						const char *label,
						const char *comment_fmt,
						...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "mov %s, %s",
						   reg_to_string(dest), label);
}

void emit_mov_global_reg(AsmFileWriter *writer,
						 const char *label,
						 enum Register src,
						 const char *comment_fmt,
						 ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "mov [%s], %s", label,
						   reg_to_string(src));
}

void emit_mov_reg_membase(AsmFileWriter *writer,
						  enum Register dest,
						  enum Register base,
						  int offset,
						  const char *comment_fmt,
						  ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "mov %s, [%s + %d]",
						   reg_to_string(dest), reg_to_string(base),
						   offset);
}

void emit_mov_membase_reg(AsmFileWriter *writer,
						  enum Register base,
						  int offset,
						  enum Register src,
						  const char *comment_fmt,
						  ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "mov [%s + %d], %s",
						   reg_to_string(base), offset,
						   reg_to_string(src));
}
void emit_movsd_reg_global(AsmFileWriter *writer,
						   enum Register dest,
						   const char *label,
						   const char *comment_fmt,
						   ...)
{
	// You could add an assert here to ensure dest is an XMM register
	assert(dest >= REG_XMM0 &&
		   "Destination for movsd must be an XMM register");
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "movsd %s, [%s]",
						   reg_to_string(dest), label);
}

void emit_movsd_membase_reg(AsmFileWriter *writer,
							enum Register base,
							int offset,
							enum Register src,
							const char *comment_fmt,
							...)
{
	assert(src >= REG_XMM0 &&
		   "Source for movsd must be an XMM register");
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "movsd [%s + %d], %s",
						   reg_to_string(base), offset,
						   reg_to_string(src));
}

void emit_call_reg(AsmFileWriter *writer,
				   enum Register target,
				   const char *comment_fmt,
				   ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "call %s",
						   reg_to_string(target));
}

void emit_call_label(AsmFileWriter *writer,
					 const char *label,
					 const char *comment_fmt,
					 ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "call %s", label);
}

void emit_add_rsp(AsmFileWriter *writer,
				  int value,
				  const char *comment_fmt,
				  ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "add rsp, %d", value);
}

void emit_sub_rsp(AsmFileWriter *writer,
				  int value,
				  const char *comment_fmt,
				  ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "sub rsp, %d", value);
}

void emit_global(AsmFileWriter *writer,
				 const char *label,
				 const char *comment_fmt,
				 ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "global %s", label);
}

void emit_extern(AsmFileWriter *writer,
				 const char *label,
				 const char *comment_fmt,
				 ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "extern %s", label);
}

void emit_label(AsmFileWriter *writer,
				const char *label,
				const char *comment_fmt,
				...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "%s:", label);
}

void emit_jmp(AsmFileWriter *writer,
			  const char *label,
			  const char *comment_fmt,
			  ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "jmp %s", label);
}

void emit_je(AsmFileWriter *writer,
			 const char *label,
			 const char *comment_fmt,
			 ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "je %s", label);
}

void emit_ret(AsmFileWriter *writer, const char *comment_fmt, ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "ret");
}

void emit_syscall(AsmFileWriter *writer, const char *comment_fmt, ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "syscall");
}

void emit_cmp_reg_imm(AsmFileWriter *writer,
					  enum Register reg,
					  int32_t imm,
					  const char *comment_fmt,
					  ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "cmp %s, %d",
						   reg_to_string(reg), imm);
}

void emit_xor_reg_reg(AsmFileWriter *writer,
					  enum Register dest,
					  enum Register src,
					  const char *comment_fmt,
					  ...)
{
	IMPLEMENT_TEXT_EMITTER(writer, comment_fmt, "xor %s, %s",
						   reg_to_string(dest), reg_to_string(src));
}

void emit_comment(AsmFileWriter *writer, const char *comment_fmt, ...)
{
	if (!comment_fmt)
	{
		return;
	}
	char comment_buffer[INSTRUCTION_BUFFER_SIZE * 2];
	va_list args;
	va_start(args, comment_fmt);
	vsnprintf(comment_buffer, sizeof(comment_buffer), comment_fmt,
			  args);
	va_end(args);
	asm_file_writer_write_text(writer, "; %s", comment_buffer);
}

void emit_data_label(AsmFileWriter *writer,
					 const char *label,
					 const char *comment_fmt,
					 ...)
{
	IMPLEMENT_DATA_EMITTER(writer, comment_fmt, "%s:", label);
}

void emit_data_dq_imm(AsmFileWriter *writer,
					  const char *label,
					  int64_t value,
					  const char *comment_fmt,
					  ...)
{
	IMPLEMENT_DATA_EMITTER(writer, comment_fmt, "%s: dq %lld", label,
						   value);
}

void emit_data_dq_float(AsmFileWriter *writer,
						const char *label,
						double value,
						const char *comment_fmt,
						...)
{
	IMPLEMENT_DATA_EMITTER(writer, comment_fmt, "%s: dq %f", label,
						   value);
}

void emit_data_string(AsmFileWriter *writer,
					  const char *label,
					  const char *str_value,
					  const char *comment_fmt,
					  ...)
{
	GString *instruction = g_string_new(NULL);
	g_string_append_printf(instruction, "%s: db ", label);

	for (size_t i = 0; str_value[i] != '\0'; ++i)
	{
		g_string_append_printf(instruction, "%d, ",
							   (unsigned char)str_value[i]);
	}
	g_string_append(instruction, "0");

	va_list comment_args;
	va_start(comment_args, comment_fmt);
	format_and_emit_data(writer, instruction->str, comment_fmt,
						 comment_args);
	va_end(comment_args);

	g_string_free(instruction, TRUE);
}