#include "vnstat_tests.h"
#include "ifinfo_tests.h"
#include "common.h"
#include "ifinfo.h"
#include "dbaccess.h"
#include "misc.h"
#include "cfg.h"

START_TEST(parseifinfo_zero_change)
{
	initdb();
	data.btime = getbtime();
	data.lastupdated -= 100;
	strcpy(data.interface, "eth0");
	ck_assert_int_eq(ibwadd("eth0", 10), 1);

	strcpy(ifinfo.name, "eth0");
	ifinfo.filled = 1;
	ifinfo.rx = 0;
	ifinfo.tx = 0;
	ifinfo.rxp = ifinfo.txp = 0;

	parseifinfo(0);

	ck_assert_int_eq(ifinfo.rx, 0);
	ck_assert_int_eq(ifinfo.tx, 0);
	ck_assert_int_eq(ifinfo.rxp, 0);
	ck_assert_int_eq(ifinfo.txp, 0);

	ck_assert_int_eq(data.day[0].rx, 0);
	ck_assert_int_eq(data.day[0].tx, 0);
	ck_assert_int_eq(data.day[0].rxk, 0);
	ck_assert_int_eq(data.day[0].txk, 0);

	ck_assert_int_eq(data.month[0].rx, 0);
	ck_assert_int_eq(data.month[0].tx, 0);
	ck_assert_int_eq(data.month[0].rxk, 0);
	ck_assert_int_eq(data.month[0].txk, 0);

	ck_assert_int_eq(data.totalrx, 0);
	ck_assert_int_eq(data.totaltx, 0);
	ck_assert_int_eq(data.totalrxk, 0);
	ck_assert_int_eq(data.totaltxk, 0);
	ck_assert_int_eq(data.currx, 0);
	ck_assert_int_eq(data.curtx, 0);
}
END_TEST

START_TEST(parseifinfo_1kb_change)
{
	initdb();
	data.btime = getbtime();
	data.lastupdated -= 100;
	data.currx = 1024;
	data.curtx = 1024;

	strcpy(data.interface, "eth0");
	ck_assert_int_eq(ibwadd("eth0", 10), 1);

	strcpy(ifinfo.name, "eth0");
	ifinfo.filled = 1;
	ifinfo.rx = 2048;
	ifinfo.tx = 2048;
	ifinfo.rxp = ifinfo.txp = 0;

	parseifinfo(0);

	ck_assert_int_eq(ifinfo.rx, 2048);
	ck_assert_int_eq(ifinfo.tx, 2048);
	ck_assert_int_eq(ifinfo.rxp, 0);
	ck_assert_int_eq(ifinfo.txp, 0);

	ck_assert_int_eq(data.day[0].rx, 0);
	ck_assert_int_eq(data.day[0].tx, 0);
	ck_assert_int_eq(data.day[0].rxk, 1);
	ck_assert_int_eq(data.day[0].txk, 1);

	ck_assert_int_eq(data.month[0].rx, 0);
	ck_assert_int_eq(data.month[0].tx, 0);
	ck_assert_int_eq(data.month[0].rxk, 1);
	ck_assert_int_eq(data.month[0].txk, 1);

	ck_assert_int_eq(data.totalrx, 0);
	ck_assert_int_eq(data.totaltx, 0);
	ck_assert_int_eq(data.totalrxk, 1);
	ck_assert_int_eq(data.totaltxk, 1);
	ck_assert_int_eq(data.currx, 2048);
	ck_assert_int_eq(data.curtx, 2048);
}
END_TEST

START_TEST(parseifinfo_newdb)
{
	initdb();
	data.btime = getbtime();
	data.lastupdated -= 100;
	data.currx = 1024;
	data.curtx = 1024;

	strcpy(data.interface, "eth0");
	ck_assert_int_eq(ibwadd("eth0", 10), 1);

	strcpy(ifinfo.name, "eth0");
	ifinfo.filled = 1;
	ifinfo.rx = 2048;
	ifinfo.tx = 2048;
	ifinfo.rxp = ifinfo.txp = 0;

	parseifinfo(1);

	ck_assert_int_eq(ifinfo.rx, 2048);
	ck_assert_int_eq(ifinfo.tx, 2048);
	ck_assert_int_eq(ifinfo.rxp, 0);
	ck_assert_int_eq(ifinfo.txp, 0);

	ck_assert_int_eq(data.day[0].rx, 0);
	ck_assert_int_eq(data.day[0].tx, 0);
	ck_assert_int_eq(data.day[0].rxk, 0);
	ck_assert_int_eq(data.day[0].txk, 0);

	ck_assert_int_eq(data.totalrx, 0);
	ck_assert_int_eq(data.totaltx, 0);
	ck_assert_int_eq(data.totalrxk, 0);
	ck_assert_int_eq(data.totaltxk, 0);
	ck_assert_int_eq(data.currx, 2048);
	ck_assert_int_eq(data.curtx, 2048);
}
END_TEST

