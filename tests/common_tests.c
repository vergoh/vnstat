#include "vnstat_tests.h"
#include "common_tests.h"
#include "common.h"
#include "dbaccess.h"
#include "cfg.h"

START_TEST(printe_options)
{
	noexit = 2;
	cfg.uselogging = 0;
	ck_assert_int_eq(printe(PT_Info), 1);

	cfg.uselogging = 1;
	ck_assert_int_eq(printe(PT_Multiline), 1);

	noexit = 0;
	strcpy(errorstring, "dummy string");
	suppress_output();
	ck_assert_int_eq(printe(PT_Info), 1);
	ck_assert_int_eq(printe(PT_Error), 1);
	ck_assert_int_eq(printe(PT_Config), 1);
	ck_assert_int_eq(printe(PT_Multiline), 1);
	ck_assert_int_eq(printe(PT_ShortMultiline), 1);
	ck_assert_int_eq(printe(5), 1);
}
END_TEST

START_TEST(logprint_options)
{
	cfg.uselogging = 0;
	ck_assert_int_eq(logprint(PT_Info), 0);

	cfg.uselogging = 1;
	strcpy(cfg.logfile, "/dev/null");
	strcpy(errorstring, "dummy string");
	ck_assert_int_eq(logprint(PT_Info), 1);
	ck_assert_int_eq(logprint(PT_Error), 1);
	ck_assert_int_eq(logprint(PT_Config), 1);
	ck_assert_int_eq(logprint(PT_Multiline), 0);
	ck_assert_int_eq(logprint(PT_ShortMultiline), 1);
	ck_assert_int_eq(logprint(5), 1);
}
END_TEST

START_TEST(dmonth_return_within_range)
{
	int m;
	m = dmonth(_i);
	ck_assert_int_ge(m, 28);
	ck_assert_int_le(m, 31);
}
END_TEST

START_TEST(mosecs_return_values)
{
	initdb();
	defaultcfg();
	ck_assert_int_eq(cfg.monthrotate, 1);
	ck_assert_int_eq((int)mosecs(), 0);
	sleep(1);
	data.lastupdated = time(NULL);
	ck_assert_int_ne((int)mosecs(), 0);
}
END_TEST

void add_common_tests(Suite *s)
{
	/* Common test cases */
	TCase *tc_common = tcase_create("Common");
	tcase_add_test(tc_common, printe_options);
	tcase_add_test(tc_common, logprint_options);
	tcase_add_loop_test(tc_common, dmonth_return_within_range, 0, 12);
	tcase_add_test(tc_common, mosecs_return_values);
	suite_add_tcase(s, tc_common);
}
