#include "vnstat_tests.h"
#include "ifinfo_tests.h"
#include "common.h"
#include "ifinfo.h"


START_TEST(countercalc_no_change)
{
	ck_assert_int_eq(countercalc(0, 0), 0);
	ck_assert_int_eq(countercalc(1, 1), 0);
}
END_TEST

START_TEST(countercalc_small_change)
{
	ck_assert_int_eq(countercalc(0, 1), 1);
	ck_assert_int_eq(countercalc(1, 2), 1);
	ck_assert_int_eq(countercalc(1, 3), 2);
}
END_TEST

START_TEST(countercalc_32bit)
{
	ck_assert(countercalc(1, 0)==(FP32-1));
}
END_TEST

START_TEST(countercalc_64bit)
{
	ck_assert(countercalc(FP32+1, 0)==(FP64-FP32-1));
}
END_TEST

void add_ifinfo_tests(Suite *s)
{
	/* Ifinfo test cases */
	TCase *tc_ifinfo = tcase_create("Ifinfo");
	tcase_add_test(tc_ifinfo, countercalc_no_change);
	tcase_add_test(tc_ifinfo, countercalc_small_change);
	tcase_add_test(tc_ifinfo, countercalc_32bit);
	tcase_add_test(tc_ifinfo, countercalc_64bit);
	suite_add_tcase(s, tc_ifinfo);
}
