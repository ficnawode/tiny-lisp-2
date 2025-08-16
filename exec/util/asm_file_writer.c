#define _GNU_SOURCE // asprintf
#include "asm_file_writer.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

AsmFileWriter *asm_file_writer_create(const char *prefix)
{
	AsmFileWriter *writer = malloc(sizeof(AsmFileWriter));
	assert(writer && "Out of memory");

	writer->file_prefix = strdup(prefix);
	assert(writer->file_prefix && "Out of memory");

	asprintf(&writer->data_filename, "%s.data.tmp.s", prefix);
	asprintf(&writer->text_filename, "%s.text.tmp.s", prefix);
	assert(writer->data_filename && writer->text_filename &&
		   "Out of memory");

	writer->data_file = fopen(writer->data_filename, "w");
	writer->text_file = fopen(writer->text_filename, "w");

	if (!writer->data_file || !writer->text_file)
	{
		assert(false && "Failed to open temporary assembly files");
		if (writer->data_file)
			fclose(writer->data_file);
		if (writer->text_file)
			fclose(writer->text_file);
		free(writer->file_prefix);
		free(writer->data_filename);
		free(writer->text_filename);
		free(writer);
		return NULL;
	}

	return writer;
}

void asm_file_writer_cleanup(AsmFileWriter *writer)
{
	if (!writer)
		return;

	if (writer->data_file)
		fclose(writer->data_file);
	if (writer->text_file)
		fclose(writer->text_file);

	free(writer->file_prefix);
	free(writer->data_filename);
	free(writer->text_filename);
	free(writer);
}

static int append_file_contents(FILE *dest, const char *src_filename)
{
	FILE *src = fopen(src_filename, "r");
	if (!src)
	{
		perror("Failed to open temporary file for reading");
		return -1;
	}

	char buffer[4096];
	size_t bytes_read;
	while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0)
	{
		fwrite(buffer, 1, bytes_read, dest);
	}

	fclose(src);
	return 0;
}

int asm_file_writer_consolidate(AsmFileWriter *writer)
{
	fflush(writer->data_file);
	fflush(writer->text_file);

	char *final_filename;
	asprintf(&final_filename, "%s.asm", writer->file_prefix);
	assert(final_filename && "Out of memory");

	FILE *final_file = fopen(final_filename, "w");
	if (!final_file)
	{
		perror("Failed to open final assembly file");
		free(final_filename);
		return -1;
	}

	fprintf(final_file, "; Generated Assembly File: %s\n\n",
			final_filename);

	fprintf(final_file, "section .data\n");
	if (append_file_contents(final_file, writer->data_filename) != 0)
		return -1;

	fprintf(final_file, "\nsection .text\n");
	fprintf(final_file, "global _start\n\n");
	if (append_file_contents(final_file, writer->text_filename) != 0)
		return -1;

	fclose(final_file);
	free(final_filename);

	remove(writer->data_filename);
	remove(writer->text_filename);

	return 0;
}

void asm_file_writer_write_text(AsmFileWriter *writer,
								const char *format,
								...)
{
	va_list args;
	va_start(args, format);
	if (format[0] != '.' && strchr(format, ':') == NULL)
	{
		fprintf(writer->text_file, "\t");
	}
	vfprintf(writer->text_file, format, args);
	fprintf(writer->text_file, "\n");
	va_end(args);
}

void asm_file_writer_write_data(AsmFileWriter *writer,
								const char *format,
								...)
{
	va_list args;
	va_start(args, format);
	vfprintf(writer->data_file, format, args);
	fprintf(writer->data_file, "\n");
	va_end(args);
}