#include "common.h"
#include "vnstat_tests.h"
#include "ifinfo_tests.h"
#include "ifinfo.h"
#include "dbaccess.h"
#include "misc.h"
#include "cfg.h"
#include "ibw.h"

START_TEST(getifliststring_no_source)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(getifliststring(&ifacelist, 0), 0);
	ck_assert_str_eq(ifacelist, "");

	free(ifacelist);
}
END_TEST

START_TEST(getifliststring_proc_one_interface)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethunusual", 0, 0, 0, 0);

	ck_assert_int_eq(getifliststring(&ifacelist, 1), 1);
	ck_assert_str_eq(ifacelist, "ethunusual ");

	free(ifacelist);
}
END_TEST

START_TEST(getifliststring_proc_one_interface_with_speed)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethunusual", 0, 0, 0, 0);
	fake_sys_class_net("ethunusual", 0, 0, 0, 0, 10);

	ck_assert_int_eq(getifliststring(&ifacelist, 1), 1);
	ck_assert_str_eq(ifacelist, "ethunusual (10 Mbit) ");

	free(ifacelist);
}
END_TEST

START_TEST(getifliststring_proc_multiple_interfaces)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "random", 0, 0, 0, 0);
	fake_proc_net_dev("a", "interfaces", 0, 0, 0, 0);
	fake_proc_net_dev("a", "having", 0, 0, 0, 0);
	fake_proc_net_dev("a", "fun", 0, 0, 0, 0);
	fake_proc_net_dev("a", "i", 0, 0, 0, 0);

	ck_assert_int_eq(getifliststring(&ifacelist, 0), 1);
	ck_assert_str_eq(ifacelist, "random interfaces having fun i ");

	free(ifacelist);
}
END_TEST

START_TEST(getifliststring_proc_multiple_interfaces_validating)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "random", 0, 0, 0, 0);
	fake_proc_net_dev("a", "interfaces", 0, 0, 0, 0);
	fake_proc_net_dev("a", "having", 0, 0, 0, 0);
	fake_proc_net_dev("a", "sit0", 0, 0, 0, 0);
	fake_proc_net_dev("a", "fun", 0, 0, 0, 0);
	fake_proc_net_dev("a", "lo0", 0, 0, 0, 0);
	fake_proc_net_dev("a", "eth0:0", 0, 0, 0, 0);
	fake_proc_net_dev("a", "i", 0, 0, 0, 0);

	ck_assert_int_eq(getifliststring(&ifacelist, 0), 1);
	ck_assert_str_eq(ifacelist, "random interfaces having fun eth0 i ");

	free(ifacelist);
}
END_TEST

START_TEST(getifliststring_proc_long_interface_names)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "random", 0, 0, 0, 0);
	fake_proc_net_dev("a", "interfaces", 0, 0, 0, 0);
	fake_proc_net_dev("a", "having", 0, 0, 0, 0);
	fake_proc_net_dev("a", "toomuchfun", 0, 0, 0, 0);
	fake_proc_net_dev("a", "longinterfaceislong", 0, 0, 0, 0);
	fake_proc_net_dev("a", "longestinterfaceislongerthanshouldbeexpectedanywhereinanormallyfunctioningenvironment", 0, 0, 0, 0);
	fake_proc_net_dev("a", "a", 0, 0, 0, 0);

	ck_assert_int_eq(getifliststring(&ifacelist, 0), 1);
	ck_assert_str_eq(ifacelist, "random interfaces having toomuchfun longinterfaceislong a ");

	free(ifacelist);
}
END_TEST

START_TEST(getifliststring_sysclassnet_one_interface)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_sys_class_net("ethunusual", 0, 0, 0, 0, 0);

	ck_assert_int_eq(getifliststring(&ifacelist, 1), 1);
	ck_assert_str_eq(ifacelist, "ethunusual ");

	free(ifacelist);
}
END_TEST

