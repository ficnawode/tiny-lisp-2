#include "dummy.h"
#include <glib.h>

static void test_dummy_case(void)
{
	int a = 10;
	int b = 15;
	int res_expected = a + b;
	int res = add(a, b);
	g_assert(res_expected == res);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/dummy/case1", test_dummy_case);

	return g_test_run();
}
