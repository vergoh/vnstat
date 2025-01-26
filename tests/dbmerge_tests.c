#include "common.h"
#include "vnstat_tests.h"
#include "dbmerge_tests.h"
#include "dbsql.h"
#include "dbmerge.h"
#include "cfg.h"

START_TEST(mergedata_with_unsupported_table)
{
    suppress_output();
	ck_assert_int_eq(mergedata("unsupported", 1, 2, 3, 1234), 0);
}
END_TEST

START_TEST(mergedata_with_zero_timestamp)
{
    suppress_output();
	ck_assert_int_eq(mergedata("top", 1, 2, 3, 0), 0);
}
END_TEST

START_TEST(mergedata_with_ifaceid_not_in_database)
{
	int ret;

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);

    suppress_output();
	ck_assert_int_eq(mergedata("top", 1, 2, 3, 1234), 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(mergedata_with_can_insert_data)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ck_assert_int_ne(cfg.hourlydays, 0);

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);

    ret = db_addinterface("eth0");
    ck_assert_int_eq(ret, 1);

    suppress_output();
    ret = mergedata("hour", 1, 2, 3, 97200);
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 1);
    ck_assert_int_eq(datalist->rx, 2);
    ck_assert_int_eq(datalist->tx, 3);
    ck_assert_int_eq(datalist->timestamp, 97200);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(mergedata_with_can_merge_data)
{
	int ret;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ck_assert_int_ne(cfg.hourlydays, 0);

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);

    ret = db_addinterface("eth0");
    ck_assert_int_eq(ret, 1);

    ret = db_addtraffic_dated("eth0", 2, 3, 97200);
    ck_assert_int_eq(ret, 1);

    suppress_output();
    ret = mergedata("hour", 1, 2, 3, 97200);
	ck_assert_int_eq(ret, 1);

	ret = db_getdata(&datalist, &datainfo, "eth0", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 1);
    ck_assert_int_eq(datalist->rx, 4);
    ck_assert_int_eq(datalist->tx, 6);
    ck_assert_int_eq(datalist->timestamp, 97200);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(mergeinterface_without_source_interface_existing)
{
	int ret;
    sqlite3 *srcdb = NULL, *dstdb = NULL;

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);
    srcdb = db;

    db = NULL;

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);
    dstdb = db;

    suppress_output();
    ret = mergeinterface(srcdb, "eth0", dstdb, "eth0");
    ck_assert_int_eq(ret, 0);

    db = srcdb;
	ret = db_close();
	ck_assert_int_eq(ret, 1);

    db = dstdb;
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(mergeinterface_without_source_data)
{
	int ret;
    sqlite3 *srcdb = NULL, *dstdb = NULL;

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);
    srcdb = db;
    ret = db_addinterface("eth0");
    ck_assert_int_eq(ret, 1);

    db = NULL;

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);
    dstdb = db;

    ret = mergeinterface(srcdb, "eth0", dstdb, "eth0");
    ck_assert_int_eq(ret, 1);

    db = srcdb;
	ret = db_close();
	ck_assert_int_eq(ret, 1);

    db = dstdb;
    ck_assert_int_eq(db_getinterfacecountbyname("eth0"), 1);
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(mergeinterface_with_source_data)
{
	int ret;
    sqlite3 *srcdb = NULL, *dstdb = NULL;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);
    srcdb = db;
    ret = db_addinterface("eth0");
    ck_assert_int_eq(ret, 1);
    ret = db_addtraffic_dated("eth0", 23, 45, 1737835200);
    ck_assert_int_eq(ret, 1);

    db = NULL;

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);
    dstdb = db;

    ret = mergeinterface(srcdb, "eth0", dstdb, "em1");
    ck_assert_int_eq(ret, 1);

    db = srcdb;
	ret = db_close();
	ck_assert_int_eq(ret, 1);

    db = dstdb;
    ck_assert_int_eq(db_getinterfacecountbyname("eth0"), 0);
    ck_assert_int_eq(db_getinterfacecountbyname("em1"), 1);
	ret = db_getdata(&datalist, &datainfo, "em1", "hour", 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 1);
    ck_assert_int_eq(datalist->rx, 23);
    ck_assert_int_eq(datalist->tx, 45);
    ck_assert_int_eq(datalist->timestamp, 1737835200);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(mergeinterface_with_source_and_destination_data)
{
	int ret;
    sqlite3 *srcdb = NULL, *dstdb = NULL;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;
    interfaceinfo info;

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);
    srcdb = db;
    ret = db_addinterface("em2");
    ck_assert_int_eq(ret, 1);
    ret = db_addtraffic_dated("em2", 1, 1, 1499997600);
    ck_assert_int_eq(ret, 1);
    ret = db_addinterface("eth1");
    ck_assert_int_eq(ret, 1);
    ret = db_addtraffic_dated("eth1", 2, 3, 1499997600);
    ck_assert_int_eq(ret, 1);
    ret = db_addtraffic_dated("eth1", 23, 45, 1649998800);
    ck_assert_int_eq(ret, 1);
    ret = db_setcreation("eth1", 1499997600);
    ck_assert_int_eq(ret, 1);
    ret = db_setalias("eth1", "Fast");
    ck_assert_int_eq(ret, 1);

    db = NULL;

	ret = db_open_rw(1);
    ck_assert_int_eq(ret, 1);
    dstdb = db;
    ret = db_addinterface("em2");
    ck_assert_int_eq(ret, 1);
    ret = db_addtraffic_dated("em2", 45, 67, 1649998800);
    ck_assert_int_eq(ret, 1);
    ret = db_addtraffic_dated("em2", 67, 89, 1737835200);
    ck_assert_int_eq(ret, 1);
    ret = db_setcreation("em2", 1649998800);
    ck_assert_int_eq(ret, 1);

    ret = mergeinterface(srcdb, "eth1", dstdb, "em2");
    ck_assert_int_eq(ret, 1);

    db = srcdb;
	ret = db_close();
	ck_assert_int_eq(ret, 1);

    db = dstdb;
    ck_assert_int_eq(db_getinterfacecountbyname("eth0"), 0);
    ck_assert_int_eq(db_getinterfacecountbyname("eth1"), 0);
    ck_assert_int_eq(db_getinterfacecountbyname("em2"), 1);
    ret = db_getinterfaceinfo("em2", &info);
    ck_assert_int_eq(ret, 1);
    ck_assert_str_eq(info.alias, "Fast");
    ck_assert_int_eq(info.created, 1499997600);
	ret = db_getdata(&datalist, &datainfo, "em2", "hour", 5);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 3);
    ck_assert_int_eq(datalist->rx, 2);
    ck_assert_int_eq(datalist->tx, 3);
    ck_assert_int_eq(datalist->timestamp, 1499997600);
    ck_assert_ptr_ne(datalist->next, NULL);
    ck_assert_int_eq(datalist->next->rx, 68);
    ck_assert_int_eq(datalist->next->tx, 112);
    ck_assert_int_eq(datalist->next->timestamp, 1649998800);
    ck_assert_ptr_ne(datalist->next->next, NULL);
    ck_assert_int_eq(datalist->next->next->rx, 67);
    ck_assert_int_eq(datalist->next->next->tx, 89);
    ck_assert_int_eq(datalist->next->next->timestamp, 1737835200);
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

void add_dbmerge_tests(Suite *s)
{
	TCase *tc_dbmerge = tcase_create("DB Merge");
	tcase_add_checked_fixture(tc_dbmerge, setup, teardown);
	tcase_add_unchecked_fixture(tc_dbmerge, setup, teardown);

    tcase_add_test(tc_dbmerge, mergedata_with_unsupported_table);
    tcase_add_test(tc_dbmerge, mergedata_with_zero_timestamp);
    tcase_add_test(tc_dbmerge, mergedata_with_ifaceid_not_in_database);
    tcase_add_test(tc_dbmerge, mergedata_with_can_insert_data);
    tcase_add_test(tc_dbmerge, mergedata_with_can_merge_data);
    tcase_add_test(tc_dbmerge, mergeinterface_without_source_interface_existing);
    tcase_add_test(tc_dbmerge, mergeinterface_without_source_data);
    tcase_add_test(tc_dbmerge, mergeinterface_with_source_data);
    tcase_add_test(tc_dbmerge, mergeinterface_with_source_and_destination_data);

	suite_add_tcase(s, tc_dbmerge);
}
