#include "vnstat_tests.h"
#include "ifinfo_tests.h"
#include "common.h"
#include "ifinfo.h"

void add_ifinfo_tests(Suite *s)
{
	/* Ifinfo test cases */
	TCase *tc_ifinfo = tcase_create("Ifinfo");
	suite_add_tcase(s, tc_ifinfo);
}