START_TEST(getifliststring_sysclassnet_one_interface_with_speed)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_sys_class_net("ethunusual", 0, 0, 0, 0, 10);

	ck_assert_int_eq(getifliststring(&ifacelist, 1), 1);
	ck_assert_str_eq(ifacelist, "ethunusual (10 Mbit) ");

	free(ifacelist);
}
END_TEST

START_TEST(getifliststring_sysclassnet_multiple_interfaces)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_sys_class_net("random", 0, 0, 0, 0, 0);
	fake_sys_class_net("interfaces", 0, 0, 0, 0, 0);
	fake_sys_class_net("having", 0, 0, 0, 0, 0);
	fake_sys_class_net("fun", 0, 0, 0, 0, 0);
	fake_sys_class_net("i", 0, 0, 0, 0, 0);

	ck_assert_int_eq(getifliststring(&ifacelist, 0), 1);
	ck_assert_int_eq(strlen(ifacelist), 31);

	free(ifacelist);
}
END_TEST

START_TEST(getifliststring_sysclassnet_multiple_interfaces_validating)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_sys_class_net("lo", 0, 0, 0, 0, 0);
	fake_sys_class_net("random", 0, 0, 0, 0, 0);
	fake_sys_class_net("lo0", 0, 0, 0, 0, 0);
	fake_sys_class_net("interfaces", 0, 0, 0, 0, 0);
	fake_sys_class_net("having", 0, 0, 0, 0, 0);
	fake_sys_class_net("fun", 0, 0, 0, 0, 0);
	fake_sys_class_net("sit0", 0, 0, 0, 0, 0);
	fake_sys_class_net("eth0:0", 0, 0, 0, 0, 0);
	fake_sys_class_net("i", 0, 0, 0, 0, 0);

	ck_assert_int_eq(getifliststring(&ifacelist, 0), 1);
	ck_assert_int_eq(strlen(ifacelist), 31);

	free(ifacelist);
}
END_TEST

START_TEST(getifliststring_sysclassnet_long_interface_names)
{
	char *ifacelist;

	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_sys_class_net("random", 0, 0, 0, 0, 0);
	fake_sys_class_net("interfaces", 0, 0, 0, 0, 0);
	fake_sys_class_net("having", 0, 0, 0, 0, 0);
	fake_sys_class_net("toomuchfun", 0, 0, 0, 0, 0);
	fake_sys_class_net("longinterfaceislong", 0, 0, 0, 0, 0);
	fake_sys_class_net("longestinterfaceislongerthanshouldbeexpectedanywhereinanormallyfunctioningenvironment", 0, 0, 0, 0, 0);
	fake_sys_class_net("a", 0, 0, 0, 0, 0);

	ck_assert_int_eq(getifliststring(&ifacelist, 0), 1);
	ck_assert_int_eq(strlen(ifacelist), 58);

	free(ifacelist);
}
END_TEST

START_TEST(readproc_no_file)
{
	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(readproc("ethunusual"), 0);
}
END_TEST

START_TEST(readproc_not_found)
{
	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethwrong", 10, 20, 30, 40);

	ck_assert_int_eq(readproc("ethunusual"), 0);
}
END_TEST

START_TEST(readproc_success)
{
	linuxonly;

	noexit = 0;
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethwrong", 10, 20, 30, 40);
	fake_proc_net_dev("a", "ethunusual", 1, 2, 3, 4);

	ck_assert_int_eq(readproc("ethunusual"), 1);
	ck_assert_str_eq(ifinfo.name, "ethunusual");
	ck_assert_int_eq(ifinfo.filled, 1);
	ck_assert_int_eq(ifinfo.rx, 1);
	ck_assert_int_eq(ifinfo.tx, 2);
	ck_assert_int_eq(ifinfo.rxp, 3);
	ck_assert_int_eq(ifinfo.txp, 4);
}
END_TEST

START_TEST(readsysclassnet_not_found)
{
	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_sys_class_net("ethwrong", 10, 20, 30, 40, 50);

	ck_assert_int_eq(readsysclassnet("ethunusual"), 0);
}
END_TEST

