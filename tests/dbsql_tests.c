#include "common.h"
#include "vnstat_tests.h"
#include "dbsql_tests.h"
#include "dbsql.h"
#include "misc.h"
#include "cfg.h"

START_TEST(db_close_does_no_harm_when_db_is_already_closed)
{
	int ret;

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_open_rw_can_create_database_if_file_does_not_exist)
{
	int ret;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_open_ro_cannot_create_a_database)
{
	int ret;

	suppress_output();

	ret = db_open_ro();
	ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(db_getinfo_fails_with_no_open_db)
{
	suppress_output();

	ck_assert_int_eq(strlen(db_getinfo("foofoo")), 0);
}
END_TEST

START_TEST(db_getinfo_fails_with_nonexisting_name)
{
	int ret;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_str_eq(db_getinfo("broken_name"), "");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinfo_can_get_dbversion)
{
	int ret;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_str_eq(db_getinfo("dbversion"), SQLDBVERSION);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setinfo_fails_with_no_open_db)
{
	suppress_output();

	ck_assert_int_eq(db_setinfo("foo", "bar", 0), 0);
	ck_assert_int_eq(db_setinfo("foo", "bar", 1), 0);
}
END_TEST

START_TEST(db_setinfo_can_set_infos)
{
	int ret;

	ret = db_open_rw(1);
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

	ret = db_open_rw(1);
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

	ret = db_open_rw(1);
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
	suppress_output();

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 0);
}
END_TEST

START_TEST(db_addtraffic_can_add_traffic_and_interfaces)
{
	int ret;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 1);
	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);
	ck_assert_int_eq(db_addtraffic("eth1", 56, 78), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addtraffic_can_add_traffic_and_interfaces_utc)
{
	int ret;

	cfg.useutc = 1;
	validatecfg(CT_Daemon);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 0, 0), 1);
	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);
	ck_assert_int_eq(db_addtraffic("eth1", 56, 78), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addtraffic_adds_traffic_to_different_data_types)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ck_assert_int_ne(cfg.fiveminutehours, 0);
	ck_assert_int_ne(cfg.hourlydays, 0);
	ck_assert_int_ne(cfg.dailydays, 0);
	ck_assert_int_ne(cfg.monthlymonths, 0);
	ck_assert_int_ne(cfg.yearlyyears, 0);
	ck_assert_int_ne(cfg.topdayentries, 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 123, 456), 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "fiveminute", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "day", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "month", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "year", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "top", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addtraffic_does_not_add_traffic_to_disabled_data_types)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	cfg.fiveminutehours = 0;
	cfg.hourlydays = 0;
	cfg.dailydays = 0;
	cfg.monthlymonths = 0;
	cfg.yearlyyears = 0;
	cfg.topdayentries = 0;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_addtraffic("eth0", 1, 1), 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "fiveminute", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "day", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "month", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "year", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "top", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addtraffic_dated_does_not_touch_updated_time)
{
	int ret;
	interfaceinfo info;

	ret = db_open_rw(1);
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
	ck_assert_int_lt(info.updated, 2100000000);

	ret = db_addtraffic_dated("eth0", 1, 1, 1000);
	ck_assert_int_eq(ret, 1);
	ret = db_getinterfaceinfo("eth0", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 4);
	ck_assert_int_eq(info.txtotal, 4);
	ck_assert_int_lt(info.updated, 2100000000);

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
	suppress_output();

	ck_assert_int_eq(db_setactive("eth0", 0), 0);
	ck_assert_int_eq(db_setactive("eth0", 1), 0);
}
END_TEST

START_TEST(db_setactive_fails_if_interface_does_not_exist_in_database)
{
	int ret;

	ret = db_open_rw(1);
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

	ret = db_open_rw(1);
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
	suppress_output();

	ck_assert_int_eq(db_setalias("eth0", "The Internet"), 0);
}
END_TEST

START_TEST(db_setalias_fails_if_interface_does_not_exist_in_database)
{
	int ret;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_setalias("eth0", "The Internet"), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setalias_can_change_interface_alias)
{
	int ret;

	ret = db_open_rw(1);
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
	suppress_output();

	ck_assert_int_eq(db_setupdated("eth0", 123456), 0);
}
END_TEST

