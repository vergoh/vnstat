#include "vnstat_tests.h"
#include "dbsql_tests.h"
#include "common.h"
#include "dbsql.h"
#include "cfg.h"

START_TEST(db_close_does_no_harm_when_db_is_already_closed)
{
	int ret;

	defaultcfg();

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_open_can_create_database_if_file_does_not_exist)
{
	int ret;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinfo_fails_with_no_open_db)
{
	defaultcfg();

	ck_assert_int_eq(strlen(db_getinfo("foofoo")), 0);
}
END_TEST

START_TEST(db_getinfo_fails_with_nonexisting_name)
{
	int ret;

	defaultcfg();

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

	ck_assert_int_eq(db_setinfo("foo", "bar", 0), 0);
	ck_assert_int_eq(db_setinfo("foo", "bar", 1), 0);
}
END_TEST

START_TEST(db_setinfo_can_set_infos)
{
	int ret;

	defaultcfg();

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

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 0);
}
END_TEST

START_TEST(db_addtraffic_can_add_traffic_and_interfaces)
{
	int ret;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 1);
	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);
	ck_assert_int_eq(db_addtraffic("eth1", 56, 78), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addtraffic_dated_does_not_turn_back_time)
{
	int ret;
	interfaceinfo info;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("eth0", 1, 1);
	ck_assert_int_eq(ret, 1);
	ret = db_getinterfaceinfo("eth0", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 1);
	ck_assert_int_eq(info.txtotal, 1);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);

	ret = db_addtraffic_dated("eth0", 1, 1, 1000);
	ck_assert_int_eq(ret, 1);
	ret = db_getinterfaceinfo("eth0", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 2);
	ck_assert_int_eq(info.txtotal, 2);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);

	ret = db_addtraffic_dated("eth0", 1, 1, 2100000000);
	ck_assert_int_eq(ret, 1);
	ret = db_getinterfaceinfo("eth0", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 3);
	ck_assert_int_eq(info.txtotal, 3);
	ck_assert_int_eq(info.updated, 2100000000);

	ret = db_addtraffic_dated("eth0", 1, 1, 1000);
	ck_assert_int_eq(ret, 1);
	ret = db_getinterfaceinfo("eth0", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 4);
	ck_assert_int_eq(info.txtotal, 4);
	ck_assert_int_eq(info.updated, 2100000000);

	ret = db_addtraffic("eth0", 1, 1);
	ck_assert_int_eq(ret, 1);
	ret = db_getinterfaceinfo("eth0", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 5);
	ck_assert_int_eq(info.txtotal, 5);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setactive_fails_with_no_open_db)
{
	defaultcfg();

	ck_assert_int_eq(db_setactive("eth0", 0), 0);
	ck_assert_int_eq(db_setactive("eth0", 1), 0);
}
END_TEST

START_TEST(db_setactive_fails_if_interface_does_not_exist_in_database)
{
	int ret;

	defaultcfg();

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

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 1);
	ck_assert_int_eq(db_setactive("eth0", 0), 1);
	ck_assert_int_eq(db_setactive("eth0", 1), 1);

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
	ck_assert_int_eq(db_setalias("eth0", "The Internet"), 0);
}
END_TEST

START_TEST(db_setalias_fails_if_interface_does_not_exist_in_database)
{
	int ret;

	defaultcfg();

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

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 1);
	ck_assert_int_eq(db_setalias("eth0", "The Internet"), 1);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);
	ck_assert_int_eq(db_setalias("eth0", "The Internet"), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setupdated_fails_with_no_open_db)
{
	defaultcfg();

	ck_assert_int_eq(db_setupdated("eth0", 123456), 0);
}
END_TEST