START_TEST(readsysclassnet_success)
{
	linuxonly;

	noexit = 0;
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_sys_class_net("ethwrong", 10, 20, 30, 40, 50);
	fake_sys_class_net("ethunusual", 1, 2, 3, 4, 5);

	ck_assert_int_eq(readsysclassnet("ethunusual"), 1);
	ck_assert_str_eq(ifinfo.name, "ethunusual");
	ck_assert_int_eq(ifinfo.filled, 1);
	ck_assert_int_eq(ifinfo.rx, 1);
	ck_assert_int_eq(ifinfo.tx, 2);
	ck_assert_int_eq(ifinfo.rxp, 3);
	ck_assert_int_eq(ifinfo.txp, 4);
}
END_TEST

START_TEST(getifinfo_not_found)
{
	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethwrong", 10, 20, 30, 40);

	suppress_output();

	ck_assert_int_eq(getifinfo("ethunusual"), 0);
}
END_TEST

START_TEST(getifinfo_success)
{
	linuxonly;

	noexit = 0;
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethwrong", 10, 20, 30, 40);
	fake_proc_net_dev("a", "ethunusual", 1, 2, 3, 4);

	suppress_output();

	ck_assert_int_eq(getifinfo("ethunusual"), 1);
	ck_assert_str_eq(ifinfo.name, "ethunusual");
	ck_assert_int_eq(ifinfo.filled, 1);
	ck_assert_int_eq(ifinfo.rx, 1);
	ck_assert_int_eq(ifinfo.tx, 2);
	ck_assert_int_eq(ifinfo.rxp, 3);
	ck_assert_int_eq(ifinfo.txp, 4);
}
END_TEST

START_TEST(isifavailable_knows_interface_availability)
{
	linuxonly;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "eth0", 10, 20, 30, 40);
	fake_proc_net_dev("a", "eth1", 1, 2, 3, 4);

	suppress_output();

	ck_assert_int_eq(isifavailable("eth0"), 1);
	ck_assert_int_eq(isifavailable("eth1"), 1);
	ck_assert_int_eq(isifavailable("eth2"), 0);
}
END_TEST

void add_ifinfo_tests(Suite *s)
{
	TCase *tc_ifinfo = tcase_create("Ifinfo");
	tcase_add_checked_fixture(tc_ifinfo, setup, teardown);
	tcase_add_unchecked_fixture(tc_ifinfo, setup, teardown);
	tcase_add_test(tc_ifinfo, getifliststring_no_source);
	tcase_add_test(tc_ifinfo, getifliststring_proc_one_interface);
	tcase_add_test(tc_ifinfo, getifliststring_proc_one_interface_with_speed);
	tcase_add_test(tc_ifinfo, getifliststring_proc_multiple_interfaces);
	tcase_add_test(tc_ifinfo, getifliststring_proc_multiple_interfaces_validating);
	tcase_add_test(tc_ifinfo, getifliststring_proc_long_interface_names);
	tcase_add_test(tc_ifinfo, getifliststring_sysclassnet_one_interface);
	tcase_add_test(tc_ifinfo, getifliststring_sysclassnet_one_interface_with_speed);
	tcase_add_test(tc_ifinfo, getifliststring_sysclassnet_multiple_interfaces);
	tcase_add_test(tc_ifinfo, getifliststring_sysclassnet_multiple_interfaces_validating);
	tcase_add_test(tc_ifinfo, getifliststring_sysclassnet_long_interface_names);
	tcase_add_test(tc_ifinfo, readproc_no_file);
	tcase_add_test(tc_ifinfo, readproc_not_found);
	tcase_add_test(tc_ifinfo, readproc_success);
	tcase_add_test(tc_ifinfo, readsysclassnet_not_found);
	tcase_add_test(tc_ifinfo, readsysclassnet_success);
	tcase_add_test(tc_ifinfo, getifinfo_not_found);
	tcase_add_test(tc_ifinfo, getifinfo_success);
	tcase_add_test(tc_ifinfo, isifavailable_knows_interface_availability);
	suite_add_tcase(s, tc_ifinfo);
}
