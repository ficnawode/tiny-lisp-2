#include "lispvalue.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void runtime_error(const char *message)
{
	printf("Runtime Error: %s\n", message);
	exit(1);
}

static void runtime_assert(bool condition, const char *message)
{
	if (!condition)
	{
		runtime_error(message);
	}
}

LispValue *lispvalue_create_int(long value)
{
	LispValue *lv = malloc(sizeof(LispValue));
	runtime_assert(lv, "Out of memory");

	lv->type = LISP_INT;
	lv->as.i_val = value;
	return lv;
}

LispValue *lispvalue_create_float(double value)
{
	LispValue *lv = malloc(sizeof(LispValue));
	runtime_assert(lv, "Out of memory");

	lv->type = LISP_FLOAT;
	lv->as.f_val = value;
	return lv;
}

LispValue *lispvalue_create_bool(long value)
{
	LispValue *lv = malloc(sizeof(LispValue));
	runtime_assert(lv, "Out of memory");

	lv->type = LISP_BOOL;
	lv->as.b_val = (value != 0);
	return lv;
}

long lisp_is_truthy(LispValue *val)
{
	if (!val || val->type == LISP_NIL)
	{
		return 0;
	}

	if (val->type == LISP_BOOL && val->as.b_val == false)
	{
		return 0;
	}

	return 1;
}

void lisp_print(LispValue *val)
{
	if (!val)
	{
		printf("NULL");
		printf("\n");
		fflush(stdout);
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

	printf("\n");
	fflush(stdout);
}

static double get_numeric_value_as_double(LispValue *lv)
{
	runtime_assert(lv, "Unexpected NULL value in numeric operation.");

	if (lv->type == LISP_INT)
	{
		return (double)lv->as.i_val;
	}
	if (lv->type == LISP_FLOAT)
	{
		return lv->as.f_val;
	}

	runtime_error(
		"Invalid type in arithmetic expression. Expected number.");
	return 0.0;
}

typedef long (*integer_op_func)(long a, long b);
typedef double (*float_op_func)(double a, double b);

static LispValue *lisp_execute_numeric_op(LispValue *a,
										  LispValue *b,
										  integer_op_func int_op,
										  float_op_func float_op)
{
	if (!a || !b)
	{
		runtime_error("NULL argument passed to numeric operation.");
	}

	if (a->type == LISP_FLOAT || b->type == LISP_FLOAT)
	{
		double val_a = get_numeric_value_as_double(a);
		double val_b = get_numeric_value_as_double(b);
		return lispvalue_create_float(float_op(val_a, val_b));
	}
	else if (a->type == LISP_INT && b->type == LISP_INT)
	{
		long val_a = a->as.i_val;
		long val_b = b->as.i_val;
		return lispvalue_create_int(int_op(val_a, val_b));
	}
	else
	{
		runtime_error(
			"Invalid type in numeric operation. Expected number.");
		return NULL;
	}
}

static long op_add_int(long a, long b) { return a + b; }
static double op_add_float(double a, double b) { return a + b; }
LispValue *lisp_add(LispValue *a, LispValue *b)
{
	return lisp_execute_numeric_op(a, b, op_add_int, op_add_float);
}

static long op_sub_int(long a, long b) { return a - b; }
static double op_sub_float(double a, double b) { return a - b; }
LispValue *lisp_subtract(LispValue *a, LispValue *b)
{
	return lisp_execute_numeric_op(a, b, op_sub_int, op_sub_float);
}