START_TEST(db_setupdated_fails_if_interface_does_not_exist_in_database)
{
	int ret;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_setupdated("eth0", 123456), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setupdated_can_change_updated)
{
	int ret;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);
	ck_assert_int_eq(db_setupdated("eth0", 123456), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addinterface_fails_with_no_open_db)
{
	int ret;

	defaultcfg();

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(db_addinterface_can_add_interfaces)
{
	int ret;

	defaultcfg();

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

START_TEST(db_removeinterface_knows_if_interface_exists)
{
	int ret;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = db_removeinterface("eth0");
	ck_assert_int_eq(ret, 0);
	ret = db_removeinterface("nothing");
	ck_assert_int_eq(ret, 0);
	ret = db_removeinterface("");
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_removeinterface_can_remove_interfaces)
{
	int ret;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth2");
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_getinterfacecount(), 3);
	ck_assert_int_eq(db_getinterfacecountbyname("eth0"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eth1"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eth2"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eth3"), 0);

	ret = db_removeinterface("eth1");
	ck_assert_int_eq(ret, 1);
	ret = db_removeinterface("nothing");
	ck_assert_int_eq(ret, 0);
	ret = db_removeinterface("");
	ck_assert_int_eq(ret, 0);

	ck_assert_int_eq(db_getinterfacecount(), 2);
	ck_assert_int_eq(db_getinterfacecountbyname("eth0"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eth1"), 0);
	ck_assert_int_eq(db_getinterfacecountbyname("eth2"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eth3"), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinterfacecount_counts_interfaces)
{
	uint64_t ret;

	defaultcfg();

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

START_TEST(db_interface_info_manipulation)
{
	int ret;
	uint64_t rx, tx, c;
	interfaceinfo info;

	defaultcfg();

	rx = tx = 1;

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth1");
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

	ret = db_settotal("eth1", 42, 24);
	ck_assert_int_eq(ret, 1);

	c = (uint64_t)time(NULL) - 100;
	ret = db_setcreation("eth1", (time_t)c);

	ret = db_getinterfaceinfo("eth0", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.active, 1);
	ck_assert_int_eq(info.rxcounter, 2);
	ck_assert_int_eq(info.txcounter, 2);
	ck_assert_int_eq(info.rxtotal, 0);
	ck_assert_int_eq(info.txtotal, 0);
	ck_assert_int_ne(info.created, 0);

	ck_assert_int_eq(db_setactive("eth1", 0), 1);
	ck_assert_int_eq(db_setupdated("eth1", 0), 1);

	ret = db_getinterfaceinfo("eth1", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.active, 0);
	ck_assert_int_eq(info.rxcounter, 0);
	ck_assert_int_eq(info.txcounter, 0);
	ck_assert_int_eq(info.rxtotal, 42);
	ck_assert_int_eq(info.txtotal, 24);
	ck_assert_int_eq((uint64_t)info.created, c);
	ck_assert_int_eq((uint64_t)info.updated, 0);

	ck_assert_int_eq(db_setupdated("eth1", 123456), 1);

	ret = db_getinterfaceinfo("eth1", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.active, 0);
	ck_assert_int_eq(info.rxcounter, 0);
	ck_assert_int_eq(info.txcounter, 0);
	ck_assert_int_eq(info.rxtotal, 42);
	ck_assert_int_eq(info.txtotal, 24);
	ck_assert_int_eq((uint64_t)info.created, c);
	ck_assert_int_eq((uint64_t)info.updated, 123456);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getiflist_lists_interfaces)
{
	int ret;
	dbiflist *dbifl = NULL, *dbifl_i = NULL;;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("a");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("b");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_getiflist(&dbifl);
	ck_assert_int_eq(ret, 4);

	dbifl_i = dbifl;

	ret = db_close();
	ck_assert_int_eq(ret, 1);

	ck_assert_str_eq(dbifl_i->interface, "a");
	ck_assert_ptr_ne(dbifl_i->next, NULL);
	dbifl_i = dbifl_i->next;

	ck_assert_str_eq(dbifl_i->interface, "b");
	ck_assert_ptr_ne(dbifl_i->next, NULL);
	dbifl_i = dbifl_i->next;

	ck_assert_str_eq(dbifl_i->interface, "eth0");
	ck_assert_ptr_ne(dbifl_i->next, NULL);
	dbifl_i = dbifl_i->next;

	ck_assert_str_eq(dbifl_i->interface, "eth1");
	ck_assert_ptr_eq(dbifl_i->next, NULL);

	dbiflistfree(&dbifl);
	ck_assert_ptr_eq(dbifl, NULL);
}
END_TEST

START_TEST(db_maintenance_does_not_fault)
{
	int ret;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_vacuum();
	ck_assert_int_eq(ret, 1);

	ret = db_removeoldentries();
	ck_assert_int_eq(ret, 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_data_can_be_inserted)
{
	int ret;
	interfaceinfo info;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_insertdata("foo", "eth0", 1, 2, 3);
	ck_assert_int_eq(ret, 0);

	ret = db_insertdata("hour", "eth1", 1, 2, 3);
	ck_assert_int_eq(ret, 0);

	ret = db_insertdata("hour", "eth0", 1, 2, 3);
	ck_assert_int_eq(ret, 1);

	ret = db_insertdata("day", "eth0", 1, 2, 3);
	ck_assert_int_eq(ret, 1);

	ret = db_insertdata("month", "eth0", 1, 2, 3);
	ck_assert_int_eq(ret, 1);

	ret = db_insertdata("year", "eth0", 1, 2, 3);
	ck_assert_int_eq(ret, 1);

	ret = db_insertdata("top", "eth0", 1, 2, 3);
	ck_assert_int_eq(ret, 1);

	/* verify that totals don't get changed */
	ret = db_getinterfaceinfo("eth0", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.active, 1);
	ck_assert_int_eq(info.rxcounter, 0);
	ck_assert_int_eq(info.txcounter, 0);
	ck_assert_int_eq(info.rxtotal, 0);
	ck_assert_int_eq(info.txtotal, 0);
	ck_assert_int_ne(info.created, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_data_can_be_retrieved)
{
	int ret;;
	dbdatalist *datalist = NULL, *datalist_iterator = NULL;
	dbdatalistinfo datainfo;

	defaultcfg();

	ret = db_open(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_insertdata("hour", "eth0", 1, 2, 3);
	ck_assert_int_eq(ret, 1);

	ret = db_insertdata("hour", "eth0", 10, 20, 10000);
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(datainfo.count, 2);
	ck_assert_int_eq(datainfo.minrx, 1);
	ck_assert_int_eq(datainfo.maxrx, 10);
	ck_assert_int_eq(datainfo.mintx, 2);
	ck_assert_int_eq(datainfo.maxtx, 20);
	/* db_insertdata rounds the timestamps to full hours */
	ck_assert_int_eq((int)datainfo.maxtime, 7200);
	ck_assert_int_eq((int)datainfo.mintime, 0);

	datalist_iterator = datalist;

	ck_assert_int_eq(datalist_iterator->rx, 1);
	ck_assert_int_eq(datalist_iterator->tx, 2);
	ck_assert_int_eq(datalist_iterator->timestamp, 0);

	datalist_iterator = datalist_iterator->next;

	ck_assert_int_eq(datalist_iterator->rx, 10);
	ck_assert_int_eq(datalist_iterator->tx, 20);
	ck_assert_int_eq(datalist_iterator->timestamp, 7200);

	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_fatal_errors_get_detected)
{
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_OK), 0);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_FULL), 0);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_IOERR), 0);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_LOCKED), 0);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_BUSY), 0);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_NOTICE), 0);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_WARNING), 0);

	ck_assert_int_eq(db_iserrcodefatal(SQLITE_ERROR), 1);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_ABORT), 1);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_EMPTY), 1);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_READONLY), 1);
}
END_TEST

