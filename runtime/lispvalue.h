#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct LispValue LispValue;

typedef enum
{
	LISP_NIL, // The empty list '()', also used for false-like values
	LISP_BOOL,
	LISP_INT,
	LISP_FLOAT,
	LISP_STRING,  // A heap-allocated, null-terminated string
	LISP_SYMBOL,  // A heap-allocated, null-terminated string
	LISP_CONS,	  // A (car . cdr) pair
	LISP_CLOSURE, // A function with its captured environment
} LispValueType;

struct LispValue
{
	LispValueType type;
	union
	{
		bool b_val;
		long i_val;
		double f_val;
		char *s_val; // strings AND symbols

		struct
		{
			LispValue *car; // First element
			LispValue *cdr; // Rest of the list
		} cons;

		struct
		{
			void (*code_ptr)(void); // Pointer to a native function
			LispValue
				*env; // ParserEnvironment (likely a list of values)
		} closure;

	} as;
};