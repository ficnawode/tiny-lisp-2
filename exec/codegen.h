#pragma once

#include "codegen_env.h"
#include "node.h"
#include "util/asm_file_writer.h"
#include <glib.h>

typedef struct CodeGenContext
{
	AsmFileWriter *writer;
	CodeGenEnv *env;
} CodeGenContext;

void codegen_compile_program(NodeArray *ast,
							 const char *output_prefix);