void add_dbsql_tests(Suite *s)
{
	TCase *tc_dbsql = tcase_create("DB SQL");
	tcase_add_test(tc_dbsql, db_close_does_no_harm_when_db_is_already_closed);
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
	tcase_add_test(tc_dbsql, db_addtraffic_dated_does_not_turn_back_time);
	tcase_add_test(tc_dbsql, db_getinterfacecount_counts_interfaces);
	tcase_add_test(tc_dbsql, db_getinterfacecountbyname_counts_interfaces);
	tcase_add_test(tc_dbsql, db_setactive_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_setactive_fails_if_interface_does_not_exist_in_database);
	tcase_add_test(tc_dbsql, db_setactive_can_change_interface_activity_status);
	tcase_add_test(tc_dbsql, db_setalias_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_setalias_fails_if_interface_does_not_exist_in_database);
	tcase_add_test(tc_dbsql, db_setalias_can_change_interface_alias);
	tcase_add_test(tc_dbsql, db_setupdated_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_setupdated_fails_if_interface_does_not_exist_in_database);
	tcase_add_test(tc_dbsql, db_setupdated_can_change_updated);
	tcase_add_test(tc_dbsql, db_addinterface_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_addinterface_can_add_interfaces);
	tcase_add_test(tc_dbsql, db_addinterface_can_not_add_same_interface_twice);
	tcase_add_test(tc_dbsql, db_removeinterface_knows_if_interface_exists);
	tcase_add_test(tc_dbsql, db_removeinterface_can_remove_interfaces);
	tcase_add_test(tc_dbsql, db_getcounters_with_no_interface);
	tcase_add_test(tc_dbsql, db_setcounters_with_no_interface);
	tcase_add_test(tc_dbsql, db_interface_info_manipulation);
	tcase_add_test(tc_dbsql, db_getiflist_lists_interfaces);
	tcase_add_test(tc_dbsql, db_maintenance_does_not_fault);
	tcase_add_test(tc_dbsql, db_data_can_be_inserted);
	tcase_add_test(tc_dbsql, db_data_can_be_retrieved);
	tcase_add_test(tc_dbsql, db_fatal_errors_get_detected);
	suite_add_tcase(s, tc_dbsql);
}
