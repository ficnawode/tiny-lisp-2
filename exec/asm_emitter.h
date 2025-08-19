#pragma once

#include "asm_file_writer.h"
#include <stdint.h>

enum Register
{
	REG_RAX,
	REG_RCX,
	REG_RDX,
	REG_RBX,
	REG_RSP,
	REG_RBP,
	REG_RSI,
	REG_RDI,
	REG_R8,
	REG_R9,
	REG_R10,
	REG_R11,
	REG_R12,
	REG_R13,
	REG_R14,
	REG_R15,

	// floating point
	REG_XMM0,

	REG_COUNT
};

const char *reg_to_string(enum Register reg);

// push rax
void emit_push_reg(AsmFileWriter *writer,
				   enum Register reg,
				   const char *comment_fmt,
				   ...);

// push 123
void emit_push_imm(AsmFileWriter *writer,
				   int64_t imm,
				   const char *comment_fmt,
				   ...);

// push [<G_global_var_0>]
void emit_push_global(AsmFileWriter *writer,
					  const char *label,
					  const char *comment_fmt,
					  ...);

// pop rax
void emit_pop_reg(AsmFileWriter *writer,
				  enum Register reg,
				  const char *comment_fmt,
				  ...);

// mov rax, r10
void emit_mov_reg_reg(AsmFileWriter *writer,
					  enum Register dest,
					  enum Register src,
					  const char *comment_fmt,
					  ...);

// mov rax, 123
void emit_mov_reg_imm(AsmFileWriter *writer,
					  enum Register dest,
					  int64_t immediate,
					  const char *comment_fmt,
					  ...);

// mov rax, [L_my_global]
void emit_mov_reg_global(AsmFileWriter *writer,
						 enum Register dest,
						 const char *label,
						 const char *comment_fmt,
						 ...);

// mov [L_my_global], rax
void emit_mov_global_reg(AsmFileWriter *writer,
						 const char *label,
						 enum Register src,
						 const char *comment_fmt,
						 ...);

// mov rax, [rbp + 16]
void emit_mov_reg_membase(AsmFileWriter *writer,
						  enum Register dest,
						  enum Register base,
						  int offset,
						  const char *comment_fmt,
						  ...);

// mov [rbp - 8], rax
void emit_mov_membase_reg(AsmFileWriter *writer,
						  enum Register base,
						  int offset,
						  enum Register src,
						  const char *comment_fmt,
						  ...);

// call rax
void emit_call_reg(AsmFileWriter *writer,
				   enum Register target,
				   const char *comment_fmt,
				   ...);

// call my_func_label_0
void emit_call_label(AsmFileWriter *writer,
					 const char *label,
					 const char *comment_fmt,
					 ...);

// add rsp, 8
void emit_add_rsp(AsmFileWriter *writer,
				  int value,
				  const char *comment_fmt,
				  ...);

// sub rsp, 8
void emit_sub_rsp(AsmFileWriter *writer,
				  int value,
				  const char *comment_fmt,
				  ...);

// extern lisp_print
void emit_global(AsmFileWriter *writer,
				 const char *label,
				 const char *comment_fmt,
				 ...);

// extern lisp_print
void emit_extern(AsmFileWriter *writer,
				 const char *label,
				 const char *comment_fmt,
				 ...);

// my_func_label_0:
void emit_label(AsmFileWriter *writer,
				const char *label,
				const char *comment_fmt,
				...);

// jmp my_func_label_0
void emit_jmp(AsmFileWriter *writer,
			  const char *label,
			  const char *comment_fmt,
			  ...);

// jmp my_func_label_0
void emit_je(AsmFileWriter *writer,
			 const char *label,
			 const char *comment_fmt,
			 ...);

// ret
void emit_ret(AsmFileWriter *writer, const char *comment_fmt, ...);

// syscall
void emit_syscall(AsmFileWriter *writer,
				  const char *comment_fmt,
				  ...);

// cmp rax, 0
void emit_cmp_reg_imm(AsmFileWriter *writer,
					  enum Register reg,
					  int32_t imm,
					  const char *comment_fmt,
					  ...);
// xor rax, rbi
void emit_xor_reg_reg(AsmFileWriter *writer,
					  enum Register dest,
					  enum Register src,
					  const char *comment_fmt,
					  ...);
// ; your comment here
void emit_comment(AsmFileWriter *writer,
				  const char *comment_fmt,
				  ...);

// my_label:
void emit_data_label(AsmFileWriter *writer,
					 const char *label,
					 const char *comment_fmt,
					 ...);

// my_int: dq 12345
void emit_data_dq_imm(AsmFileWriter *writer,
					  const char *label,
					  int64_t value,
					  const char *comment_fmt,
					  ...);

// my_float: dq 3.14159
void emit_data_dq_float(AsmFileWriter *writer,
						const char *label,
						double value,
						const char *comment_fmt,
						...);

// my_string: db "Hello", 10, 0
void emit_data_string(AsmFileWriter *writer,
					  const char *label,
					  const char *str_value,
					  const char *comment_fmt,
					  ...);