START_TEST(db_setupdated_fails_if_interface_does_not_exist_in_database)
{
	int ret;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_setupdated("eth0", 123456), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setupdated_can_change_updated)
{
	int ret;

	ret = db_open_rw(1);
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

	suppress_output();

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(db_addinterface_can_add_interfaces)
{
	int ret;

	ret = db_open_rw(1);
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

	suppress_output();

	ret = db_open_rw(1);
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

	ret = db_open_rw(1);
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

	ret = db_open_rw(1);
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

START_TEST(db_renameinterface_knows_if_interface_exists)
{
	int ret;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_renameinterface("eth0", "eth1");
	ck_assert_int_eq(ret, 0);
	ret = db_renameinterface("nothing", "something");
	ck_assert_int_eq(ret, 0);
	ret = db_renameinterface("", "");
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_renameinterface_can_rename_interfaces)
{
	int ret;

	ret = db_open_rw(1);
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

	suppress_output();

	ret = db_renameinterface("eth0", "eth1");
	ck_assert_int_eq(ret, 0);
	ret = db_renameinterface("eth2", "eth3");
	ck_assert_int_eq(ret, 1);
	ret = db_renameinterface("eth1", "eth2");
	ck_assert_int_eq(ret, 1);
	ret = db_renameinterface("eth0", "");
	ck_assert_int_eq(ret, 0);
	ret = db_renameinterface("eth0", "eth1");
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(db_getinterfacecount(), 3);
	ck_assert_int_eq(db_getinterfacecountbyname("eth0"), 0);
	ck_assert_int_eq(db_getinterfacecountbyname("eth1"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eth2"), 1);
	ck_assert_int_eq(db_getinterfacecountbyname("eth3"), 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinterfacecount_counts_interfaces)
{
	int ret;

	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecount();
	ck_assert_int_eq(ret, 0);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecount();
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 0);

	ret = (int)db_getinterfacecount();
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecount();
	ck_assert_int_eq(ret, 2);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinterfacecountbyname_counts_interfaces)
{
	int ret;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth3");
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecountbyname("foo");
	ck_assert_int_eq(ret, 0);

	ret = (int)db_getinterfacecountbyname("eth0");
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecountbyname("eth1");
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecountbyname("eth2");
	ck_assert_int_eq(ret, 0);

	ret = (int)db_getinterfacecountbyname("");
	ck_assert_int_eq(ret, 3);

	ret = (int)db_getinterfacecountbyname("eth0+eth1");
	ck_assert_int_eq(ret, 2);

	ret = (int)db_getinterfacecountbyname("eth0+eth1+eth3");
	ck_assert_int_eq(ret, 3);

	ret = (int)db_getinterfacecountbyname("eth0+eth1+eth2");
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getcounters_with_no_interface)
{
	int ret;
	uint64_t rx, tx;

	rx = tx = 1;

	ret = db_open_rw(1);
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

	rx = tx = 1;

	ret = db_open_rw(1);
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

	rx = tx = 1;

	ret = db_open_rw(1);
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
	iflist *dbifl = NULL, *dbifl_i = NULL;

	ret = db_open_rw(1);
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

	iflistfree(&dbifl);
	ck_assert_ptr_eq(dbifl, NULL);
}
END_TEST

START_TEST(db_maintenance_does_not_fault)
{
	int ret;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_vacuum();
	ck_assert_int_eq(ret, 1);

	ret = db_removeoldentries();
	ck_assert_int_eq(ret, 1);

	ret = db_removedisabledresolutionentries();
	ck_assert_int_eq(ret, 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_data_can_be_inserted)
{
	int ret;
	interfaceinfo info;

	ret = db_open_rw(1);
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
	int ret;
	dbdatalist *datalist = NULL, *datalist_iterator = NULL;
	dbdatalistinfo datainfo;

	ret = db_open_rw(1);
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
	ck_assert_int_eq(datainfo.sumrx, 11);
	ck_assert_int_eq(datainfo.mintx, 2);
	ck_assert_int_eq(datainfo.maxtx, 20);
	ck_assert_int_eq(datainfo.sumtx, 22);
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

START_TEST(db_data_can_be_inserted_utc)
{
	int ret;
	interfaceinfo info;

	cfg.useutc = 1;
	validatecfg(CT_Daemon);

	ret = db_open_rw(1);
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

START_TEST(db_data_can_be_retrieved_utc)
{
	int ret;
	dbdatalist *datalist = NULL, *datalist_iterator = NULL;
	dbdatalistinfo datainfo;

	cfg.useutc = 1;
	validatecfg(CT_Daemon);

	ret = db_open_rw(1);
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
	ck_assert_int_eq(datainfo.sumrx, 11);
	ck_assert_int_eq(datainfo.mintx, 2);
	ck_assert_int_eq(datainfo.maxtx, 20);
	ck_assert_int_eq(datainfo.sumtx, 22);
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

	ck_assert_int_eq(db_iserrcodefatal(SQLITE_ERROR), 1);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_ABORT), 1);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_EMPTY), 1);
	ck_assert_int_eq(db_iserrcodefatal(SQLITE_READONLY), 1);
}
END_TEST

START_TEST(db_validate_with_valid_version)
{
	int ret;
	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_validate(0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(db_errcode, 0);

	ret = db_validate(1);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(db_errcode, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_validate_with_no_version)
{
	int ret;
	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_exec("delete from info where name='dbversion';");
	ck_assert_int_eq(ret, 1);

	ret = db_validate(0);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(db_errcode, 0);

	ret = db_validate(1);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(db_errcode, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_validate_with_low_version)
{
	int ret;
	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_exec("update info set value='-1' where name='dbversion';");
	ck_assert_int_eq(ret, 1);

	ret = db_validate(0);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(db_errcode, 0);

	ret = db_validate(1);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(db_errcode, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_validate_with_high_version)
{
	int ret;
	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_exec("update info set value='100' where name='dbversion';");
	ck_assert_int_eq(ret, 1);

	ret = db_validate(0);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(db_errcode, 0);

	ret = db_validate(1);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(db_errcode, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

void range_test_month_setup(void)
{
	int ret, i;
	cfg.monthrotate = 2; /* this should have no effect */

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethtest");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i <= 6; i++) {
		ret = db_insertdata("month", "ethtest", 1, 2, get_timestamp(2000, i, 1, 0, 0));
		ck_assert_int_eq(ret, 1);
	}
}

START_TEST(db_getdata_range_can_get_months_without_range_defined)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_month_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 6);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-01-01");
				break;
			case 5:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-06-01");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_months_with_range_matching_existing_data)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_month_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "2000-01-01", "2000-06-01");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 6);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-01-01");
				break;
			case 5:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-06-01");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_months_with_range_past_existing_data)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_month_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "1999-01-01", "2000-08-01");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 6);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-01-01");
				break;
			case 5:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-06-01");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_months_with_range_limiting_begin_and_end)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_month_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "2000-02-01", "2000-04-01");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 3);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-02-01");
				break;
			case 2:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-04-01");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_months_with_range_limiting_begin)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_month_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "2000-03-01", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 4);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-03-01");
				break;
			case 3:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-06-01");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_months_with_range_limiting_begin_with_limit)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_month_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 3, "2000-03-01", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 3);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-03-01");
				break;
			case 2:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-05-01");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_months_with_range_limiting_end)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_month_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "", "2000-04-01");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 4);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-01-01");
				break;
			case 3:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-04-01");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_months_with_range_limiting_end_with_limit)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_month_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 3, "", "2000-04-01");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 3);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-02-01");
				break;
			case 2:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-04-01");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_months_with_range_on_same_month)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_month_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "2000-04-01", "2000-04-01");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 1);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-04-01");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_months_with_range_past_first_day_of_month)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_month_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 3, "2000-02-02", "2000-05-06");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 3);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-03-01");
				break;
			case 2:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-05-01");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

void range_test_hour_setup(void)
{
	int ret, i, j;
	cfg.monthrotate = 2; /* this should have no effect */

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethtest");
	ck_assert_int_eq(ret, 1);

	for (j = 2; j < 5; j++) {
		for (i = 0; i < 24; i++) {
			ret = db_insertdata("hour", "ethtest", 1, 2, get_timestamp(2002, 2, j, i, 0));
			ck_assert_int_eq(ret, 1);
		}
	}
}

