#include "vnstat_tests.h"
#include "daemon_tests.h"
#include "common.h"
#include "dbaccess.h"
#include "datacache.h"
#include "dbsql.h"
#include "ifinfo.h"
#include "cfg.h"
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
	linuxonly;

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
	linuxonly;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	fake_proc_net_dev("w", "ethone", 1, 2, 3, 4);
	fake_proc_net_dev("a", "lo0", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethtwo", 5, 6, 7, 8);
	fake_proc_net_dev("a", "sit0", 0, 0, 0, 0);
	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = addinterfaces(&s);
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
	linuxonly;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	fake_proc_net_dev("w", "ethone", 1, 2, 3, 4);
	fake_proc_net_dev("a", "lo0", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethtwo", 5, 6, 7, 8);
	fake_proc_net_dev("a", "sit0", 0, 0, 0, 0);
	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = addinterfaces(&s);
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

	ret = addinterfaces(&s);
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
	linuxonly;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	fake_proc_net_dev("w", "ethone", 1, 2, 3, 4);
	fake_proc_net_dev("a", "ethtwo", 5, 6, 7, 8);
	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(datacache_count(&s.dcache), 0);

	s.running = 1;
	ret = addinterfaces(&s);
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

	ret = addinterfaces(&s);
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

	linuxonly_exit;

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

	linuxonly_exit;

	defaultcfg();
	initdstate(&s);
	suppress_output();
	strncpy_nt(s.dirname, TESTDBDIR, 512);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);

	preparedatabases(&s);
}
END_TEST

START_TEST(preparedatabases_exits_with_no_databases_and_noadd)
{
	DSTATE s;

	linuxonly_exit;

	defaultcfg();
	initdstate(&s);
	s.noadd = 1;
	suppress_output();
	strncpy_nt(s.dirname, TESTDBDIR, 512);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);

	preparedatabases(&s);
}
END_TEST

START_TEST(preparedatabases_with_no_databases_creates_databases)
{
	int ret;
	DSTATE s;

	linuxonly;

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
	ret = db_open(1);
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
	strncpy_nt(s.dirname, TESTDBDIR, 512);
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
	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	filldatabaselist(&s);

	ck_assert_int_eq(s.dbcount, 0);
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
	ret = db_open(1);
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
	ck_assert_int_eq(s.dbcount, 2);
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

START_TEST(processdatacache_empty_does_nothing)
{
	DSTATE s;
	initdstate(&s);

	processdatacache(&s);
}
END_TEST

START_TEST(handleintsignals_handles_signals)
{
	DSTATE s;
	defaultcfg();
	initdstate(&s);
	s.running = 1;
	s.dbcount = 1;

	intsignal = 0;
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 1);
	ck_assert_int_eq(s.dbcount, 1);

	intsignal = 42;
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 1);
	ck_assert_int_eq(s.dbcount, 1);

	disable_logprints();

	intsignal = 43;
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 1);
	ck_assert_int_eq(s.dbcount, 1);

	intsignal = SIGTERM;
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 0);
	ck_assert_int_eq(s.dbcount, 1);

	s.running = 1;
	intsignal = SIGINT;
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 0);
	ck_assert_int_eq(s.dbcount, 1);

	s.running = 1;
	intsignal = SIGHUP;
	strncpy_nt(s.dirname, TESTDBDIR, 512);
	handleintsignals(&s);
	ck_assert_int_eq(intsignal, 0);
	ck_assert_int_eq(s.running, 1);
	ck_assert_int_eq(s.dbcount, 0);
}
END_TEST

START_TEST(preparedirs_with_no_dir)
{
	char logdir[512], piddir[512];

	DSTATE s;
	initdstate(&s);
	defaultcfg();
	cfg.uselogging = 1;
	s.rundaemon = 1;
	strncpy_nt(s.dirname, TESTDBDIR, 512);
	snprintf(logdir, 512, "%s/log/vnstat", TESTDIR);
	snprintf(piddir, 512, "%s/pid/vnstat", TESTDIR);
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
	char logdir[512], piddir[512];

	DSTATE s;
	initdstate(&s);
	defaultcfg();
	cfg.uselogging = 1;
	s.rundaemon = 1;
	strncpy_nt(s.dirname, TESTDBDIR, 512);
	snprintf(logdir, 512, "%s/log/vnstat", TESTDIR);
	snprintf(piddir, 512, "%s/pid/vnstat", TESTDIR);
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

	linuxonly;

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

	linuxonly;

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

	linuxonly;

	initdstate(&s);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethsomething", 1, 2, 3, 4);
	fake_proc_net_dev("a", "ethelse", 5, 6, 7, 8);
	ck_assert_int_ne(getiflist(&ifacelist, 0), 0);
	ifhash = simplehash(ifacelist, (int)strlen(ifacelist));
	s.iflisthash = ifhash;

	interfacechangecheck(&s);
	ck_assert_int_eq(s.iflisthash, ifhash);
	ck_assert_int_eq(s.forcesave, 0);
}
END_TEST

START_TEST(interfacechangecheck_with_filled_cache)
{
	linuxonly;

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
	ret = db_open(1);
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
	tcase_add_test(tc_daemon, handleintsignals_handles_signals);
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
	suite_add_tcase(s, tc_daemon);
}
