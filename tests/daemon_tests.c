#include "common.h"
#include "vnstat_tests.h"
#include "daemon_tests.h"
#include "dbaccess.h"
#include "datacache.h"
#include "dbsql.h"
#include "ifinfo.h"
#include "cfg.h"
#include "ibw.h"
#include "fs.h"
#include "daemon.h"

START_TEST(debugtimestamp_does_not_exit)
{
	suppress_output();
	debugtimestamp();
}
END_TEST

START_TEST(addinterfaces_does_nothing_with_no_files)
{
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ck_assert_int_eq(addinterfaces(&s), 0);
}
END_TEST

START_TEST(addinterfaces_adds_interfaces)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	fake_proc_net_dev("w", "ethone", 1, 2, 3, 4);
	fake_proc_net_dev("a", "lo0", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethtwo", 5, 6, 7, 8);
	fake_proc_net_dev("a", "sit0", 0, 0, 0, 0);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = (int)addinterfaces(&s);
	ck_assert_int_eq(ret, 2);

	ck_assert_int_eq(db_getinterfacecountbyname("ethone"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("ethtwo"), 1);

	ck_assert_int_eq(check_dbfile_exists("ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("ethtwo", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethtwo", sizeof(DATA)), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(addinterfaces_adds_only_new_interfaces)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	fake_proc_net_dev("w", "ethone", 1, 2, 3, 4);
	fake_proc_net_dev("a", "lo0", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethtwo", 5, 6, 7, 8);
	fake_proc_net_dev("a", "sit0", 0, 0, 0, 0);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = (int)addinterfaces(&s);
	ck_assert_int_eq(ret, 2);

	ck_assert_int_eq(db_getinterfacecountbyname("ethone"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("ethtwo"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eththree"), 0);
	ck_assert_int_eq(db_getinterfacecountbyname("lo0"), 0);
	ck_assert_int_eq(db_getinterfacecountbyname("sit0"), 0);

	/* legacy database files should not get created */
	ck_assert_int_eq(check_dbfile_exists("ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("ethtwo", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethtwo", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("eththree", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".eththree", sizeof(DATA)), 0);

	fake_proc_net_dev("a", "eththree", 9, 10, 11, 12);

	ret = (int)addinterfaces(&s);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_getinterfacecountbyname("ethone"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("ethtwo"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eththree"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("lo0"), 0);
	ck_assert_int_eq(db_getinterfacecountbyname("sit0"), 0);

	/* legacy database files should still not get created */
	ck_assert_int_eq(check_dbfile_exists("ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("ethtwo", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethtwo", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("eththree", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".eththree", sizeof(DATA)), 0);
	ck_assert_int_eq(datacache_count(&s.dcache), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(addinterfaces_adds_to_cache_when_running)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	fake_proc_net_dev("w", "ethone", 1, 2, 3, 4);
	fake_proc_net_dev("a", "ethtwo", 5, 6, 7, 8);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(datacache_count(&s.dcache), 0);

	s.running = 1;
	ret = (int)addinterfaces(&s);
	ck_assert_int_eq(ret, 2);
	ck_assert_int_eq(datacache_count(&s.dcache), 2);

	ck_assert_int_eq(db_getinterfacecountbyname("ethone"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("ethtwo"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eththree"), 0);
	ck_assert_int_eq(db_getinterfacecountbyname("lo0"), 0);
	ck_assert_int_eq(db_getinterfacecountbyname("sit0"), 0);

	ck_assert_int_eq(check_dbfile_exists("ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("ethtwo", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethtwo", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("eththree", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".eththree", sizeof(DATA)), 0);

	fake_proc_net_dev("a", "eththree", 9, 10, 11, 12);

	ret = (int)addinterfaces(&s);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datacache_count(&s.dcache), 3);

	ck_assert_int_eq(db_getinterfacecountbyname("ethone"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("ethtwo"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eththree"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("lo0"), 0);
	ck_assert_int_eq(db_getinterfacecountbyname("sit0"), 0);

	ck_assert_int_eq(check_dbfile_exists("ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("ethtwo", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethtwo", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("eththree", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".eththree", sizeof(DATA)), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
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
	ck_assert_int_eq(remove_directory(TESTDIR), 1);

	preparedatabases(&s);
}
END_TEST

START_TEST(preparedatabases_exits_with_no_databases)
{
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);

	preparedatabases(&s);
}
END_TEST

START_TEST(preparedatabases_exits_with_no_databases_and_noadd)
{
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	s.noadd = 1;
	suppress_output();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);

	preparedatabases(&s);
}
END_TEST

START_TEST(preparedatabases_with_no_databases_creates_databases)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	fake_proc_net_dev("w", "ethone", 1, 2, 3, 4);
	fake_proc_net_dev("a", "lo0", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethtwo", 5, 6, 7, 8);
	fake_proc_net_dev("a", "sit0", 0, 0, 0, 0);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	preparedatabases(&s);

	ck_assert_int_eq(db_getinterfacecountbyname("ethone"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("ethtwo"), 1);

	ck_assert_int_eq(check_dbfile_exists("ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethone", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists("ethtwo", sizeof(DATA)), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethtwo", sizeof(DATA)), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
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
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);

	filldatabaselist(&s);
}
END_TEST

START_TEST(filldatabaselist_does_not_exit_with_empty_database_dir)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	disable_logprints();
	s.sync = 1;
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	filldatabaselist(&s);

	ck_assert_int_eq(s.dbifcount, 0);
	ck_assert_int_eq(s.sync, 0);
	ck_assert_int_eq(s.updateinterval, 120);
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(filldatabaselist_adds_databases)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	disable_logprints();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	s.sync = 1;
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("name1"), 1);
	ck_assert_int_eq(create_zerosize_dbfile("name2"), 1);
	ck_assert_int_eq(check_dbfile_exists("name1", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".name1", 0), 0);
	ck_assert_int_eq(check_dbfile_exists("name2", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".name2", 0), 0);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	filldatabaselist(&s);

	/* filldatabaselist() doesn't import legacy dbs */
	ck_assert_int_eq(datacache_count(&s.dcache), 2);
	ck_assert_int_eq(datacache_activecount(&s.dcache), 2);
	ck_assert_int_eq(check_dbfile_exists("name1", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".name1", 0), 0);
	ck_assert_int_eq(check_dbfile_exists("name2", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".name2", 0), 0);
	ck_assert_int_eq(s.dbifcount, 2);
	ck_assert_int_eq(s.sync, 0);
	ck_assert_int_eq(s.updateinterval, 0);
	ck_assert_int_eq(intsignal, 42);
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(adjustsaveinterval_with_empty_cache)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	s.saveinterval = 0;
	ck_assert_int_eq(datacache_activecount(&s.dcache), 0);

	adjustsaveinterval(&s);

	ck_assert_int_eq(s.saveinterval, cfg.offsaveinterval * 60);
}
END_TEST

START_TEST(adjustsaveinterval_with_filled_cache)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	s.saveinterval = 0;

	ret = datacache_add(&s.dcache, "name1", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datacache_activecount(&s.dcache), 1);

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
	s.current = 125;
	s.prevdbsave = 110;
	s.saveinterval = 30;
	s.forcesave = 1;

	checkdbsaveneed(&s);

	ck_assert_int_eq(s.dodbsave, 1);
	ck_assert_int_eq(s.prevdbsave, 120);
	ck_assert_int_eq(s.forcesave, 0);
}
END_TEST

START_TEST(checkdbsaveneed_needs)
{
	DSTATE s;
	initdstate(&s);
	s.dodbsave = 2;
	s.current = 65;
	s.prevdbsave = 5;
	s.saveinterval = 30;
	s.forcesave = 0;

	checkdbsaveneed(&s);

	ck_assert_int_eq(s.dodbsave, 1);
	ck_assert_int_eq(s.prevdbsave, 60);
	ck_assert_int_eq(s.forcesave, 0);
}
END_TEST

START_TEST(processdatacache_empty_does_nothing)
{
	DSTATE s;
	initdstate(&s);

	processdatacache(&s);
}
END_TEST

START_TEST(processdatacache_can_process_things)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	suppress_output();

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	debug = 1;

	s.iflisthash = 42;
	s.bootdetected = 1;
	s.dodbsave = 1;
	s.cleanuphour = getcurrenthour() + 1;

	ck_assert_int_eq(datacache_count(&s.dcache), 0);
	ret = datacache_add(&s.dcache, "ethnotindb", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&s.dcache, "ethonlyindb", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&s.dcache, "ethexisting", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datacache_count(&s.dcache), 3);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethonlyindb");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethexisting");
	ck_assert_int_eq(ret, 1);

	fake_proc_net_dev("w", "ethexisting", 10, 20, 30, 40);

	processdatacache(&s);

	ck_assert_int_eq(s.iflisthash, 0);
	ck_assert_int_eq(s.bootdetected, 0);
	ck_assert_int_eq(s.dodbsave, 0);
	ck_assert_int_eq(datacache_count(&s.dcache), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(handleintsignals_handles_no_signal)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	s.running = 1;
	s.dbifcount = 1;

	intsignal = 0;
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 1);
	ck_assert_int_eq(s.dbifcount, 1);
}
END_TEST

START_TEST(handleintsignals_handles_42)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	s.running = 1;
	s.dbifcount = 1;

	intsignal = 42;
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 1);
	ck_assert_int_eq(s.dbifcount, 1);
}
END_TEST

START_TEST(handleintsignals_handles_unknown_signal)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	s.running = 1;
	s.dbifcount = 1;

	disable_logprints();

	intsignal = 43;
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 1);
	ck_assert_int_eq(s.dbifcount, 1);
}
END_TEST

START_TEST(handleintsignals_handles_sigterm)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	s.running = 1;
	s.dbifcount = 1;

	disable_logprints();

	intsignal = SIGTERM;
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 0);
	ck_assert_int_eq(s.dbifcount, 1);
}
END_TEST

START_TEST(handleintsignals_handles_sigint)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	s.running = 1;
	s.dbifcount = 1;

	disable_logprints();

	s.running = 1;
	intsignal = SIGINT;
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 0);
	ck_assert_int_eq(s.dbifcount, 1);
}
END_TEST

START_TEST(handleintsignals_handles_sighup)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	s.running = 1;
	s.dbifcount = 1;

	disable_logprints();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	filldatabaselist(&s);

	disable_logprints();

	s.running = 1;
	intsignal = SIGHUP;
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 1);
	ck_assert_int_eq(s.dbifcount, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(preparedirs_with_no_dir)
{
	char logdir[500], piddir[500];

	DSTATE s;
	initdstate(&s);
	defaultcfg();
	cfg.uselogging = 1;
	s.rundaemon = 1;
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	snprintf(logdir, 500, "%s/log/vnstat", TESTDIR);
	snprintf(piddir, 500, "%s/pid/vnstat", TESTDIR);
	snprintf(cfg.logfile, 512, "%s/vnstat.log", logdir);
	snprintf(cfg.pidfile, 512, "%s/vnstat.pid", piddir);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(direxists(TESTDBDIR), 0);
	ck_assert_int_eq(direxists(logdir), 0);
	ck_assert_int_eq(direxists(piddir), 0);
	preparedirs(&s);
	ck_assert_int_eq(direxists(TESTDBDIR), 1);
	ck_assert_int_eq(direxists(logdir), 1);
	ck_assert_int_eq(direxists(piddir), 1);
}
END_TEST

START_TEST(preparedirs_with_dir)
{
	char logdir[500], piddir[500];

	DSTATE s;
	initdstate(&s);
	defaultcfg();
	cfg.uselogging = 1;
	s.rundaemon = 1;
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	snprintf(logdir, 500, "%s/log/vnstat", TESTDIR);
	snprintf(piddir, 500, "%s/pid/vnstat", TESTDIR);
	snprintf(cfg.logfile, 512, "%s/vnstat.log", logdir);
	snprintf(cfg.pidfile, 512, "%s/vnstat.pid", piddir);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(direxists(TESTDBDIR), 0);
	ck_assert_int_eq(mkpath(TESTDBDIR, 0775), 1);
	ck_assert_int_eq(direxists(TESTDBDIR), 1);
	ck_assert_int_eq(direxists(logdir), 0);
	ck_assert_int_eq(direxists(piddir), 0);
	preparedirs(&s);
	ck_assert_int_eq(direxists(TESTDBDIR), 1);
	ck_assert_int_eq(direxists(logdir), 1);
	ck_assert_int_eq(direxists(piddir), 1);
}
END_TEST

START_TEST(interfacechangecheck_with_no_interfaces)
{
	DSTATE s;

	initdstate(&s);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	interfacechangecheck(&s);
	ck_assert_int_eq(s.iflisthash, 0);
	ck_assert_int_eq(s.forcesave, 0);
}
END_TEST

START_TEST(interfacechangecheck_with_empty_cache)
{
	DSTATE s;

	initdstate(&s);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethsomething", 1, 2, 3, 4);
	fake_proc_net_dev("a", "ethelse", 5, 6, 7, 8);

	interfacechangecheck(&s);
	ck_assert_int_ne(s.iflisthash, 0);
	ck_assert_int_eq(s.forcesave, 0);
}
END_TEST

START_TEST(interfacechangecheck_with_no_changes_in_iflist)
{
	DSTATE s;
	uint32_t ifhash;
	char *ifacelist;

	initdstate(&s);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethsomething", 1, 2, 3, 4);
	fake_proc_net_dev("a", "ethelse", 5, 6, 7, 8);
	ck_assert_int_ne(getifliststring(&ifacelist, 0), 0);
	ifhash = simplehash(ifacelist, (int)strlen(ifacelist));
	s.iflisthash = ifhash;

	interfacechangecheck(&s);
	ck_assert_int_eq(s.iflisthash, ifhash);
	ck_assert_int_eq(s.forcesave, 0);
}
END_TEST

START_TEST(interfacechangecheck_with_filled_cache)
{
	int ret;
	DSTATE s;
	datacache *iterator;

	defaultcfg();
	initdstate(&s);
	disable_logprints();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	s.iflisthash = 123;

	ck_assert_int_eq(datacache_count(&s.dcache), 0);
	ret = datacache_add(&s.dcache, "ethbasic", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&s.dcache, "ethactive", 0);
	ck_assert_int_eq(ret, 1);

	/* cache data needs to appear filled during this test */
	iterator = s.dcache;
	while (iterator != NULL) {
		iterator->filled = 1;
		iterator = iterator->next;
	}

	ck_assert_int_eq(datacache_count(&s.dcache), 2);
	ck_assert_int_eq(datacache_activecount(&s.dcache), 2);

	fake_proc_net_dev("w", "ethbasic", 1, 2, 3, 4);

	interfacechangecheck(&s);
	ck_assert_int_ne(s.iflisthash, 0);
	ck_assert_int_eq(s.forcesave, 1);
	ck_assert_int_eq(datacache_count(&s.dcache), 2);
	ck_assert_int_eq(datacache_activecount(&s.dcache), 1);
}
END_TEST

START_TEST(simplehash_with_empty_strings)
{
	ck_assert_int_eq(simplehash(NULL, 10), 0);
	ck_assert_int_eq(simplehash("empty", 0), 0);
}
END_TEST

START_TEST(simplehash_with_simple_strings)
{
	ck_assert_int_eq(simplehash("0", 1), 49);
	ck_assert_int_eq(simplehash("1", 1), 50);
	ck_assert_int_eq(simplehash("12", 2), 101);
}
END_TEST

START_TEST(initcachevalues_does_not_init_without_database)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	disable_logprints();

	ret = datacache_add(&s.dcache, "eth0", 0);
	ck_assert_int_eq(ret, 1);

	ret = initcachevalues(&s, &s.dcache);
	ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(initcachevalues_does_init)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	disable_logprints();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setcounters("eth0", 1, 2);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&s.dcache, "eth0", 0);
	ck_assert_int_eq(ret, 1);

	ret = initcachevalues(&s, &s.dcache);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(s.dcache->currx, 1);
	ck_assert_int_eq(s.dcache->curtx, 2);
	ck_assert_int_ne(s.dcache->updated, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(getcurrenthour_returns_something_realistic)
{
	int ret;

	ret = getcurrenthour();
	ck_assert_int_ge(ret, 0);
	ck_assert_int_le(ret, 23);
}
END_TEST

START_TEST(waittimesync_does_not_wait_unless_configured_to_do_so)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	disable_logprints();
	cfg.timesyncwait = 0;

	ret = waittimesync(&s);
	ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(waittimesync_does_not_wait_with_no_interfaces)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	debug = 1;

	ret = waittimesync(&s);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(s.prevdbsave, 0);
}
END_TEST

START_TEST(waittimesync_does_not_wait_with_new_interfaces)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	debug = 1;
	cfg.timesyncwait = 60;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	/* 'updated' needs to be slightly adjusted in order to be sure to never trigger an error */
	ret = db_exec("update interface set updated=datetime('now', '-2 seconds', 'localtime') where id=1;");
	ck_assert_int_eq(ret, 1);

	filldatabaselist(&s);
	s.prevdbsave = 0;

	ret = waittimesync(&s);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_ne(s.prevdbsave, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(waittimesync_knows_when_to_wait)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	debug = 1;
	cfg.timesyncwait = 60;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	ret = db_exec("update interface set updated=datetime('now', '+7 days', 'localtime') where id=1;");
	ck_assert_int_eq(ret, 1);

	filldatabaselist(&s);
	s.prevdbsave = 0;

	ret = waittimesync(&s);
	ck_assert_int_eq(ret, 1);

	s.prevdbsave = time(NULL) - 100;

	ret = waittimesync(&s);
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(waittimesync_knows_when_to_give_up)
{
	int ret;
	DSTATE s;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	debug = 1;
	cfg.timesyncwait = 60;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	ret = db_exec("update interface set updated=datetime('now', '+7 days', 'localtime') where id=1;");
	ck_assert_int_eq(ret, 1);

	filldatabaselist(&s);
	s.prevdbsave = 0;

	ret = waittimesync(&s);
	ck_assert_int_eq(ret, 1);

	s.prevdbupdate -= 5000;

	ret = waittimesync(&s);
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(detectboot_sets_btime_if_missing_from_database)
{
	int ret;
	DSTATE s;
	char *buffer;

	defaultcfg();
	initdstate(&s);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_exec("delete from info where name='btime';");
	ck_assert_int_eq(ret, 1);

	buffer = db_getinfo("btime");
	ck_assert_int_eq((int)strlen(buffer), 0);

	ck_assert_int_eq(s.bootdetected, 0);

	detectboot(&s);

	ck_assert_int_eq(s.bootdetected, 0);

	buffer = db_getinfo("btime");
	ck_assert_int_ne((int)strlen(buffer), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(detectboot_sets_btime_for_new_database)
{
	int ret;
	DSTATE s;
	char *buffer;
	char temp[64];

	defaultcfg();
	initdstate(&s);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	buffer = db_getinfo("btime");
	ck_assert_int_ne((int)strlen(buffer), 0);

	strncpy_nt(temp, buffer, 64);
	ck_assert_str_eq(buffer, temp);

	ck_assert_int_eq(s.bootdetected, 0);

	detectboot(&s);

	ck_assert_int_eq(s.bootdetected, 0);

	buffer = db_getinfo("btime");
	ck_assert_int_ne((int)strlen(buffer), 0);

	ck_assert_str_ne(buffer, temp);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(detectboot_can_detect_boot)
{
	int ret;
	DSTATE s;
	char *buffer;
	char temp[64];

	defaultcfg();
	initdstate(&s);
	suppress_output();
	debug = 1;
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	db_setinfo("btime", "1", 1);

	buffer = db_getinfo("btime");
	ck_assert_int_ne((int)strlen(buffer), 0);

	strncpy_nt(temp, buffer, 64);
	ck_assert_str_eq(buffer, temp);

	ck_assert_int_eq(s.bootdetected, 0);

	detectboot(&s);

	ck_assert_int_eq(s.bootdetected, 1);

	buffer = db_getinfo("btime");
	ck_assert_int_ne((int)strlen(buffer), 0);

	ck_assert_str_ne(buffer, temp);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(handledatabaseerror_exits_on_fatal_error)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	disable_logprints();

	db_errcode = SQLITE_ERROR;
	handledatabaseerror(&s);
}
END_TEST

START_TEST(handledatabaseerror_does_not_exit_if_limit_is_not_reached)
{
	int i;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	disable_logprints();

	ck_assert_int_eq(s.dbretrycount, 0);

	db_errcode = SQLITE_BUSY;
	handledatabaseerror(&s);

	ck_assert_int_eq(s.dbretrycount, 1);

	for (i = 1; i < DBRETRYLIMIT - 1; i++) {
		handledatabaseerror(&s);
	}

	ck_assert_int_eq(s.dbretrycount, DBRETRYLIMIT - 1);
}
END_TEST

START_TEST(handledatabaseerror_exits_if_limit_is_reached)
{
	int i;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	disable_logprints();

	ck_assert_int_eq(s.dbretrycount, 0);

	db_errcode = SQLITE_BUSY;
	handledatabaseerror(&s);

	ck_assert_int_eq(s.dbretrycount, 1);

	for (i = 1; i < DBRETRYLIMIT; i++) {
		handledatabaseerror(&s);
	}
}
END_TEST

START_TEST(cleanremovedinterfaces_allows_interfaces_to_be_removed)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	disable_logprints();

	ck_assert_int_eq(datacache_count(&s.dcache), 0);
	ret = datacache_add(&s.dcache, "ethnotindb1", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&s.dcache, "ethindb1", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&s.dcache, "ethnotindb2", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&s.dcache, "ethindb2", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&s.dcache, "ethindb3", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&s.dcache, "ethnotindb3", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&s.dcache, "ethnotindb4", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datacache_count(&s.dcache), 7);
	s.dbifcount = 7;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethindb1");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethindb2");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethindb3");
	ck_assert_int_eq(ret, 1);

	cleanremovedinterfaces(&s);

	ck_assert_int_eq(s.dbifcount, 3);
	ck_assert_int_eq(datacache_count(&s.dcache), 3);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(processifinfo_syncs_when_needed)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);

	ifinfo.rx = 11;
	ifinfo.tx = 22;

	ck_assert_int_eq(datacache_count(&s.dcache), 0);
	ret = datacache_add(&s.dcache, "ethsomething", 1);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(s.dcache->syncneeded, 1);
	ck_assert_int_eq(s.dcache->currx, 0);
	ck_assert_int_eq(s.dcache->curtx, 0);

	ret = processifinfo(&s, &s.dcache);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 11);
	ck_assert_int_eq(s.dcache->curtx, 22);
}
END_TEST

START_TEST(processifinfo_skips_update_if_timestamps_make_no_sense)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);

	ifinfo.rx = 11;
	ifinfo.tx = 22;
	ifinfo.timestamp = 250;

	ck_assert_int_eq(datacache_count(&s.dcache), 0);
	ret = datacache_add(&s.dcache, "ethsomething", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 0);
	ck_assert_int_eq(s.dcache->curtx, 0);
	s.dcache->updated = 300;

	ret = processifinfo(&s, &s.dcache);
	ck_assert_int_eq(ret, 0);

	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 0);
	ck_assert_int_eq(s.dcache->curtx, 0);
}
END_TEST

START_TEST(processifinfo_exits_if_timestamps_really_make_no_sense)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	disable_logprints();

	ifinfo.rx = 11;
	ifinfo.tx = 22;
	ifinfo.timestamp = 250;

	ck_assert_int_eq(datacache_count(&s.dcache), 0);
	ret = datacache_add(&s.dcache, "ethsomething", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 0);
	ck_assert_int_eq(s.dcache->curtx, 0);
	s.dcache->updated = 100000;

	processifinfo(&s, &s.dcache);
}
END_TEST

START_TEST(processifinfo_syncs_if_timestamps_match)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);

	ifinfo.rx = 11;
	ifinfo.tx = 22;
	ifinfo.timestamp = 250;

	ck_assert_int_eq(datacache_count(&s.dcache), 0);
	ret = datacache_add(&s.dcache, "ethsomething", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 0);
	ck_assert_int_eq(s.dcache->curtx, 0);
	s.dcache->updated = 250;

	ret = processifinfo(&s, &s.dcache);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 11);
	ck_assert_int_eq(s.dcache->curtx, 22);
	ck_assert_ptr_eq(s.dcache->log, NULL);
}
END_TEST

