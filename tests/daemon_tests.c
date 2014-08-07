#include "vnstat_tests.h"
#include "daemon_tests.h"
#include "common.h"
#include "dbaccess.h"
#include "dbcache.h"
#include "cfg.h"
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

START_TEST(addinterfaces_does_nothing_with_no_files)
{
	defaultcfg();
	suppress_output();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ck_assert_int_eq(addinterfaces(TESTDBDIR), 0);
}
END_TEST

START_TEST(addinterfaces_adds_interfaces)
{
	defaultcfg();
	suppress_output();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	fake_proc_net_dev("w", "ethone", 1, 2, 3, 4);
	fake_proc_net_dev("a", "lo0", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethtwo", 5, 6, 7, 8);
	fake_proc_net_dev("a", "sit0", 0, 0, 0, 0);

	ck_assert_int_eq(addinterfaces(TESTDBDIR), 2);

	ck_assert_int_eq(check_dbfile_exists("ethone", sizeof(DATA)), 1);
	ck_assert_int_eq(check_dbfile_exists(".ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("ethtwo", sizeof(DATA)), 1);
	ck_assert_int_eq(check_dbfile_exists(".ethtwo", sizeof(DATA)), 0);
}
END_TEST

START_TEST(initdstate_does_not_crash)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
}
END_TEST

START_TEST(preparedatabases_exits_with_no_database_dir)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	suppress_output();

	preparedatabases(&s);
}
END_TEST

START_TEST(preparedatabases_exits_with_no_databases)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	suppress_output();
	ck_assert_int_eq(clean_testdbdir(), 1);

	preparedatabases(&s);
}
END_TEST

START_TEST(preparedatabases_with_no_databases_creates_databases)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	suppress_output();
	strncpy_nt(s.dirname, TESTDBDIR, 512);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	fake_proc_net_dev("w", "ethone", 1, 2, 3, 4);
	fake_proc_net_dev("a", "lo0", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethtwo", 5, 6, 7, 8);
	fake_proc_net_dev("a", "sit0", 0, 0, 0, 0);

	preparedatabases(&s);

	ck_assert_int_eq(check_dbfile_exists("ethone", sizeof(DATA)), 1);
	ck_assert_int_eq(check_dbfile_exists(".ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("ethtwo", sizeof(DATA)), 1);
	ck_assert_int_eq(check_dbfile_exists(".ethtwo", sizeof(DATA)), 0);
}
END_TEST

START_TEST(setsignaltraps_does_not_exit)
{
	intsignal = 1;
	setsignaltraps();
	ck_assert_int_eq(intsignal, 0);
}
END_TEST

START_TEST(filldatabaselist_exits_with_no_database_dir)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	disable_logprints();
	strncpy_nt(s.dirname, TESTDBDIR, 512);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);

	filldatabaselist(&s);
}
END_TEST

START_TEST(filldatabaselist_does_not_exit_with_empty_database_dir)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	disable_logprints();
	strncpy_nt(s.dirname, TESTDBDIR, 512);
	s.sync = 1;
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);

	filldatabaselist(&s);

	ck_assert_int_eq(s.dbcount, 0);
	ck_assert_int_eq(s.sync, 0);
	ck_assert_int_eq(s.updateinterval, 120);
}
END_TEST

START_TEST(filldatabaselist_adds_databases)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	disable_logprints();
	strncpy_nt(s.dirname, TESTDBDIR, 512);
	s.sync = 1;
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("name1"), 1);
	ck_assert_int_eq(create_zerosize_dbfile("name2"), 1);
	ck_assert_int_eq(check_dbfile_exists("name1", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".name1", 0), 0);
	ck_assert_int_eq(check_dbfile_exists("name2", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".name2", 0), 0);

	filldatabaselist(&s);

	ck_assert_int_eq(cachecount(), 2);
	ck_assert_int_eq(cacheactivecount(), 2);
	ck_assert_int_eq(check_dbfile_exists("name1", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".name1", 0), 0);
	ck_assert_int_eq(check_dbfile_exists("name2", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".name2", 0), 0);
	ck_assert_int_eq(s.dbcount, 2);
	ck_assert_int_eq(s.sync, 0);
	ck_assert_int_eq(s.updateinterval, 0);
	ck_assert_int_eq(intsignal, 42);
}
END_TEST