START_TEST(parseifinfo_1kb_change_with_booted_system)
{
	initdb();
	data.btime = 0;
	data.lastupdated -= 100;
	data.currx = 1024;
	data.curtx = 1024;

	strcpy(data.interface, "eth0");
	ck_assert_int_eq(ibwadd("eth0", 10), 1);

	strcpy(ifinfo.name, "eth0");
	ifinfo.filled = 1;
	ifinfo.rx = 1024;
	ifinfo.tx = 1024;
	ifinfo.rxp = ifinfo.txp = 0;

	parseifinfo(0);

	ck_assert_int_eq(ifinfo.rx, 1024);
	ck_assert_int_eq(ifinfo.tx, 1024);
	ck_assert_int_eq(ifinfo.rxp, 0);
	ck_assert_int_eq(ifinfo.txp, 0);

	ck_assert_int_eq(data.day[0].rx, 0);
	ck_assert_int_eq(data.day[0].tx, 0);
	ck_assert_int_eq(data.day[0].rxk, 1);
	ck_assert_int_eq(data.day[0].txk, 1);

	ck_assert_int_eq(data.totalrx, 0);
	ck_assert_int_eq(data.totaltx, 0);
	ck_assert_int_eq(data.totalrxk, 1);
	ck_assert_int_eq(data.totaltxk, 1);
	ck_assert_int_eq(data.currx, 1024);
	ck_assert_int_eq(data.curtx, 1024);
}
END_TEST

START_TEST(parseifinfo_long_update_interval_causes_sync)
{
	initdb();
	data.btime = getbtime();
	data.lastupdated -= (60*MAXUPDATEINTERVAL + 10);

	strcpy(data.interface, "eth0");
	ck_assert_int_eq(ibwadd("eth0", 10), 1);

	strcpy(ifinfo.name, "eth0");
	ifinfo.filled = 1;
	ifinfo.rx = 1024;
	ifinfo.tx = 1024;
	ifinfo.rxp = ifinfo.txp = 0;

	parseifinfo(0);

	ck_assert_int_eq(ifinfo.rx, 1024);
	ck_assert_int_eq(ifinfo.tx, 1024);
	ck_assert_int_eq(ifinfo.rxp, 0);
	ck_assert_int_eq(ifinfo.txp, 0);

	ck_assert_int_eq(data.day[0].rx, 0);
	ck_assert_int_eq(data.day[0].tx, 0);
	ck_assert_int_eq(data.day[0].rxk, 0);
	ck_assert_int_eq(data.day[0].txk, 0);

	ck_assert_int_eq(data.totalrx, 0);
	ck_assert_int_eq(data.totaltx, 0);
	ck_assert_int_eq(data.totalrxk, 0);
	ck_assert_int_eq(data.totaltxk, 0);
	ck_assert_int_eq(data.currx, 1024);
	ck_assert_int_eq(data.curtx, 1024);
}
END_TEST

START_TEST(parseifinfo_hitting_maxbw_limit_causes_sync)
{
	initdb();
	data.btime = getbtime();
	data.lastupdated -= 1;
	data.currx = 1024;
	data.curtx = 1024;

	strcpy(data.interface, "eth0");
	ck_assert_int_eq(ibwadd("eth0", 10), 1);

	strcpy(ifinfo.name, "eth0");
	ifinfo.filled = 1;
	ifinfo.rx = 123456789;
	ifinfo.tx = 123456789;
	ifinfo.rxp = ifinfo.txp = 0;

	debug = 1;
	suppress_output();
	parseifinfo(0);

	ck_assert_int_eq(ifinfo.rx, 123456789);
	ck_assert_int_eq(ifinfo.tx, 123456789);
	ck_assert_int_eq(ifinfo.rxp, 0);
	ck_assert_int_eq(ifinfo.txp, 0);

	ck_assert_int_eq(data.day[0].rx, 0);
	ck_assert_int_eq(data.day[0].tx, 0);
	ck_assert_int_eq(data.day[0].rxk, 0);
	ck_assert_int_eq(data.day[0].txk, 0);

	ck_assert_int_eq(data.totalrx, 0);
	ck_assert_int_eq(data.totaltx, 0);
	ck_assert_int_eq(data.totalrxk, 0);
	ck_assert_int_eq(data.totaltxk, 0);
	ck_assert_int_eq(data.currx, 123456789);
	ck_assert_int_eq(data.curtx, 123456789);
}
END_TEST