START_TEST(processifinfo_adds_traffic)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	suppress_output();
	debug = 1;

	ifinfo.rx = 11;
	ifinfo.tx = 22;
	ifinfo.timestamp = 250;

	ck_assert_int_eq(datacache_count(&s.dcache), 0);
	ret = datacache_add(&s.dcache, "ethsomething", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 0);
	ck_assert_int_eq(s.dcache->curtx, 0);
	s.dcache->updated = 200;

	ret = ibwadd("ethsomething", 1000);
	ck_assert_int_eq(ret, 1);

	ret = processifinfo(&s, &s.dcache);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 11);
	ck_assert_int_eq(s.dcache->curtx, 22);
	ck_assert_ptr_ne(s.dcache->log, NULL);
}
END_TEST

START_TEST(processifinfo_does_not_add_traffic_when_over_limit)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	suppress_output();
	debug = 1;

	ifinfo.rx = 1111111;
	ifinfo.tx = 2222222;
	ifinfo.timestamp = 250;
	cfg.trafficlessentries = 0;

	ck_assert_int_eq(datacache_count(&s.dcache), 0);
	ret = datacache_add(&s.dcache, "ethsomething", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 0);
	ck_assert_int_eq(s.dcache->curtx, 0);
	s.dcache->updated = 249;

	ret = ibwadd("ethsomething", 1);
	ck_assert_int_eq(ret, 1);

	ret = processifinfo(&s, &s.dcache);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 1111111);
	ck_assert_int_eq(s.dcache->curtx, 2222222);
	ck_assert_ptr_eq(s.dcache->log, NULL);
}
END_TEST

