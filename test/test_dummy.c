#include <glib.h>

static void test_dummy_case(void) { g_assert_true(TRUE); }

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/dummy/case1", test_dummy_case);

	return g_test_run();
}