START_TEST(db_getdata_range_can_get_hours_without_range_defined)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_hour_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "hour", 0, "", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 72);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 00:00");
				break;
			case 71:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-04 23:00");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_hours_with_range_matching_existing_data)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_hour_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "hour", 0, "2002-02-02 00:00", "2002-02-04 23:00");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 72);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 00:00");
				break;
			case 71:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-04 23:00");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_hours_with_range_past_existing_data)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_hour_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "hour", 0, "2002-01-01 15:00", "2002-02-04 23:30");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 72);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 00:00");
				break;
			case 71:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-04 23:00");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_hours_with_range_limiting_begin_and_end)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_hour_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "hour", 0, "2002-02-02 20:00", "2002-02-03 17:00");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 22);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 20:00");
				break;
			case 21:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-03 17:00");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_hours_with_range_limiting_begin)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_hour_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "hour", 0, "2002-02-02 20:00", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 52);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 20:00");
				break;
			case 51:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-04 23:00");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_hours_with_range_limiting_begin_with_limit)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_hour_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "hour", 11, "2002-02-02 20:00", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 11);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 20:00");
				break;
			case 10:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-03 06:00");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_hours_with_range_limiting_end)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_hour_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "hour", 0, "", "2002-02-02 20:00");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 21);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 00:00");
				break;
			case 20:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 20:00");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_hours_with_range_limiting_end_with_limit)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_hour_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "hour", 5, "", "2002-02-02 20:00");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 5);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 16:00");
				break;
			case 4:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 20:00");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_can_get_hours_with_range_on_same_hour)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	range_test_hour_setup();

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "hour", 0, "2002-02-02 20:00", "2002-02-02 20:00");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 1);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d %H:%M", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2002-02-02 20:00");
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "hour", 0, "2002-02-02 20:15", "2002-02-02 20:45");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_with_merged_interfaces)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	cfg.monthrotate = 1;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethtest");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethother");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i <= 20; i++) {
		ret = db_addtraffic_dated("ethtest", 1, 2, get_timestamp(2000, 2, i, 0, 0));
		ck_assert_int_eq(ret, 1);
		ret = db_addtraffic_dated("ethother", 3, 7, get_timestamp(2000, 2, i, 0, 0));
		ck_assert_int_eq(ret, 1);
	}

	ret = db_getdata_range(&datalist, &datainfo, "ethtest+ethother", "month", 0, "", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 1);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-02-01");
				ck_assert_int_eq(datalist_i->rx, 80);
				ck_assert_int_eq(datalist_i->tx, 180);
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getdata_range_with_long_merged_interfaces)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	cfg.monthrotate = 1;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethtester");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethotherthing");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethreallyfast");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethturtle");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i <= 20; i++) {
		ret = db_addtraffic_dated("ethtester", 1, 2, get_timestamp(2010, 9, i, 0, 0));
		ck_assert_int_eq(ret, 1);
		ret = db_addtraffic_dated("ethotherthing", 3, 7, get_timestamp(2010, 9, i, 0, 0));
		ck_assert_int_eq(ret, 1);
		ret = db_addtraffic_dated("ethreallyfast", 4, 8, get_timestamp(2010, 9, i, 0, 0));
		ck_assert_int_eq(ret, 1);
		ret = db_addtraffic_dated("ethturtle", 5, 9, get_timestamp(2010, 9, i, 0, 0));
		ck_assert_int_eq(ret, 1);
	}

	ret = db_getdata_range(&datalist, &datainfo, "ethtester+ethotherthing+ethreallyfast+ethturtle", "month", 0, "", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 1);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2010-09-01");
				ck_assert_int_eq(datalist_i->rx, 260);
				ck_assert_int_eq(datalist_i->tx, 520);
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_getdata_range(&datalist, &datainfo, "ethotherthing+ethreallyfast+ethturtle", "month", 0, "", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 1);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2010-09-01");
				ck_assert_int_eq(datalist_i->rx, 240);
				ck_assert_int_eq(datalist_i->tx, 480);
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addtraffic_without_monthrotate)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	cfg.monthrotate = 1;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethtest");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i <= 20; i++) {
		ret = db_addtraffic_dated("ethtest", 1, 2, get_timestamp(2000, 2, i, 0, 0));
		ck_assert_int_eq(ret, 1);
	}

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 1);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-02-01");
				ck_assert_int_eq(datalist_i->rx, 20);
				ck_assert_int_eq(datalist_i->tx, 40);
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addtraffic_with_monthrotate)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	cfg.monthrotate = 7;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethtest");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i <= 20; i++) {
		ret = db_addtraffic_dated("ethtest", 1, 2, get_timestamp(2000, 2, i, 0, 0));
		ck_assert_int_eq(ret, 1);
	}

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 2);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-01-01");
				ck_assert_int_eq(datalist_i->rx, 6);
				ck_assert_int_eq(datalist_i->tx, 12);
				break;
			case 1:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-02-01");
				ck_assert_int_eq(datalist_i->rx, 14);
				ck_assert_int_eq(datalist_i->tx, 28);
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addtraffic_without_monthrotate_utc)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	cfg.monthrotate = 1;
	cfg.useutc = 1;
	validatecfg(CT_Daemon);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethtest");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i <= 20; i++) {
		ret = db_addtraffic_dated("ethtest", 1, 2, get_timestamp(2000, 2, i, 0, 0));
		ck_assert_int_eq(ret, 1);
	}

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 1);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-02-01");
				ck_assert_int_eq(datalist_i->rx, 20);
				ck_assert_int_eq(datalist_i->tx, 40);
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_addtraffic_with_monthrotate_utc)
{
	int ret, i;
	char timestamp[64];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	cfg.monthrotate = 7;
	cfg.useutc = 1;
	validatecfg(CT_Daemon);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethtest");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i <= 20; i++) {
		ret = db_addtraffic_dated("ethtest", 1, 2, get_timestamp(2000, 2, i, 0, 0));
		ck_assert_int_eq(ret, 1);
	}

	ret = db_getdata_range(&datalist, &datainfo, "ethtest", "month", 0, "", "");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 2);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		switch (i) {
			case 0:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-01-01");
				ck_assert_int_eq(datalist_i->rx, 6);
				ck_assert_int_eq(datalist_i->tx, 12);
				break;
			case 1:
				strftime(timestamp, 64, "%Y-%m-%d", localtime(&datalist_i->timestamp));
				ck_assert_str_eq(timestamp, "2000-02-01");
				ck_assert_int_eq(datalist_i->rx, 14);
				ck_assert_int_eq(datalist_i->tx, 28);
				break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_get_date_generator_can_generate_dates)
{

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 0, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 0, "foo"), "strftime('%Y-%m-%d %H:00:00', foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 0, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 0, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 0, "foo"), "strftime('%Y-%m-01', foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 0, "foo"), "strftime('%Y-01-01', foo, 'localtime')"), NULL);

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 1, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 1, "foo"), "strftime('%Y-%m-%d %H:00:00', foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 1, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 1, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 1, "foo"), "strftime('%Y-%m-01', foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 1, "foo"), "strftime('%Y-01-01', foo, 'localtime')"), NULL);
}
END_TEST