START_TEST(adjustsaveinterval_with_empty_cache)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	s.saveinterval = 0;
	ck_assert_int_eq(cacheactivecount(), 0);

	adjustsaveinterval(&s);

	ck_assert_int_eq(s.saveinterval, cfg.offsaveinterval * 60);
}
END_TEST

START_TEST(adjustsaveinterval_with_filled_cache)
{
	DSTATE s;
	defaultcfg();
	initdb();
	initdstate(&s);
	s.saveinterval = 0;
	strcpy(data.interface, "name1");
	ck_assert_int_eq(cacheupdate(), 1);
	ck_assert_int_eq(cacheactivecount(), 1);

	adjustsaveinterval(&s);

	ck_assert_int_eq(s.saveinterval, cfg.saveinterval * 60);
}
END_TEST

START_TEST(checkdbsaveneed_has_no_need)
{
	DSTATE s;
	initdstate(&s);
	s.dodbsave = 2;
	s.current = 10;
	s.prevdbsave = 0;
	s.saveinterval = 30;
	s.forcesave = 0;

	checkdbsaveneed(&s);

	ck_assert_int_eq(s.dodbsave, 0);
	ck_assert_int_ne(s.prevdbsave, s.current);
}
END_TEST

START_TEST(checkdbsaveneed_is_forced)
{
	DSTATE s;
	initdstate(&s);
	s.dodbsave = 2;
	s.current = 10;
	s.prevdbsave = 0;
	s.saveinterval = 30;
	s.forcesave = 1;

	checkdbsaveneed(&s);

	ck_assert_int_eq(s.dodbsave, 1);
	ck_assert_int_eq(s.prevdbsave, s.current);
	ck_assert_int_eq(s.forcesave, 0);
}
END_TEST

START_TEST(checkdbsaveneed_needs)
{
	DSTATE s;
	initdstate(&s);
	s.dodbsave = 2;
	s.current = 60;
	s.prevdbsave = 5;
	s.saveinterval = 30;
	s.forcesave = 0;

	checkdbsaveneed(&s);

	ck_assert_int_eq(s.dodbsave, 1);
	ck_assert_int_eq(s.prevdbsave, s.current);
	ck_assert_int_eq(s.forcesave, 0);
}
END_TEST

START_TEST(processdatalist_empty_does_nothing)
{
	DSTATE s;
	initdstate(&s);

	processdatalist(&s);
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
	tcase_add_test(tc_daemon, addinterfaces_does_nothing_with_no_files);
	tcase_add_test(tc_daemon, addinterfaces_adds_interfaces);
	tcase_add_test(tc_daemon, initdstate_does_not_crash);
	tcase_add_exit_test(tc_daemon, preparedatabases_exits_with_no_database_dir, 1);
	tcase_add_exit_test(tc_daemon, preparedatabases_exits_with_no_databases, 1);
	tcase_add_test(tc_daemon, preparedatabases_with_no_databases_creates_databases);
	tcase_add_test(tc_daemon, setsignaltraps_does_not_exit);
	tcase_add_exit_test(tc_daemon, filldatabaselist_exits_with_no_database_dir, 1);
	tcase_add_test(tc_daemon, filldatabaselist_does_not_exit_with_empty_database_dir);
	tcase_add_test(tc_daemon, filldatabaselist_adds_databases);
	tcase_add_test(tc_daemon, adjustsaveinterval_with_empty_cache);
	tcase_add_test(tc_daemon, adjustsaveinterval_with_filled_cache);
	tcase_add_test(tc_daemon, checkdbsaveneed_has_no_need);
	tcase_add_test(tc_daemon, checkdbsaveneed_is_forced);
	tcase_add_test(tc_daemon, checkdbsaveneed_needs);
	tcase_add_test(tc_daemon, processdatalist_empty_does_nothing);
	suite_add_tcase(s, tc_daemon);
}