START_TEST(parseifinfo_multiple_parses)
{
	initdb();
	data.btime = getbtime();
	data.lastupdated -= 20;
	data.currx = 0;
	data.curtx = 0;

	strcpy(data.interface, "eth0");
	ck_assert_int_eq(ibwadd("eth0", 10), 1);

	strcpy(ifinfo.name, "eth0");
	ifinfo.filled = 1;
	ifinfo.rx = 2049;
	ifinfo.tx = 2049;
	ifinfo.rxp = ifinfo.txp = 0;

	parseifinfo(0);

	ck_assert_int_eq(ifinfo.rx, 2049);
	ck_assert_int_eq(ifinfo.tx, 2049);
	ck_assert_int_eq(ifinfo.rxp, 1);
	ck_assert_int_eq(ifinfo.txp, 1);

	ck_assert_int_eq(data.day[0].rx, 0);
	ck_assert_int_eq(data.day[0].tx, 0);
	ck_assert_int_eq(data.day[0].rxk, 2);
	ck_assert_int_eq(data.day[0].txk, 2);

	ck_assert_int_eq(data.totalrx, 0);
	ck_assert_int_eq(data.totaltx, 0);
	ck_assert_int_eq(data.totalrxk, 2);
	ck_assert_int_eq(data.totaltxk, 2);
	ck_assert_int_eq(data.currx, 2048);
	ck_assert_int_eq(data.curtx, 2048);

	data.lastupdated -= 15;

	ifinfo.rx = 4098;
	ifinfo.tx = 4098;
	ifinfo.rxp = ifinfo.txp = 0;

	parseifinfo(0);

	ck_assert_int_eq(ifinfo.rx, 4098);
	ck_assert_int_eq(ifinfo.tx, 4098);
	ck_assert_int_eq(ifinfo.rxp, 2);
	ck_assert_int_eq(ifinfo.txp, 2);

	ck_assert_int_eq(data.day[0].rx, 0);
	ck_assert_int_eq(data.day[0].tx, 0);
	ck_assert_int_eq(data.day[0].rxk, 4);
	ck_assert_int_eq(data.day[0].txk, 4);

	ck_assert_int_eq(data.totalrx, 0);
	ck_assert_int_eq(data.totaltx, 0);
	ck_assert_int_eq(data.totalrxk, 4);
	ck_assert_int_eq(data.totaltxk, 4);
	ck_assert_int_eq(data.currx, 4096);
	ck_assert_int_eq(data.curtx, 4096);

	data.lastupdated -= 10;

	ifinfo.rx = 8192;
	ifinfo.tx = 8192;
	ifinfo.rxp = ifinfo.txp = 0;

	parseifinfo(0);

	ck_assert_int_eq(ifinfo.rx, 8192);
	ck_assert_int_eq(ifinfo.tx, 8192);
	ck_assert_int_eq(ifinfo.rxp, 0);
	ck_assert_int_eq(ifinfo.txp, 0);

	ck_assert_int_eq(data.day[0].rx, 0);
	ck_assert_int_eq(data.day[0].tx, 0);
	ck_assert_int_eq(data.day[0].rxk, 8);
	ck_assert_int_eq(data.day[0].txk, 8);

	ck_assert_int_eq(data.totalrx, 0);
	ck_assert_int_eq(data.totaltx, 0);
	ck_assert_int_eq(data.totalrxk, 8);
	ck_assert_int_eq(data.totaltxk, 8);
	ck_assert_int_eq(data.currx, 8192);
	ck_assert_int_eq(data.curtx, 8192);

	data.lastupdated -= 5;

	ifinfo.rx = 1048576;
	ifinfo.tx = 1048576;
	ifinfo.rxp = ifinfo.txp = 0;

	parseifinfo(0);

	ck_assert_int_eq(ifinfo.rx, 1048576);
	ck_assert_int_eq(ifinfo.tx, 1048576);
	ck_assert_int_eq(ifinfo.rxp, 0);
	ck_assert_int_eq(ifinfo.txp, 0);

	ck_assert_int_eq(data.day[0].rx, 1);
	ck_assert_int_eq(data.day[0].tx, 1);
	ck_assert_int_eq(data.day[0].rxk, 0);
	ck_assert_int_eq(data.day[0].txk, 0);

	ck_assert_int_eq(data.totalrx, 1);
	ck_assert_int_eq(data.totaltx, 1);
	ck_assert_int_eq(data.totalrxk, 0);
	ck_assert_int_eq(data.totaltxk, 0);
	ck_assert_int_eq(data.currx, 1048576);
	ck_assert_int_eq(data.curtx, 1048576);
}
END_TEST

