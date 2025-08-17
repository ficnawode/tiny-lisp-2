#pragma once

#include <stdio.h>

typedef struct AsmFileWriter
{
	char *file_prefix;

	FILE *data_file;
	FILE *text_file;

	char *data_filename;
	char *text_filename;

} AsmFileWriter;

AsmFileWriter *asm_file_writer_create(const char *prefix);

void asm_file_writer_cleanup(AsmFileWriter *writer);

void asm_file_writer_consolidate(AsmFileWriter *writer);

void asm_file_writer_write_text(AsmFileWriter *writer,
								const char *format,
								...);

void asm_file_writer_write_data(AsmFileWriter *writer,
								const char *format,
								...);