START_TEST(db_get_date_generator_can_generate_dates_with_monthrotate)
{

	cfg.monthrotate = 10;
	cfg.monthrotateyears = 0;

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 0, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 0, "foo"), "strftime('%Y-%m-%d %H:00:00', foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 0, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 0, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 0, "foo"), "strftime('%Y-%m-01', datetime(foo, '-9 days'), 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 0, "foo"), "strftime('%Y-01-01', foo, 'localtime')"), NULL);

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 1, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 1, "foo"), "strftime('%Y-%m-%d %H:00:00', foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 1, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 1, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 1, "foo"), "strftime('%Y-%m-01', foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 1, "foo"), "strftime('%Y-01-01', foo, 'localtime')"), NULL);

	cfg.monthrotate = 8;
	cfg.monthrotateyears = 1;

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 0, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 0, "foo"), "strftime('%Y-%m-%d %H:00:00', foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 0, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 0, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 0, "foo"), "strftime('%Y-%m-01', datetime(foo, '-7 days'), 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 0, "foo"), "strftime('%Y-01-01', datetime(foo, '-7 days'), 'localtime')"), NULL);

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 1, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 1, "foo"), "strftime('%Y-%m-%d %H:00:00', foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 1, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 1, "foo"), "date(foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 1, "foo"), "strftime('%Y-%m-01', foo, 'localtime')"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 1, "foo"), "strftime('%Y-01-01', foo, 'localtime')"), NULL);
}
END_TEST

START_TEST(db_get_date_generator_can_generate_dates_utc)
{
	cfg.useutc = 1;
	validatecfg(CT_Daemon);

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 0, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 0, "foo"), "strftime('%Y-%m-%d %H:00:00', foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 0, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 0, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 0, "foo"), "strftime('%Y-%m-01', foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 0, "foo"), "strftime('%Y-01-01', foo)"), NULL);

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 1, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 1, "foo"), "strftime('%Y-%m-%d %H:00:00', foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 1, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 1, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 1, "foo"), "strftime('%Y-%m-01', foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 1, "foo"), "strftime('%Y-01-01', foo)"), NULL);
}
END_TEST

START_TEST(db_get_date_generator_can_generate_dates_with_monthrotate_utc)
{
	cfg.useutc = 1;
	validatecfg(CT_Daemon);

	cfg.monthrotate = 10;
	cfg.monthrotateyears = 0;

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 0, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 0, "foo"), "strftime('%Y-%m-%d %H:00:00', foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 0, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 0, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 0, "foo"), "strftime('%Y-%m-01', datetime(foo, '-9 days'))"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 0, "foo"), "strftime('%Y-01-01', foo)"), NULL);

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 1, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 1, "foo"), "strftime('%Y-%m-%d %H:00:00', foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 1, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 1, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 1, "foo"), "strftime('%Y-%m-01', foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 1, "foo"), "strftime('%Y-01-01', foo)"), NULL);

	cfg.monthrotate = 8;
	cfg.monthrotateyears = 1;

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 0, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 0, "foo"), "strftime('%Y-%m-%d %H:00:00', foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 0, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 0, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 0, "foo"), "strftime('%Y-%m-01', datetime(foo, '-7 days'))"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 0, "foo"), "strftime('%Y-01-01', datetime(foo, '-7 days'))"), NULL);

	ck_assert_ptr_ne(strstr(db_get_date_generator(0, 1, "foo"), "minutes"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(1, 1, "foo"), "strftime('%Y-%m-%d %H:00:00', foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(2, 1, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(5, 1, "foo"), "date(foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(3, 1, "foo"), "strftime('%Y-%m-01', foo)"), NULL);
	ck_assert_ptr_ne(strstr(db_get_date_generator(4, 1, "foo"), "strftime('%Y-01-01', foo)"), NULL);
}
END_TEST

START_TEST(getifaceinquery_does_not_mess_regular_interfaces)
{
	char *result;

	result = getifaceinquery("eth0");
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, "'eth0'");
	free(result);

	/* this isn't a realistic scenario but doesn't hurt to have a test */
	result = getifaceinquery("eth0 with space");
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, "'eth0 with space'");
	free(result);

	result = getifaceinquery("em1_em2");
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, "'em1_em2'");
	free(result);

	result = getifaceinquery("em1-em2");
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, "'em1-em2'");
	free(result);
}
END_TEST

START_TEST(getifaceinquery_can_create_merge_queries)
{
	char *result;

	result = getifaceinquery("eth0+eth1");
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, "'eth0','eth1'");
	free(result);

	result = getifaceinquery("eth1+eth0");
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, "'eth1','eth0'");
	free(result);

	result = getifaceinquery("eth0+em1+eth1");
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, "'eth0','em1','eth1'");
	free(result);
}
END_TEST

START_TEST(getifaceinquery_can_create_merge_query_longer_than_single_interface_maxlen)
{
	char *result;

	result = getifaceinquery("eth0+em1+eth1+eth2+em2+eth3+eth4+em3+eth5");
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, "'eth0','em1','eth1','eth2','em2','eth3','eth4','em3','eth5'");
	free(result);
}
END_TEST

START_TEST(getifaceinquery_does_not_tolerate_nonsense)
{
	char *result;

	result = getifaceinquery("");
	ck_assert_ptr_eq(result, NULL);

	result = getifaceinquery("+eth0");
	ck_assert_ptr_eq(result, NULL);

	result = getifaceinquery("eth0+");
	ck_assert_ptr_eq(result, NULL);

	result = getifaceinquery("+eth0+");
	ck_assert_ptr_eq(result, NULL);

	result = getifaceinquery("eth0++eth1");
	ck_assert_ptr_eq(result, NULL);

	result = getifaceinquery("eth0+++eth1");
	ck_assert_ptr_eq(result, NULL);

	result = getifaceinquery("eth0+eth1++eth2");
	ck_assert_ptr_eq(result, NULL);

	result = getifaceinquery("+++ATH0");
	ck_assert_ptr_eq(result, NULL);
}
END_TEST

