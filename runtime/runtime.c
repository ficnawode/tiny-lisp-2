#include "lispvalue.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Creates a new LispValue of type integer on the heap.
 *
 * According to the x86-64 System V ABI (used by Linux/macOS):
 * - The first integer argument (`value`) will be in the RDI register.
 * - The return value (the pointer) will be placed in the RAX
 * register.
 */
LispValue *lispvalue_create_int(long value)
{
	LispValue *lv = malloc(sizeof(LispValue));
	assert(lv && "Runtime error: Out of memory");

	lv->type = LISP_INT;
	lv->as.i_val = value;
	return lv;
}

/**
 * @brief Creates a new LispValue of type float on the heap.
 *
 * According to the x86-64 System V ABI:
 * - The first floating-point argument (`value`) will be in the XMM0
 * register.
 * - The return value (the pointer) will be placed in the RAX
 * register.
 */
LispValue *lispvalue_create_float(double value)
{
	LispValue *lv = malloc(sizeof(LispValue));
	assert(lv && "Runtime error: Out of memory");

	lv->type = LISP_FLOAT;
	lv->as.f_val = value;
	return lv;
}

/**
 * @brief A simple print function for debugging.
 *
 * Can be called from assembly to inspect values on the stack or in
 registers.
 * ABI: Expects the LispValue pointer argument in the RDI register.
 */
void lisp_print(LispValue *val)
{
	if (!val)
	{
		printf("NULL");
		return;
	}

	switch (val->type)
	{
	case LISP_NIL:
		printf("()");
		break;
	case LISP_BOOL:
		printf(val->as.b_val ? "#t" : "#f");
		break;
	case LISP_INT:
		printf("%ld", val->as.i_val);
		break;
	case LISP_FLOAT:
		printf("%f", val->as.f_val);
		break;
	case LISP_STRING:
		printf("\"%s\"", val->as.s_val);
		break;
	case LISP_SYMBOL:
		printf("%s", val->as.s_val);
		break;
	case LISP_CONS:
		printf("(");
		lisp_print(val->as.cons.car);
		printf(" ...)");
		break;
	case LISP_CLOSURE:
		printf("#<closure:%p>", val->as.closure.code_ptr);
		break;
	default:
		printf("#<unknown_type:%d>", val->type);
		break;
	}
	fflush(stdout);
}