#pragma once

#include "node.h"
#include "util/asm_file_writer.h"
#include <glib.h>

typedef enum
{
	VAR_LOCATION_STACK,
	// VAR_LOCATION_GLOBAL, // top-level 'def's
	// VAR_LOCATION_ENV,    // captured closure variables
} VarLocationType;

typedef struct
{
	VarLocationType type;
	int offset; // from RBP for stack variables
} VarLocation;

typedef struct CodeGenContext
{
	AsmFileWriter *writer;

	GPtrArray *env_stack;
	int stack_offset; // from RBP for the next local variable.

	int label_counter;

} CodeGenContext;

void codegen_compile_program(NodeArray *ast,
							 const char *output_prefix);