START_TEST(db_getinterfaceid_can_get_ids)
{
	int ret;


	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth3");
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecount();
	ck_assert_int_eq(ret, 3);

	ret = (int)db_getinterfaceid("eth0", 0);
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfaceid("eth0", 1);
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfaceid("eth1", 0);
	ck_assert_int_eq(ret, 2);

	ret = (int)db_getinterfaceid("eth2", 0);
	ck_assert_int_eq(ret, 0);

	ret = (int)db_getinterfaceid("eth3", 0);
	ck_assert_int_eq(ret, 3);

	ret = (int)db_getinterfaceid("eth2", 1);
	ck_assert_int_eq(ret, 4);

	ret = (int)db_getinterfaceid("eth2", 0);
	ck_assert_int_eq(ret, 4);

	ret = (int)db_getinterfacecount();
	ck_assert_int_eq(ret, 4);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinterfaceidin_can_get_in_groups)
{
	int ret;
	char *result;


	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth3");
	ck_assert_int_eq(ret, 1);

	result = db_getinterfaceidin("eth0");
	ck_assert_str_eq(result, "1");
	free(result);

	result = db_getinterfaceidin("eth0+eth1");
	ck_assert_str_eq(result, "1,2");
	free(result);

	result = db_getinterfaceidin("eth0+eth3");
	ck_assert_str_eq(result, "1,3");
	free(result);

	result = db_getinterfaceidin("eth0+eth3+eth1");
	ck_assert_str_eq(result, "1,2,3");
	free(result);

	result = db_getinterfaceidin("eth0+eth3+eth1+eth4");
	ck_assert_str_eq(result, "1,2,3");
	free(result);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinterfaceidin_can_handle_error_situations)
{
	int ret;
	char *result;


	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	result = db_getinterfaceidin("+eth0");
	ck_assert_ptr_eq(result, NULL);

	result = db_getinterfaceidin("eth4");
	ck_assert_ptr_eq(result, NULL);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinterfaceinfo_can_handle_interface_merges)
{
	int ret;
	interfaceinfo info;


	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("eth0", 1, 1);
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("eth1", 2, 2);
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("eth2", 5, 5);
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfaceinfo("eth0+ethnone", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 1);
	ck_assert_int_eq(info.txtotal, 1);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);
	ck_assert_str_eq(info.name, "eth0+ethnone");

	ret = db_getinterfaceinfo("eth0+eth1", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 3);
	ck_assert_int_eq(info.txtotal, 3);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);
	ck_assert_str_eq(info.name, "eth0+eth1");

	ret = db_getinterfaceinfo("eth1+eth2", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 7);
	ck_assert_int_eq(info.txtotal, 7);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);
	ck_assert_str_eq(info.name, "eth1+eth2");

	ret = db_getinterfaceinfo("eth0+eth1+eth2", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 8);
	ck_assert_int_eq(info.txtotal, 8);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);
	ck_assert_str_eq(info.name, "eth0+eth1+eth2");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinterfaceinfo_can_handle_interface_merges_with_long_name)
{
	int ret;
	interfaceinfo info;


	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("eth0notsolong", 1, 1);
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("eth1withratherlongname", 2, 2);
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("eth2withstillmuchlongername", 5, 5);
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfaceinfo("eth0notsolong+ethnone", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 1);
	ck_assert_int_eq(info.txtotal, 1);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);
	ck_assert_str_eq(info.name, "eth0notsolong+ethnone");

	ret = db_getinterfaceinfo("eth0notsolong+eth1withratherlongname", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 3);
	ck_assert_int_eq(info.txtotal, 3);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);
	ck_assert_str_eq(info.name, "eth0notsolong+eth1withratherlongname");

	ret = db_getinterfaceinfo("eth1withratherlongname+eth2withstillmuchlongername", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 7);
	ck_assert_int_eq(info.txtotal, 7);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);
	ck_assert_str_eq(info.name, "eth1withratherlongname+eth2withstillmuchlongername");

	ret = db_getinterfaceinfo("eth0notsolong+eth1withratherlongname+eth2withstillmuchlongername", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(info.rxtotal, 8);
	ck_assert_int_eq(info.txtotal, 8);
	ck_assert_int_gt(info.updated, 1000);
	ck_assert_int_lt(info.updated, 2100000000);
	ck_assert_str_eq(info.name, "eth0notsolong+eth1withratherlongname+eth2withstillmuchlongername");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_getinterfaceinfo_can_handle_invalid_input)
{
	int ret;
	interfaceinfo info;


	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("eth0", 1, 1);
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("eth1", 2, 2);
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("eth2", 5, 5);
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfaceinfo("eth0+", &info);
	ck_assert_int_eq(ret, 0);

	ret = db_getinterfaceinfo("+", &info);
	ck_assert_int_eq(ret, 0);

	ret = db_getinterfaceinfo("++", &info);
	ck_assert_int_eq(ret, 0);

	ret = db_getinterfaceinfo("", &info);
	ck_assert_int_eq(ret, 0);

	ret = db_getinterfaceinfo("ethunknown", &info);
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(getqueryinterfacecount_can_count)
{
	ck_assert_int_eq(getqueryinterfacecount("eth0"), 1);
	ck_assert_int_eq(getqueryinterfacecount("eth1"), 1);
	ck_assert_int_eq(getqueryinterfacecount("eth1+eth2"), 2);
	ck_assert_int_eq(getqueryinterfacecount("eth1+eth2+eth3"), 3);
	ck_assert_int_eq(getqueryinterfacecount("eth1+eth2+eth3+eth1"), 4);
	ck_assert_int_eq(getqueryinterfacecount("eth0+eth0"), 2);
	ck_assert_int_eq(getqueryinterfacecount("eth0++eth1"), 0);
	ck_assert_int_eq(getqueryinterfacecount(""), 0);
	ck_assert_int_eq(getqueryinterfacecount("1"), 1);
	ck_assert_int_eq(getqueryinterfacecount("+"), 0);
	ck_assert_int_eq(getqueryinterfacecount("++"), 0);
	ck_assert_int_eq(getqueryinterfacecount("+ +"), 0);
	ck_assert_int_eq(getqueryinterfacecount("+ethsomething"), 0);
	ck_assert_int_eq(getqueryinterfacecount("ethnothing+"), 0);
	ck_assert_int_eq(getqueryinterfacecount("eth+nothing"), 2);
	ck_assert_int_eq(getqueryinterfacecount("ethlongcanbelong+ethnotsoshort+ethdoesnotcare"), 3);
	ck_assert_int_eq(getqueryinterfacecount("eth1/eth2"), 1);
	ck_assert_int_eq(getqueryinterfacecount("eth1/eth2+eth3/eth4"), 2);
}
END_TEST

START_TEST(getqueryinterfacecount_can_reject_query_due_to_some_characters)
{
	ck_assert_int_eq(getqueryinterfacecount("eth0;"), 0);
	ck_assert_int_eq(getqueryinterfacecount("<eth0<"), 0);
	ck_assert_int_eq(getqueryinterfacecount(">eth0>"), 0);
	ck_assert_int_eq(getqueryinterfacecount("<eth0>"), 0);
	ck_assert_int_eq(getqueryinterfacecount("\"eth0\""), 0);
	ck_assert_int_eq(getqueryinterfacecount("\'eth0\'"), 0);
	ck_assert_int_eq(getqueryinterfacecount("eth0\'"), 0);
	ck_assert_int_eq(getqueryinterfacecount("eth0\""), 0);
	ck_assert_int_eq(getqueryinterfacecount("eth0\"+eth1"), 0);
	ck_assert_int_eq(getqueryinterfacecount("eth0\\+eth1"), 0);
}
END_TEST

START_TEST(top_list_returns_items_in_correct_order)
{
	int ret;
	dbdatalist *datalist = NULL, *datalist_iterator = NULL;
	dbdatalistinfo datainfo;
	uint64_t previous_entry;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethtest");
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic_dated("ethtest", 0, 0, get_timestamp(2000, 3, 10, 0, 0));
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic_dated("ethtest", 12, 34, get_timestamp(2000, 3, 11, 0, 0));
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic_dated("ethtest", 0, 0, get_timestamp(2000, 3, 12, 0, 0));
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic_dated("ethtest", 0, 0, get_timestamp(2000, 3, 13, 0, 0));
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic_dated("ethtest", 56, 78, get_timestamp(2000, 3, 14, 0, 0));
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic_dated("ethtest", 1, 1, get_timestamp(2000, 3, 15, 0, 0));
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic_dated("ethtest", 1, 1, get_timestamp(2000, 3, 16, 0, 0));
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic_dated("ethtest", 45, 1, get_timestamp(2000, 3, 17, 0, 0));
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "ethtest", "top", 10);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(datainfo.count, 8);
	ck_assert_int_eq(datainfo.maxrx, 56);
	ck_assert_int_eq(datainfo.maxtx, 78);
	ck_assert_int_eq(datainfo.minrx, 0);
	ck_assert_int_eq(datainfo.mintx, 0);

	datalist_iterator = datalist;

	ck_assert_int_eq(datalist_iterator->rx, 56);
	ck_assert_int_eq(datalist_iterator->tx, 78);

	datalist_iterator = datalist_iterator->next;

	ck_assert_int_eq(datalist_iterator->rx, 12);
	ck_assert_int_eq(datalist_iterator->tx, 34);

	datalist_iterator = datalist_iterator->next;

	ck_assert_int_eq(datalist_iterator->rx, 45);
	ck_assert_int_eq(datalist_iterator->tx, 1);

	datalist_iterator = datalist_iterator->next;

	ck_assert_int_eq(datalist_iterator->rx, 1);
	ck_assert_int_eq(datalist_iterator->tx, 1);
	previous_entry = (uint64_t)datalist_iterator->timestamp;

	datalist_iterator = datalist_iterator->next;

	ck_assert_int_eq(datalist_iterator->rx, 1);
	ck_assert_int_eq(datalist_iterator->tx, 1);
	ck_assert_int_lt(previous_entry, (uint64_t)datalist_iterator->timestamp);

	datalist_iterator = datalist_iterator->next;

	ck_assert_int_eq(datalist_iterator->rx, 0);
	ck_assert_int_eq(datalist_iterator->tx, 0);
	previous_entry = (uint64_t)datalist_iterator->timestamp;

	datalist_iterator = datalist_iterator->next;

	ck_assert_int_eq(datalist_iterator->rx, 0);
	ck_assert_int_eq(datalist_iterator->tx, 0);
	ck_assert_int_lt(previous_entry, (uint64_t)datalist_iterator->timestamp);
	previous_entry = (uint64_t)datalist_iterator->timestamp;

	datalist_iterator = datalist_iterator->next;

	ck_assert_int_eq(datalist_iterator->rx, 0);
	ck_assert_int_eq(datalist_iterator->tx, 0);
	ck_assert_int_lt(previous_entry, (uint64_t)datalist_iterator->timestamp);

	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setinterfacebyalias_sets_nothing_when_there_is_no_match)
{
	int ret;
	char interface[32];

	interface[0] = '\0';
	ck_assert_int_eq(strlen(interface), 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_setinterfacebyalias(interface, "internet", 1);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(strlen(interface), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setinterfacebyalias_can_set)
{
	int ret;
	char interface[32];

	interface[0] = '\0';
	ck_assert_int_eq(strlen(interface), 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth0", "lan");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth1", "internet");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth2");
	ck_assert_int_eq(ret, 1);

	ret = db_setinterfacebyalias(interface, "internet", 1);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(strlen(interface), 4);
	ck_assert_str_eq(interface, "eth1");

	ret = db_setinterfacebyalias(interface, "lan", 1);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(strlen(interface), 4);
	ck_assert_str_eq(interface, "eth0");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setinterfacebyalias_sets_highest_traffic_interface_if_alias_is_not_unique)
{
	int ret;
	char interface[32];

	interface[0] = '\0';
	ck_assert_int_eq(strlen(interface), 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth0", "notnet");
	ck_assert_int_eq(ret, 1);

	ret = db_settotal("eth0", 2, 2);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth12");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth12", "notnet");
	ck_assert_int_eq(ret, 1);

	ret = db_settotal("eth12", 2, 3);
	ck_assert_int_eq(ret, 1);

	ret = db_setinterfacebyalias(interface, "notnet", 1);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(strlen(interface), 5);
	ck_assert_str_eq(interface, "eth12");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setinterfacebyalias_can_be_case_insensitive)
{
	int ret;
	char interface[32];

	interface[0] = '\0';
	ck_assert_int_eq(strlen(interface), 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth0", "LAN");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth1", "Internet");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth2");
	ck_assert_int_eq(ret, 1);

	ret = db_setinterfacebyalias(interface, "internet", 1);
	ck_assert_int_eq(ret, 0);

	ret = db_setinterfacebyalias(interface, "lan", 1);
	ck_assert_int_eq(ret, 0);

	ret = db_setinterfacebyalias(interface, "internet", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(strlen(interface), 4);
	ck_assert_str_eq(interface, "eth1");

	ret = db_setinterfacebyalias(interface, "lan", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(strlen(interface), 4);
	ck_assert_str_eq(interface, "eth0");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_setinterfacebyalias_can_match_prefix)
{
	int ret;
	char interface[32];

	interface[0] = '\0';
	ck_assert_int_eq(strlen(interface), 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth0", "LAN");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth1", "Internet");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth2");
	ck_assert_int_eq(ret, 1);

	ret = db_setinterfacebyalias(interface, "in", 1);
	ck_assert_int_eq(ret, 0);

	ret = db_setinterfacebyalias(interface, "la", 1);
	ck_assert_int_eq(ret, 0);

	ret = db_setinterfacebyalias(interface, "in", 2);
	ck_assert_int_eq(ret, 0);

	ret = db_setinterfacebyalias(interface, "la", 2);
	ck_assert_int_eq(ret, 0);

	ret = db_setinterfacebyalias(interface, "in", 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(strlen(interface), 4);
	ck_assert_str_eq(interface, "eth1");

	ret = db_setinterfacebyalias(interface, "la", 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(strlen(interface), 4);
	ck_assert_str_eq(interface, "eth0");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_removedisabledresolutionentries_does_nothing_when_nothing_is_disabled)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_ne(cfg.fiveminutehours, 0);
	ck_assert_int_ne(cfg.hourlydays, 0);
	ck_assert_int_ne(cfg.dailydays, 0);
	ck_assert_int_ne(cfg.monthlymonths, 0);
	ck_assert_int_ne(cfg.yearlyyears, 0);
	ck_assert_int_ne(cfg.topdayentries, 0);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);

	ret = db_removedisabledresolutionentries();
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "fiveminute", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "day", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "month", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "year", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "top", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_removedisabledresolutionentries_removes_fiveminutes_when_disabled)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	cfg.fiveminutehours = 0;

	ck_assert_int_eq(cfg.fiveminutehours, 0);
	ck_assert_int_ne(cfg.hourlydays, 0);
	ck_assert_int_ne(cfg.dailydays, 0);
	ck_assert_int_ne(cfg.monthlymonths, 0);
	ck_assert_int_ne(cfg.yearlyyears, 0);
	ck_assert_int_ne(cfg.topdayentries, 0);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);

	ret = db_removedisabledresolutionentries();
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "fiveminute", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "day", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "month", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "year", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "top", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_removedisabledresolutionentries_removes_hours_when_disabled)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	cfg.hourlydays = 0;

	ck_assert_int_ne(cfg.fiveminutehours, 0);
	ck_assert_int_eq(cfg.hourlydays, 0);
	ck_assert_int_ne(cfg.dailydays, 0);
	ck_assert_int_ne(cfg.monthlymonths, 0);
	ck_assert_int_ne(cfg.yearlyyears, 0);
	ck_assert_int_ne(cfg.topdayentries, 0);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);

	ret = db_removedisabledresolutionentries();
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "fiveminute", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "day", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "month", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "year", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "top", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_removedisabledresolutionentries_removes_days_when_disabled)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	cfg.dailydays = 0;

	ck_assert_int_ne(cfg.fiveminutehours, 0);
	ck_assert_int_ne(cfg.hourlydays, 0);
	ck_assert_int_eq(cfg.dailydays, 0);
	ck_assert_int_ne(cfg.monthlymonths, 0);
	ck_assert_int_ne(cfg.yearlyyears, 0);
	ck_assert_int_ne(cfg.topdayentries, 0);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);

	ret = db_removedisabledresolutionentries();
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "fiveminute", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "day", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "month", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "year", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "top", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_removedisabledresolutionentries_removes_months_when_disabled)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	cfg.monthlymonths = 0;

	ck_assert_int_ne(cfg.fiveminutehours, 0);
	ck_assert_int_ne(cfg.hourlydays, 0);
	ck_assert_int_ne(cfg.dailydays, 0);
	ck_assert_int_eq(cfg.monthlymonths, 0);
	ck_assert_int_ne(cfg.yearlyyears, 0);
	ck_assert_int_ne(cfg.topdayentries, 0);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);

	ret = db_removedisabledresolutionentries();
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "fiveminute", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "day", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "month", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "year", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "top", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_removedisabledresolutionentries_removes_years_when_disabled)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	cfg.yearlyyears = 0;

	ck_assert_int_ne(cfg.fiveminutehours, 0);
	ck_assert_int_ne(cfg.hourlydays, 0);
	ck_assert_int_ne(cfg.dailydays, 0);
	ck_assert_int_ne(cfg.monthlymonths, 0);
	ck_assert_int_eq(cfg.yearlyyears, 0);
	ck_assert_int_ne(cfg.topdayentries, 0);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);

	ret = db_removedisabledresolutionentries();
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "fiveminute", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "day", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "month", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "year", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "top", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(db_removedisabledresolutionentries_removes_top_days_when_disabled)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	cfg.topdayentries = 0;

	ck_assert_int_ne(cfg.fiveminutehours, 0);
	ck_assert_int_ne(cfg.hourlydays, 0);
	ck_assert_int_ne(cfg.dailydays, 0);
	ck_assert_int_ne(cfg.monthlymonths, 0);
	ck_assert_int_ne(cfg.yearlyyears, 0);
	ck_assert_int_eq(cfg.topdayentries, 0);

	ck_assert_int_eq(db_addtraffic("eth0", 12, 34), 1);

	ret = db_removedisabledresolutionentries();
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "fiveminute", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "day", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "month", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "year", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_gt(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "eth0", "top", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 0);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

