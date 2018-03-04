#include "common.h"
#include "vnstat_tests.h"
#include "id_tests.h"
#include "id.h"

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
#if defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)
	ck_assert_int_eq((int)getgroup("root"), 0);
#else
	ck_assert_int_eq((int)getgroup("wheel"), 0);
#endif
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

START_TEST(setuser_with_empty_user)
{
	suppress_output();
	setuser("");
}
END_TEST

START_TEST(setgroup_with_empty_group)
{
	suppress_output();
	setgroup("");
}
END_TEST

void add_id_tests(Suite *s)
{
	TCase *tc_id = tcase_create("ID");
	tcase_add_test(tc_id, getuser_root_string);
	tcase_add_test(tc_id, getuser_root_numeric);
	tcase_add_exit_test(tc_id, getuser_no_such_user_string, 1);
	tcase_add_exit_test(tc_id, getuser_no_such_user_numeric, 1);
	tcase_add_test(tc_id, getgroup_root_string);
	tcase_add_test(tc_id, getgroup_root_numeric);
	tcase_add_exit_test(tc_id, getgroup_no_such_user_string, 1);
	tcase_add_exit_test(tc_id, getgroup_no_such_user_numeric, 1);
	tcase_add_test(tc_id, setuser_with_empty_user);
	tcase_add_test(tc_id, setgroup_with_empty_group);
	suite_add_tcase(s, tc_id);
}