START_TEST(processifinfo_adds_zero_traffic_when_over_limit)
{
	int ret;
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	suppress_output();
	debug = 1;

	ifinfo.rx = 1111111;
	ifinfo.tx = 2222222;
	ifinfo.timestamp = 250;
	cfg.trafficlessentries = 1;

	ck_assert_int_eq(datacache_count(&s.dcache), 0);
	ret = datacache_add(&s.dcache, "ethsomething", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 0);
	ck_assert_int_eq(s.dcache->curtx, 0);
	s.dcache->updated = 249;

	ret = ibwadd("ethsomething", 1);
	ck_assert_int_eq(ret, 1);

	ret = processifinfo(&s, &s.dcache);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(s.dcache->syncneeded, 0);
	ck_assert_int_eq(s.dcache->currx, 1111111);
	ck_assert_int_eq(s.dcache->curtx, 2222222);
	ck_assert_ptr_ne(s.dcache->log, NULL);
	ck_assert_int_eq(s.dcache->log->rx, 0);
	ck_assert_int_eq(s.dcache->log->tx, 0);
}
END_TEST

START_TEST(datacache_status_can_handle_nothing)
{
	datacache *dcache;
	disable_logprints();
	dcache = NULL;

	datacache_status(&dcache);
}
END_TEST

