#pragma once

#include "asm_file_writer.h"
#include "codegen_env.h"
#include "node.h"
#include <glib.h>

typedef struct CodeGenContext
{
	AsmFileWriter *writer;
	CodeGenEnv *env;
	StringToStringMap *builtin_func_map;
} CodeGenContext;

void codegen_compile_program(NodeArray *ast,
							 const char *output_prefix);