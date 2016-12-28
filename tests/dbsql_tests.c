#include "vnstat_tests.h"
#include "dbsql_tests.h"
#include "common.h"
#include "dbsql.h"
#include "cfg.h"

START_TEST(db_close_does_no_harm_when_db_is_already_closed)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_open_can_not_open_nonexisting_file)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(0);
	ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(db_open_can_create_database_if_file_does_not_exist)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinfo_fails_with_no_open_db)
{
	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ck_assert_int_eq(strlen(db_getinfo("foofoo")), 0);
}
END_TEST

START_TEST(db_getinfo_fails_with_nonexisting_name)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_str_eq(db_getinfo("broken_name"), "");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinfo_can_get_dbversion)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_str_eq(db_getinfo("dbversion"), SQLDBVERSION);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setinfo_fails_with_no_open_db)
{
	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ck_assert_int_eq(db_setinfo("foo", "bar", 0), 0);
	ck_assert_int_eq(db_setinfo("foo", "bar", 1), 0);
}
END_TEST

START_TEST(db_setinfo_can_set_infos)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_setinfo("foo", "bar", 1), 1);
	ck_assert_str_eq(db_getinfo("foo"), "bar");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setinfo_can_update_infos)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_setinfo("foo", "bar", 1), 1);
	ck_assert_str_eq(db_getinfo("foo"), "bar");

	ck_assert_int_eq(db_setinfo("foo", "qux", 0), 1);
	ck_assert_str_eq(db_getinfo("foo"), "qux");

	ck_assert_int_eq(db_setinfo("foo", "quux", 1), 1);
	ck_assert_str_eq(db_getinfo("foo"), "quux");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setinfo_can_not_update_nonexisting_name)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_setinfo("foo", "bar", 1), 1);
	ck_assert_str_eq(db_getinfo("foo"), "bar");

	ck_assert_int_eq(db_setinfo("bar", "qux", 0), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addtraffic_with_no_traffic_does_nothing)
{
	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 1);
}
END_TEST

START_TEST(db_addtraffic_can_add_traffic_and_interfaces)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 1);
	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);
	ck_assert_int_eq(db_addtraffic("eth1", 56, 78), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setactive_fails_with_no_open_db)
{
	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ck_assert_int_eq(db_setactive("eth0", 0), 0);
	ck_assert_int_eq(db_setactive("eth0", 1), 0);
}
END_TEST

START_TEST(db_setactive_fails_if_interface_does_not_exist_in_database)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_setactive("eth0", 0), 0);
	ck_assert_int_eq(db_setactive("eth0", 1), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setactive_can_change_interface_activity_status)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 1);
	ck_assert_int_eq(db_setactive("eth0", 0), 0);
	ck_assert_int_eq(db_setactive("eth0", 1), 0);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);
	ck_assert_int_eq(db_setactive("eth0", 0), 1);
	ck_assert_int_eq(db_setactive("eth0", 1), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setalias_fails_with_no_open_db)
{
	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ck_assert_int_eq(db_setalias("eth0", "The Internet"), 0);
}
END_TEST

START_TEST(db_setalias_fails_if_interface_does_not_exist_in_database)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_setalias("eth0", "The Internet"), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setalias_can_change_interface_alias)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 1);
	ck_assert_int_eq(db_setalias("eth0", "The Internet"), 0);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);
	ck_assert_int_eq(db_setalias("eth0", "The Internet"), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addinterface_fails_with_no_open_db)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(db_addinterface_can_add_interfaces)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addinterface_can_not_add_same_interface_twice)
{
	int ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 0);
	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinterfacecount_counts_interfaces)
{
	uint64_t ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfacecount();
	ck_assert_int_eq(ret, 0);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfacecount();
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 0);

	ret = db_getinterfacecount();
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfacecount();
	ck_assert_int_eq(ret, 2);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinterfacecountbyname_counts_interfaces)
{
	uint64_t ret;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfacecountbyname("foo");
	ck_assert_int_eq(ret, 0);

	ret = db_getinterfacecountbyname("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfacecountbyname("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfacecountbyname("eth2");
	ck_assert_int_eq(ret, 0);

	ret = db_getinterfacecountbyname("");
	ck_assert_int_eq(ret, 2);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getcounters_with_no_interface)
{
	int ret;
	uint64_t rx, tx;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	rx = tx = 1;

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = db_getcounters("eth0", &rx, &tx);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(rx, 0);
	ck_assert_int_eq(tx, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setcounters_with_no_interface)
{
	int ret;
	uint64_t rx, tx;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	rx = tx = 1;

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = db_setcounters("eth0", 2, 2);
	ck_assert_int_eq(ret, 0);

	ret = db_getcounters("eth0", &rx, &tx);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(rx, 0);
	ck_assert_int_eq(tx, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_counters_with_interface)
{
	int ret;
	uint64_t rx, tx;

	defaultcfg();
	strncpy_nt(cfg.dbdir, TESTDBDIR, 512);
	ck_assert_int_eq(clean_testdbdir(), 1);

	rx = tx = 1;

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_getcounters("eth0", &rx, &tx);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(rx, 0);
	ck_assert_int_eq(tx, 0);

	ret = db_setcounters("eth0", 2, 2);
	ck_assert_int_eq(ret, 1);

	ret = db_getcounters("eth0", &rx, &tx);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(rx, 2);
	ck_assert_int_eq(tx, 2);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

void add_dbsql_tests(Suite *s)
{
	TCase *tc_dbsql = tcase_create("DB SQL");
	tcase_add_test(tc_dbsql, db_close_does_no_harm_when_db_is_already_closed);
	tcase_add_test(tc_dbsql, db_open_can_not_open_nonexisting_file);
	tcase_add_test(tc_dbsql, db_open_can_create_database_if_file_does_not_exist);
	tcase_add_test(tc_dbsql, db_getinfo_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_getinfo_fails_with_nonexisting_name);
	tcase_add_test(tc_dbsql, db_getinfo_can_get_dbversion);
	tcase_add_test(tc_dbsql, db_setinfo_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_setinfo_can_set_infos);
	tcase_add_test(tc_dbsql, db_setinfo_can_update_infos);
	tcase_add_test(tc_dbsql, db_setinfo_can_not_update_nonexisting_name);
	tcase_add_test(tc_dbsql, db_addtraffic_with_no_traffic_does_nothing);
	tcase_add_test(tc_dbsql, db_addtraffic_can_add_traffic_and_interfaces);
	tcase_add_test(tc_dbsql, db_getinterfacecount_counts_interfaces);
	tcase_add_test(tc_dbsql, db_getinterfacecountbyname_counts_interfaces);
	tcase_add_test(tc_dbsql, db_setactive_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_setactive_fails_if_interface_does_not_exist_in_database);
	tcase_add_test(tc_dbsql, db_setactive_can_change_interface_activity_status);
	tcase_add_test(tc_dbsql, db_setalias_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_setalias_fails_if_interface_does_not_exist_in_database);
	tcase_add_test(tc_dbsql, db_setalias_can_change_interface_alias);
	tcase_add_test(tc_dbsql, db_addinterface_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_addinterface_can_add_interfaces);
	tcase_add_test(tc_dbsql, db_addinterface_can_not_add_same_interface_twice);
	tcase_add_test(tc_dbsql, db_getcounters_with_no_interface);
	tcase_add_test(tc_dbsql, db_setcounters_with_no_interface);
	tcase_add_test(tc_dbsql, db_counters_with_interface);
	suite_add_tcase(s, tc_dbsql);
}