START_TEST(datacache_status_can_show_limits)
{
	int ret;
	datacache *dcache;
	defaultcfg();
	disable_logprints();
	dcache = NULL;

	ret = datacache_add(&dcache, "ethdefault", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&dcache, "ethslow", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&dcache, "ethfast", 0);
	ck_assert_int_eq(ret, 1);
	ret = datacache_add(&dcache, "ethnolimit", 0);
	ck_assert_int_eq(ret, 1);

	ret = ibwadd("ethslow", 1);
	ck_assert_int_eq(ret, 1);
	ret = ibwadd("ethfast", 1000);
	ck_assert_int_eq(ret, 1);
	ret = ibwadd("ethnolimit", 0);
	ck_assert_int_eq(ret, 1);

	datacache_status(&dcache);
}
END_TEST

START_TEST(datacache_status_has_no_issues_with_large_number_of_interfaces)
{
	int i, ret;
	char buffer[8];
	datacache *dcache;
	defaultcfg();
	disable_logprints();
	dcache = NULL;

	for (i = 0; i < 100; i++) {
		snprintf(buffer, 8, "eth%d", i);
		ret = datacache_add(&dcache, buffer, 0);
		ck_assert_int_eq(ret, 1);
		ret = ibwadd(buffer, (uint32_t)i);
		ck_assert_int_eq(ret, 1);
	}

	datacache_status(&dcache);
}
END_TEST

