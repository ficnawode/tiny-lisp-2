#pragma once

#include <glib.h>
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
	LISP_CELL
} LispValueType;

typedef struct LispCell
{
	LispValue *value;
} LispCell;

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

		void *closure_obj;
		LispCell *cell;
	} as;
};

typedef struct
{
	LispValueType type; // must be LISP_CLOSURE
	void (*code_ptr)(void);
	long arity;
	long num_free_vars;
	LispValue *free_vars[];
} LispClosureObject;