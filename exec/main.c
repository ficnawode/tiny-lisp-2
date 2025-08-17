#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "parser.h"

static char *read_file_to_string(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	assert(file && "File does not exist?");

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	assert(size >= 0 && "Negative file size?");

	char *buffer = (char *)malloc(size + 1);
	assert(buffer && "Out of memory");

	size_t bytes_read = fread(buffer, 1, size, file);
	assert(bytes_read >= size && !ferror(file) &&
		   "File reading failed");

	buffer[bytes_read] = '\0';

	fclose(file);
	return buffer;
}

static char *get_output_prefix(const char *input_filename)
{
	const char *last_slash = strrchr(input_filename, '/');
	const char *last_bslash = strrchr(input_filename, '\\');
	const char *basename = input_filename;

	if (last_slash && last_bslash)
	{
		basename =
			(last_slash > last_bslash ? last_slash : last_bslash) + 1;
	}
	else if (last_slash)
	{
		basename = last_slash + 1;
	}
	else if (last_bslash)
	{
		basename = last_bslash + 1;
	}

	const char *last_dot = strrchr(basename, '.');

	if (last_dot && last_dot != basename)
	{
		size_t len = last_dot - basename;
		char *prefix = (char *)malloc(len + 1);
		strncpy(prefix, basename, len);
		prefix[len] = '\0';
		return prefix;
	}
	else
	{
		return strdup(basename);
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <input_file.lisp>\n", argv[0]);
		return 1;
	}

	const char *input_filename = argv[1];

	printf("--- Reading source file: %s ---\n", input_filename);
	char *source_code = read_file_to_string(input_filename);

	if (!source_code)
	{
		printf("Error: Could not read file '%s': %s\n",
			   input_filename);
		return 1;
	}
	printf("Source loaded successfully (%zu bytes).\n\n",
		   strlen(source_code));

	printf("--- Parsing source code ---\n");
	ParserContext *parser_ctx = parser_create(source_code);
	NodeArray *ast = parser_parse(parser_ctx);

	if (parser_ctx->errors->len > 0)
	{
		fprintf(stderr, "Parsing failed with %d error(s):\n",
				parser_ctx->errors->len);
		parser_print_errors(parser_ctx);
		parser_cleanup(parser_ctx);
		node_array_free(ast);
		free(source_code);
		return 1;
	}
	printf(
		"Parsing successful. AST has %d top-level expression(s).\n\n",
		ast->_array->len);

	parser_cleanup(parser_ctx);
	free(source_code);

	char *output_prefix = get_output_prefix(input_filename);
	printf("--- Generating assembly with prefix: %s ---\n",
		   output_prefix);

	codegen_compile_program(ast, output_prefix);

	node_array_free(ast);

	printf("\nCompilation successful!\n");
	printf("Generated: %s.asm\n\n", output_prefix);
	printf("To assemble and link, run:\n");
	printf("  nasm -f elf64 -g %s.asm -o %s.o\n", output_prefix,
		   output_prefix);
	printf("  gcc %s.o runtime.o -o %s\n\n", output_prefix,
		   output_prefix);

	free(output_prefix);

	return 0;
}