void add_daemon_tests(Suite *s)
{
	TCase *tc_daemon = tcase_create("Daemon");
	tcase_add_test(tc_daemon, debugtimestamp_does_not_exit);
	tcase_add_test(tc_daemon, initdstate_does_not_crash);
	tcase_add_test(tc_daemon, addinterfaces_does_nothing_with_no_files);
	tcase_add_test(tc_daemon, addinterfaces_adds_interfaces);
	tcase_add_test(tc_daemon, addinterfaces_adds_only_new_interfaces);
	tcase_add_test(tc_daemon, addinterfaces_adds_to_cache_when_running);
	tcase_add_exit_test(tc_daemon, preparedatabases_exits_with_no_database_dir, 1);
	tcase_add_exit_test(tc_daemon, preparedatabases_exits_with_no_databases, 1);
	tcase_add_exit_test(tc_daemon, preparedatabases_exits_with_no_databases_and_noadd, 1);
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
	tcase_add_test(tc_daemon, processdatacache_empty_does_nothing);
	tcase_add_test(tc_daemon, processdatacache_can_process_things);
	tcase_add_test(tc_daemon, handleintsignals_handles_no_signal);
	tcase_add_test(tc_daemon, handleintsignals_handles_42);
	tcase_add_test(tc_daemon, handleintsignals_handles_unknown_signal);
	tcase_add_test(tc_daemon, handleintsignals_handles_sigterm);
	tcase_add_test(tc_daemon, handleintsignals_handles_sigint);
	tcase_add_test(tc_daemon, handleintsignals_handles_sighup);
	tcase_add_test(tc_daemon, preparedirs_with_no_dir);
	tcase_add_test(tc_daemon, preparedirs_with_dir);
	tcase_add_test(tc_daemon, interfacechangecheck_with_no_interfaces);
	tcase_add_test(tc_daemon, interfacechangecheck_with_empty_cache);
	tcase_add_test(tc_daemon, interfacechangecheck_with_no_changes_in_iflist);
	tcase_add_test(tc_daemon, interfacechangecheck_with_filled_cache);
	tcase_add_test(tc_daemon, simplehash_with_empty_strings);
	tcase_add_test(tc_daemon, simplehash_with_simple_strings);
	tcase_add_test(tc_daemon, initcachevalues_does_not_init_without_database);
	tcase_add_test(tc_daemon, initcachevalues_does_init);
	tcase_add_test(tc_daemon, getcurrenthour_returns_something_realistic);
	tcase_add_test(tc_daemon, waittimesync_does_not_wait_unless_configured_to_do_so);
	tcase_add_test(tc_daemon, waittimesync_does_not_wait_with_no_interfaces);
	tcase_add_test(tc_daemon, waittimesync_does_not_wait_with_new_interfaces);
	tcase_add_test(tc_daemon, waittimesync_knows_when_to_wait);
	tcase_add_test(tc_daemon, waittimesync_knows_when_to_give_up);
	tcase_add_test(tc_daemon, detectboot_sets_btime_if_missing_from_database);
	tcase_add_test(tc_daemon, detectboot_sets_btime_for_new_database);
	tcase_add_test(tc_daemon, detectboot_can_detect_boot);
	tcase_add_exit_test(tc_daemon, handledatabaseerror_exits_on_fatal_error, 1);
	tcase_add_test(tc_daemon, handledatabaseerror_does_not_exit_if_limit_is_not_reached);
	tcase_add_exit_test(tc_daemon, handledatabaseerror_exits_if_limit_is_reached, 1);
	tcase_add_test(tc_daemon, cleanremovedinterfaces_allows_interfaces_to_be_removed);
	tcase_add_test(tc_daemon, processifinfo_syncs_when_needed);
	tcase_add_test(tc_daemon, processifinfo_skips_update_if_timestamps_make_no_sense);
	tcase_add_exit_test(tc_daemon, processifinfo_exits_if_timestamps_really_make_no_sense, 1);
	tcase_add_test(tc_daemon, processifinfo_syncs_if_timestamps_match);
	tcase_add_test(tc_daemon, processifinfo_adds_traffic);
	tcase_add_test(tc_daemon, processifinfo_does_not_add_traffic_when_over_limit);
	tcase_add_test(tc_daemon, processifinfo_adds_zero_traffic_when_over_limit);
	tcase_add_test(tc_daemon, datacache_status_can_handle_nothing);
	tcase_add_test(tc_daemon, datacache_status_can_show_limits);
	tcase_add_test(tc_daemon, datacache_status_has_no_issues_with_large_number_of_interfaces);
	suite_add_tcase(s, tc_daemon);
}