START_TEST(getiflist_no_source)
{
	char *ifacelist;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(getiflist(&ifacelist), 0);
	ck_assert_str_eq(ifacelist, "");

	free(ifacelist);
}
END_TEST

START_TEST(getiflist_one_interface)
{
	char *ifacelist;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethunusual", 0, 0, 0, 0);

	ck_assert_int_eq(getiflist(&ifacelist), 1);
	ck_assert_str_eq(ifacelist, "lo ethunusual ");

	free(ifacelist);
}
END_TEST

START_TEST(getiflist_multiple_interfaces)
{
	char *ifacelist;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "random", 0, 0, 0, 0);
	fake_proc_net_dev("a", "interfaces", 0, 0, 0, 0);
	fake_proc_net_dev("a", "having", 0, 0, 0, 0);
	fake_proc_net_dev("a", "fun", 0, 0, 0, 0);
	fake_proc_net_dev("a", "i", 0, 0, 0, 0);

	ck_assert_int_eq(getiflist(&ifacelist), 1);
	ck_assert_str_eq(ifacelist, "lo random interfaces having fun i ");

	free(ifacelist);
}
END_TEST

START_TEST(readproc_no_file)
{
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(readproc("ethunusual"), 0);
}
END_TEST

START_TEST(readproc_not_found)
{
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethwrong", 10, 20, 30, 40);

	ck_assert_int_eq(readproc("ethunusual"), 0);
}
END_TEST

START_TEST(readproc_success)
{
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

START_TEST(getifinfo_not_found)
{
	suppress_output();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethwrong", 10, 20, 30, 40);

	ck_assert_int_eq(getifinfo("ethunusual"), 0);
}
END_TEST

START_TEST(getifinfo_success)
{
	suppress_output();

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethwrong", 10, 20, 30, 40);
	fake_proc_net_dev("a", "ethunusual", 1, 2, 3, 4);

	ck_assert_int_eq(getifinfo("ethunusual"), 1);
	ck_assert_str_eq(ifinfo.name, "ethunusual");
	ck_assert_int_eq(ifinfo.filled, 1);
	ck_assert_int_eq(ifinfo.rx, 1);
	ck_assert_int_eq(ifinfo.tx, 2);
	ck_assert_int_eq(ifinfo.rxp, 3);
	ck_assert_int_eq(ifinfo.txp, 4);
}
END_TEST

void add_ifinfo_tests(Suite *s)
{
	/* Ifinfo test cases */
	TCase *tc_ifinfo = tcase_create("Ifinfo");
	tcase_add_test(tc_ifinfo, parseifinfo_zero_change);
	tcase_add_test(tc_ifinfo, parseifinfo_1kb_change);
	tcase_add_test(tc_ifinfo, parseifinfo_newdb);
	tcase_add_test(tc_ifinfo, parseifinfo_1kb_change_with_booted_system);
	tcase_add_test(tc_ifinfo, parseifinfo_long_update_interval_causes_sync);
	tcase_add_test(tc_ifinfo, parseifinfo_hitting_maxbw_limit_causes_sync);
	tcase_add_test(tc_ifinfo, parseifinfo_multiple_parses);
#if defined(__linux__)
	tcase_add_test(tc_ifinfo, getiflist_no_source);
	tcase_add_test(tc_ifinfo, getiflist_one_interface);
	tcase_add_test(tc_ifinfo, getiflist_multiple_interfaces);
	tcase_add_test(tc_ifinfo, readproc_no_file);
	tcase_add_test(tc_ifinfo, readproc_not_found);
	tcase_add_test(tc_ifinfo, readproc_success);
	tcase_add_test(tc_ifinfo, getifinfo_not_found);
	tcase_add_test(tc_ifinfo, getifinfo_success);
#endif
	suite_add_tcase(s, tc_ifinfo);
}