void add_dbsql_tests(Suite *s)
{
	TCase *tc_dbsql = tcase_create("DB SQL");
	tcase_add_checked_fixture(tc_dbsql, setup, teardown);
	tcase_add_unchecked_fixture(tc_dbsql, setup, teardown);
	tcase_add_test(tc_dbsql, db_close_does_no_harm_when_db_is_already_closed);
	tcase_add_test(tc_dbsql, db_open_rw_can_create_database_if_file_does_not_exist);
	tcase_add_test(tc_dbsql, db_open_ro_cannot_create_a_database);
	tcase_add_test(tc_dbsql, db_getinfo_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_getinfo_fails_with_nonexisting_name);
	tcase_add_test(tc_dbsql, db_getinfo_can_get_dbversion);
	tcase_add_test(tc_dbsql, db_setinfo_fails_with_no_open_db);
	tcase_add_test(tc_dbsql, db_setinfo_can_set_infos);
	tcase_add_test(tc_dbsql, db_setinfo_can_update_infos);
	tcase_add_test(tc_dbsql, db_setinfo_can_not_update_nonexisting_name);
	tcase_add_test(tc_dbsql, db_addtraffic_with_no_traffic_does_nothing);
	tcase_add_test(tc_dbsql, db_addtraffic_can_add_traffic_and_interfaces);
	tcase_add_test(tc_dbsql, db_addtraffic_can_add_traffic_and_interfaces_utc);
	tcase_add_test(tc_dbsql, db_addtraffic_adds_traffic_to_different_data_types);
	tcase_add_test(tc_dbsql, db_addtraffic_does_not_add_traffic_to_disabled_data_types);
	tcase_add_test(tc_dbsql, db_addtraffic_dated_does_not_touch_updated_time);
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
	tcase_add_test(tc_dbsql, db_renameinterface_knows_if_interface_exists);
	tcase_add_test(tc_dbsql, db_renameinterface_can_rename_interfaces);
	tcase_add_test(tc_dbsql, db_getcounters_with_no_interface);
	tcase_add_test(tc_dbsql, db_setcounters_with_no_interface);
	tcase_add_test(tc_dbsql, db_interface_info_manipulation);
	tcase_add_test(tc_dbsql, db_getiflist_lists_interfaces);
	tcase_add_test(tc_dbsql, db_maintenance_does_not_fault);
	tcase_add_test(tc_dbsql, db_data_can_be_inserted);
	tcase_add_test(tc_dbsql, db_data_can_be_retrieved);
	tcase_add_test(tc_dbsql, db_data_can_be_inserted_utc);
	tcase_add_test(tc_dbsql, db_data_can_be_retrieved_utc);
	tcase_add_test(tc_dbsql, db_fatal_errors_get_detected);
	tcase_add_test(tc_dbsql, db_validate_with_valid_version);
	tcase_add_test(tc_dbsql, db_validate_with_no_version);
	tcase_add_test(tc_dbsql, db_validate_with_low_version);
	tcase_add_test(tc_dbsql, db_validate_with_high_version);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_months_without_range_defined);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_months_with_range_matching_existing_data);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_months_with_range_past_existing_data);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_months_with_range_limiting_begin_and_end);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_months_with_range_limiting_begin);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_months_with_range_limiting_begin_with_limit);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_months_with_range_limiting_end);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_months_with_range_limiting_end_with_limit);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_months_with_range_on_same_month);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_months_with_range_past_first_day_of_month);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_hours_without_range_defined);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_hours_with_range_matching_existing_data);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_hours_with_range_past_existing_data);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_hours_with_range_limiting_begin_and_end);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_hours_with_range_limiting_begin);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_hours_with_range_limiting_begin_with_limit);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_hours_with_range_limiting_end);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_hours_with_range_limiting_end_with_limit);
	tcase_add_test(tc_dbsql, db_getdata_range_can_get_hours_with_range_on_same_hour);
	tcase_add_test(tc_dbsql, db_getdata_range_with_merged_interfaces);
	tcase_add_test(tc_dbsql, db_getdata_range_with_long_merged_interfaces);
	tcase_add_test(tc_dbsql, db_addtraffic_without_monthrotate);
	tcase_add_test(tc_dbsql, db_addtraffic_with_monthrotate);
	tcase_add_test(tc_dbsql, db_addtraffic_without_monthrotate_utc);
	tcase_add_test(tc_dbsql, db_addtraffic_with_monthrotate_utc);
	tcase_add_test(tc_dbsql, db_get_date_generator_can_generate_dates);
	tcase_add_test(tc_dbsql, db_get_date_generator_can_generate_dates_with_monthrotate);
	tcase_add_test(tc_dbsql, db_get_date_generator_can_generate_dates_utc);
	tcase_add_test(tc_dbsql, db_get_date_generator_can_generate_dates_with_monthrotate_utc);
	tcase_add_test(tc_dbsql, getifaceinquery_does_not_mess_regular_interfaces);
	tcase_add_test(tc_dbsql, getifaceinquery_can_create_merge_queries);
	tcase_add_test(tc_dbsql, getifaceinquery_can_create_merge_query_longer_than_single_interface_maxlen);
	tcase_add_test(tc_dbsql, getifaceinquery_does_not_tolerate_nonsense);
	tcase_add_test(tc_dbsql, db_getinterfaceid_can_get_ids);
	tcase_add_test(tc_dbsql, db_getinterfaceidin_can_get_in_groups);
	tcase_add_test(tc_dbsql, db_getinterfaceidin_can_handle_error_situations);
	tcase_add_test(tc_dbsql, db_getinterfaceinfo_can_handle_interface_merges);
	tcase_add_test(tc_dbsql, db_getinterfaceinfo_can_handle_interface_merges_with_long_name);
	tcase_add_test(tc_dbsql, db_getinterfaceinfo_can_handle_invalid_input);
	tcase_add_test(tc_dbsql, getqueryinterfacecount_can_count);
	tcase_add_test(tc_dbsql, getqueryinterfacecount_can_reject_query_due_to_some_characters);
	tcase_add_test(tc_dbsql, top_list_returns_items_in_correct_order);
	tcase_add_test(tc_dbsql, db_setinterfacebyalias_sets_nothing_when_there_is_no_match);
	tcase_add_test(tc_dbsql, db_setinterfacebyalias_can_set);
	tcase_add_test(tc_dbsql, db_setinterfacebyalias_sets_highest_traffic_interface_if_alias_is_not_unique);
	tcase_add_test(tc_dbsql, db_setinterfacebyalias_can_be_case_insensitive);
	tcase_add_test(tc_dbsql, db_setinterfacebyalias_can_match_prefix);
	tcase_add_test(tc_dbsql, db_removedisabledresolutionentries_does_nothing_when_nothing_is_disabled);
	tcase_add_test(tc_dbsql, db_removedisabledresolutionentries_removes_fiveminutes_when_disabled);
	tcase_add_test(tc_dbsql, db_removedisabledresolutionentries_removes_hours_when_disabled);
	tcase_add_test(tc_dbsql, db_removedisabledresolutionentries_removes_days_when_disabled);
	tcase_add_test(tc_dbsql, db_removedisabledresolutionentries_removes_months_when_disabled);
	tcase_add_test(tc_dbsql, db_removedisabledresolutionentries_removes_years_when_disabled);
	tcase_add_test(tc_dbsql, db_removedisabledresolutionentries_removes_top_days_when_disabled);
	suite_add_tcase(s, tc_dbsql);
}
