#include "vnstat_tests.h"
#include "daemon_tests.h"
#include "common.h"
#include "dbcache.h"
#include "daemon.h"

START_TEST(getuser_root_string)
{
	ck_assert_int_eq((int)getuser("root"), 0);
}
END_TEST

START_TEST(getuser_root_numeric)
{
	ck_assert_int_eq((int)getuser("0"), 0);
}
END_TEST

START_TEST(getuser_no_such_user_string)
{
	suppress_output();
	getuser("reallynosuchuser");
}
END_TEST

START_TEST(getuser_no_such_user_numeric)
{
	suppress_output();
	getuser("99999999");
}
END_TEST

START_TEST(getgroup_root_string)
{
	ck_assert_int_eq((int)getgroup("root"), 0);
}
END_TEST

START_TEST(getgroup_root_numeric)
{
	ck_assert_int_eq((int)getgroup("0"), 0);
}
END_TEST

START_TEST(getgroup_no_such_user_string)
{
	suppress_output();
	getgroup("reallynosuchgroup");
}
END_TEST

START_TEST(getgroup_no_such_user_numeric)
{
	suppress_output();
	getgroup("99999999");
}
END_TEST

START_TEST(debugtimestamp_does_not_exit)
{
	suppress_output();
	debugtimestamp();
}
END_TEST

void add_daemon_tests(Suite *s)
{
	/* Config test cases */
	TCase *tc_daemon = tcase_create("Daemon");
	tcase_add_test(tc_daemon, getuser_root_string);
	tcase_add_test(tc_daemon, getuser_root_numeric);
	tcase_add_exit_test(tc_daemon, getuser_no_such_user_string, 1);
	tcase_add_exit_test(tc_daemon, getuser_no_such_user_numeric, 1);
	tcase_add_test(tc_daemon, getgroup_root_string);
	tcase_add_test(tc_daemon, getgroup_root_numeric);
	tcase_add_exit_test(tc_daemon, getgroup_no_such_user_string, 1);
	tcase_add_exit_test(tc_daemon, getgroup_no_such_user_numeric, 1);
	tcase_add_test(tc_daemon, debugtimestamp_does_not_exit);
	suite_add_tcase(s, tc_daemon);
}
