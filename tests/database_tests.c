#include "common.h"
#include "vnstat_tests.h"
#include "database_tests.h"
#include "dbaccess.h"
#include "ifinfo.h"
#include "percentile.h"
#include "dbsql.h"
#include "dbshow.h"
#include "dbxml.h"
#include "dbjson.h"
#include "cfg.h"
#include "ibw.h"
#include "fs.h"

int writedb(DATA *data, const char *iface, const char *dirname, int newdb);
int backupdb(const char *current, const char *backup);

START_TEST(initdb_activates_database)
{
	DATA data;
	initdb(&data);
	ck_assert_int_eq(data.active, 1);
}
END_TEST

START_TEST(readdb_with_empty_file)
{
	DATA data;
	disable_logprints();
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("existingdb"), 1);
	ck_assert_int_eq(readdb(&data, "existingdb", TESTDBDIR, 0), -1);
}
END_TEST

START_TEST(readdb_with_empty_file_and_backup)
{
	DATA data;
	disable_logprints();
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("existingdb"), 1);
	ck_assert_int_eq(create_zerosize_dbfile(".existingdb"), 1);
	ck_assert_int_eq(readdb(&data, "existingdb", TESTDBDIR, 0), -1);
}
END_TEST

START_TEST(readdb_with_nonexisting_file)
{
	DATA data;
	disable_logprints();
	strcpy(data.interface, "none");
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(readdb(&data, "existingdb", TESTDBDIR, 0), 1);
	ck_assert_str_eq(data.interface, "existingdb");
	ck_assert_str_eq(data.nick, "existingdb");
}
END_TEST

START_TEST(readdb_with_existing_dbfile)
{
	DATA data;
	initdb(&data);
	disable_logprints();
	strcpy(data.interface, "ethtest");
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(writedb(&data, "ethtest", TESTDBDIR, 1), 1);
	ck_assert_int_eq(check_dbfile_exists("ethtest", sizeof(DATA)), 1);

	strcpy(data.interface, "none");
	ck_assert_int_eq(readdb(&data, "ethtest", TESTDBDIR, 0), 0);
	ck_assert_str_eq(data.interface, "ethtest");
}
END_TEST

START_TEST(readdb_with_existing_dbfile_and_max_name_length)
{
	DATA data;
	initdb(&data);
	disable_logprints();
	strcpy(data.interface, "1234567890123456789012345678901");
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(writedb(&data, "1234567890123456789012345678901", TESTDBDIR, 1), 1);
	ck_assert_int_eq(check_dbfile_exists("1234567890123456789012345678901", sizeof(DATA)), 1);

	strcpy(data.interface, "none");
	ck_assert_int_eq(readdb(&data, "1234567890123456789012345678901", TESTDBDIR, 0), 0);
	ck_assert_str_eq(data.interface, "1234567890123456789012345678901");
}
END_TEST

START_TEST(readdb_with_existing_dbfile_with_rename)
{
	DATA data;
	initdb(&data);
	disable_logprints();
	strcpy(data.interface, "ethtest");
	strcpy(data.nick, "ethtest");
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(writedb(&data, "ethtest2", TESTDBDIR, 1), 1);
	ck_assert_int_eq(check_dbfile_exists("ethtest2", sizeof(DATA)), 1);

	strcpy(data.interface, "none");
	strcpy(data.nick, "none");
	ck_assert_int_eq(readdb(&data, "ethtest2", TESTDBDIR, 0), 0);
	ck_assert_str_eq(data.interface, "ethtest2");
	ck_assert_str_eq(data.nick, "ethtest2");
}
END_TEST

START_TEST(readdb_with_existing_dbfile_and_over_max_name_length)
{
	DATA data;
	initdb(&data);
	disable_logprints();
	strcpy(data.interface, "dummy");
	strcpy(data.nick, "dummy");
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(writedb(&data, "1234567890123456789012345678901XX", TESTDBDIR, 1), 1);
	ck_assert_int_eq(check_dbfile_exists("1234567890123456789012345678901XX", sizeof(DATA)), 1);

	strcpy(data.interface, "none");
	strcpy(data.nick, "none");
	ck_assert_int_eq(readdb(&data, "1234567890123456789012345678901XX", TESTDBDIR, 0), 0);
	ck_assert_str_eq(data.interface, "1234567890123456789012345678901");
	ck_assert_str_eq(data.nick, "1234567890123456789012345678901");
}
END_TEST

START_TEST(validatedb_with_initdb)
{
	DATA data;
	initdb(&data);
	strcpy(data.interface, "ethtest");
	ck_assert_int_eq(validatedb(&data), 1);
}
END_TEST

START_TEST(validatedb_with_invalid_totals)
{
	DATA data;
	initdb(&data);
	suppress_output();
	strcpy(data.interface, "ethtest");
	data.day[0].rx++;
	ck_assert_int_eq(validatedb(&data), 0);
}
END_TEST

START_TEST(validatedb_with_top10_use)
{
	DATA data;
	initdb(&data);
	suppress_output();
	strcpy(data.interface, "ethtest");
	data.top10[0].used = 1;
	data.top10[1].used = 1;
	data.top10[2].used = 1;
	data.top10[5].used = 1;
	ck_assert_int_eq(validatedb(&data), 0);
}
END_TEST

START_TEST(database_outputs_do_not_crash)
{
	int ret, i;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i < 100; i++) {
		ret = db_addtraffic_dated("something", (uint64_t)i * 1234, (uint64_t)i * 2345, (uint64_t)i * 85000);
		ck_assert_int_eq(ret, 1);
	}

	ret = db_setupdated("something", (time_t)i * 85000);
	ck_assert_int_eq(ret, 1);

	suppress_output();

	for (i = 0; i <= 4; i++) {
		cfg.ostyle = i;
		showdb("something", 0, "", "");
		showdb("something", 1, "", "");
		showdb("something", 2, "", "");
		showdb("something", 3, "", "");
		showdb("something", 4, "", "");
		showdb("something", 5, "", "");
		showdb("something", 6, "", "");
		showdb("something", 7, "", "");
		showdb("something", 9, "", "");
		showdb("something", 11, "", "");
		showdb("something", 12, "", "");
		showdb("something", 13, "", "");
	}

	xmlheader();
	showxml("something", 'd', "", "");
	showxml("something", 'm', "", "");
	showxml("something", 't', "", "");
	showxml("something", 'h', "", "");
	showxml("something", 'y', "", "");
	showxml("something", 'f', "", "");
	showxml("something", 'a', "", "");
	showxml("something", 's', "", "");
	xmlfooter();

	jsonheader(JSONVERSION);
	showjson("something", 0, 'd', "", "");
	showjson("something", 0, 'm', "", "");
	showjson("something", 0, 't', "", "");
	showjson("something", 0, 'h', "", "");
	showjson("something", 0, 'y', "", "");
	showjson("something", 0, 'f', "", "");
	showjson("something", 0, 'a', "", "");
	showjson("something", 1, 'a', "", "");
	showjson("something", 0, 's', "", "");
	jsonfooter();

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(database_showdb_exists_for_invalid_interface)
{
	int ret, i;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i < 100; i++) {
		ret = db_addtraffic_dated("something", (uint64_t)i * 1234, (uint64_t)i * 2345, (uint64_t)i * 85000);
		ck_assert_int_eq(ret, 1);
	}

	ret = db_setupdated("something", (time_t)i * 85000);
	ck_assert_int_eq(ret, 1);

	suppress_output();

	showdb("nothing", 0, "", "");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(database_showjson_exists_for_invalid_interface)
{
	int ret, i;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i < 100; i++) {
		ret = db_addtraffic_dated("something", (uint64_t)i * 1234, (uint64_t)i * 2345, (uint64_t)i * 85000);
		ck_assert_int_eq(ret, 1);
	}

	ret = db_setupdated("something", (time_t)i * 85000);
	ck_assert_int_eq(ret, 1);

	suppress_output();

	showjson("nothing", 0, 'a', "", "");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(database_showxml_exists_for_invalid_interface)
{
	int ret, i;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i < 100; i++) {
		ret = db_addtraffic_dated("something", (uint64_t)i * 1234, (uint64_t)i * 2345, (uint64_t)i * 85000);
		ck_assert_int_eq(ret, 1);
	}

	ret = db_setupdated("something", (time_t)i * 85000);
	ck_assert_int_eq(ret, 1);

	suppress_output();

	showxml("nothing", 'a', "", "");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(database_outputs_do_not_crash_without_traffic)
{
	int ret, i;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic_dated("something", 0, 0, 85000);
	ck_assert_int_eq(ret, 1);

	ret = db_setupdated("something", 85000);
	ck_assert_int_eq(ret, 1);

	suppress_output();

	for (i = 0; i <= 4; i++) {
		cfg.ostyle = i;
		showdb("something", 0, "", "");
		showdb("something", 1, "", "");
		showdb("something", 2, "", "");
		showdb("something", 3, "", "");
		showdb("something", 4, "", "");
		showdb("something", 5, "", "");
		showdb("something", 6, "", "");
		showdb("something", 7, "", "");
		showdb("something", 9, "", "");
		showdb("something", 11, "", "");
		showdb("something", 12, "", "");
		showdb("something", 13, "", "");
	}

	xmlheader();
	showxml("something", 'd', "", "");
	showxml("something", 'm', "", "");
	showxml("something", 't', "", "");
	showxml("something", 'h', "", "");
	showxml("something", 'y', "", "");
	showxml("something", 'f', "", "");
	showxml("something", 's', "", "");
	showxml("something", 'a', "", "");

	xmlfooter();

	jsonheader(JSONVERSION);
	showjson("something", 0, 'd', "", "");
	showjson("something", 0, 'm', "", "");
	showjson("something", 0, 't', "", "");
	showjson("something", 0, 'h', "", "");
	showjson("something", 0, 'y', "", "");
	showjson("something", 0, 'f', "", "");
	showjson("something", 0, 'a', "", "");
	showjson("something", 1, 'a', "", "");
	showjson("something", 0, 's', "", "");
	jsonfooter();

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(database_outputs_do_not_crash_without_data)
{
	int ret, i;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	suppress_output();

	for (i = 0; i <= 4; i++) {
		cfg.ostyle = i;
		showdb("something", 0, "", "");
		showdb("something", 1, "", "");
		showdb("something", 2, "", "");
		showdb("something", 3, "", "");
		showdb("something", 4, "", "");
		showdb("something", 5, "", "");
		showdb("something", 6, "", "");
		showdb("something", 7, "", "");
		showdb("something", 9, "", "");
		showdb("something", 11, "", "");
		showdb("something", 12, "", "");
		showdb("something", 13, "", "");
	}

	xmlheader();
	showxml("something", 'd', "", "");
	showxml("something", 'm', "", "");
	showxml("something", 't', "", "");
	showxml("something", 'h', "", "");
	showxml("something", 'y', "", "");
	showxml("something", 'f', "", "");
	showxml("something", 's', "", "");
	showxml("something", 'a', "", "");

	xmlfooter();

	jsonheader(JSONVERSION);
	showjson("something", 0, 'd', "", "");
	showjson("something", 0, 'm', "", "");
	showjson("something", 0, 't', "", "");
	showjson("something", 0, 'h', "", "");
	showjson("something", 0, 'y', "", "");
	showjson("something", 0, 'f', "", "");
	showjson("something", 0, 'a', "", "");
	showjson("something", 1, 'a', "", "");
	showjson("something", 0, 's', "", "");
	jsonfooter();

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(database_outputs_do_not_crash_without_data_if_totals_are_wrong)
{
	int ret, i;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	ret = db_settotal("something", 42, 84);
	ck_assert_int_eq(ret, 1);

	ret = db_setupdated("something", 85000);
	ck_assert_int_eq(ret, 1);

	suppress_output();

	for (i = 0; i <= 4; i++) {
		cfg.ostyle = i;
		showdb("something", 0, "", "");
		showdb("something", 1, "", "");
		showdb("something", 2, "", "");
		showdb("something", 3, "", "");
		showdb("something", 4, "", "");
		showdb("something", 5, "", "");
		showdb("something", 6, "", "");
		showdb("something", 7, "", "");
		showdb("something", 9, "", "");
		showdb("something", 11, "", "");
		showdb("something", 12, "", "");
	}

	xmlheader();
	showxml("something", 'd', "", "");
	showxml("something", 'm', "", "");
	showxml("something", 't', "", "");
	showxml("something", 'h', "", "");
	showxml("something", 'y', "", "");
	showxml("something", 'f', "", "");
	showxml("something", 's', "", "");
	showxml("something", 'a', "", "");

	xmlfooter();

	jsonheader(JSONVERSION);
	showjson("something", 0, 'd', "", "");
	showjson("something", 0, 'm', "", "");
	showjson("something", 0, 't', "", "");
	showjson("something", 0, 'h', "", "");
	showjson("something", 0, 'y', "", "");
	showjson("something", 0, 'f', "", "");
	showjson("something", 0, 'a', "", "");
	showjson("something", 1, 'a', "", "");
	showjson("something", 0, 's', "", "");
	jsonfooter();

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(showbar_with_zero_len_is_nothing)
{
	int len;
	suppress_output();
	len = showbar(1, 2, 3, 0);
	ck_assert_int_eq(len, 0);
}
END_TEST

START_TEST(showbar_with_zero_max)
{
	int len;
	suppress_output();
	len = showbar(0, 0, 0, 10);
	ck_assert_int_eq(len, 0);
}
END_TEST

START_TEST(showbar_with_big_max_and_small_numbers)
{
	int len;
	suppress_output();
	len = showbar(1, 2, 1000, 10);
	ck_assert_int_eq(len, 0);
}
END_TEST

START_TEST(showbar_with_all_rx)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(1, 0, 1, 10);
	ck_assert_int_eq(len, 10);
	fflush(stdout);

	len = (int)read(pipe, buffer, 512);
	ck_assert_str_eq(buffer, "  rrrrrrrrrr");
}
END_TEST

START_TEST(showbar_with_all_tx)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(0, 1, 1, 10);
	ck_assert_int_eq(len, 10);
	fflush(stdout);

	len = (int)read(pipe, buffer, 512);
	ck_assert_str_eq(buffer, "  tttttttttt");
}
END_TEST

START_TEST(showbar_with_half_and_half)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(1, 1, 2, 10);
	ck_assert_int_eq(len, 10);
	fflush(stdout);

	len = (int)read(pipe, buffer, 512);
	ck_assert_str_eq(buffer, "  rrrrrttttt");
}
END_TEST

START_TEST(showbar_with_one_tenth)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(1, 9, 10, 10);
	ck_assert_int_eq(len, 10);
	fflush(stdout);

	len = (int)read(pipe, buffer, 512);
	ck_assert_str_eq(buffer, "  rttttttttt");
}
END_TEST

START_TEST(showbar_with_small_rx_shows_all_tx)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(1, 1000, 1001, 10);
	ck_assert_int_eq(len, 10);
	fflush(stdout);

	len = (int)read(pipe, buffer, 512);
	ck_assert_str_eq(buffer, "  tttttttttt");
}
END_TEST

START_TEST(showbar_with_max_smaller_than_real_max)
{
	int len;
	suppress_output();
	len = showbar(1, 2, 1, 10);
	ck_assert_int_eq(len, 0);
}
END_TEST

START_TEST(showbar_with_half_and_half_of_half)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(1, 1, 4, 12);
	ck_assert_int_eq(len, 6);
	fflush(stdout);

	len = (int)read(pipe, buffer, 512);
	ck_assert_str_eq(buffer, "  rrrttt");
}
END_TEST

START_TEST(importlegacydb_does_not_overwrite_existing_interface_data)
{
	int ret;
	disable_logprints();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethtest");
	ck_assert_int_eq(ret, 1);

	ret = importlegacydb("ethtest", TESTDBDIR);
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(importlegacydb_can_detect_when_database_read_fails)
{
	int ret;
	disable_logprints();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethsomethingelse");
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("ethtest"), 1);
	ck_assert_int_eq(create_zerosize_dbfile(".ethtest"), 1);

	ret = importlegacydb("ethtest", TESTDBDIR);
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(importlegacydb_can_import_legacy_database)
{
	int ret, i;
	DATA data;
	interfaceinfo info;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	initdb(&data);
	disable_logprints();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethsomethingelse");
	ck_assert_int_eq(ret, 1);
	ret = (int)db_getinterfacecount();
	ck_assert_int_eq(ret, 1);

	strcpy(data.interface, "ethtest");
	strcpy(data.nick, "still testing");
	data.totalrx = 123123123;
	data.totaltx = 321321321;
	data.totalrxk = 1;
	data.totaltxk = 2;
	data.currx = 456;
	data.curtx = 654;
	for (i = 0; i < 24; i++) {
		data.hour[i].date = 788911200 + 3600 * i;
		data.hour[i].rx = (uint64_t)(12 * (i + 1));
		data.hour[i].tx = (uint64_t)(23 * (i + 1));
	}
	for (i = 0; i < 30; i++) {
		data.day[i].date = 788911200 + 86400 * (29 - i);
		data.day[i].used = 1;
		data.day[i].rx = (uint64_t)(34 * i);
		data.day[i].tx = (uint64_t)(45 * i);
	}
	for (i = 0; i < 12; i++) {
		data.month[i].month = 788911200 + 2678400 * (11 - i);
		data.month[i].used = 1;
		data.month[i].rx = (uint64_t)(56 * i);
		data.month[i].tx = (uint64_t)(67 * i);
	}
	for (i = 0; i < 10; i++) {
		data.top10[i].date = 788911200 + 86400 * i;
		data.top10[i].used = 1;
		data.top10[i].rx = (uint64_t)(89 * (9 - i + 1));
		data.top10[i].tx = (uint64_t)(90 * (9 - i + 1));
	}

	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(writedb(&data, "ethtest", TESTDBDIR, 1), 1);
	ck_assert_int_eq(check_dbfile_exists("ethtest", sizeof(DATA)), 1);

	ret = importlegacydb("ethtest", TESTDBDIR);
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecount();
	ck_assert_int_eq(ret, 2);

	ret = (int)db_getinterfacecountbyname("ethtest");
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfaceinfo("ethtest", &info);
	ck_assert_int_eq(ret, 1);

	ck_assert_str_eq(info.alias, data.nick);
	ck_assert_int_eq(info.active, data.active);
	ck_assert_int_eq(info.rxtotal, (data.totalrx * 1024 * 1024) + (uint64_t)(data.totalrxk * 1024));
	ck_assert_int_eq(info.txtotal, (data.totaltx * 1024 * 1024) + (uint64_t)(data.totaltxk * 1024));
	ck_assert_int_eq(info.rxcounter, data.currx);
	ck_assert_int_eq(info.txcounter, data.curtx);
	ck_assert_int_ge(info.created, data.created);
	ck_assert_int_ge(info.updated, data.lastupdated);

	ret = db_getdata(&datalist, &datainfo, "ethtest", "day", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 30);
	datalist_i = datalist;
	i = 29;
	while (datalist_i != NULL) {
		ck_assert_int_eq(datalist_i->rx, data.day[i].rx * 1024 * 1024);
		ck_assert_int_eq(datalist_i->tx, data.day[i].tx * 1024 * 1024);
		datalist_i = datalist_i->next;
		i--;
	}
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "ethtest", "month", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 12);
	datalist_i = datalist;
	i = 11;
	while (datalist_i != NULL) {
		ck_assert_int_eq(datalist_i->rx, data.month[i].rx * 1024 * 1024);
		ck_assert_int_eq(datalist_i->tx, data.month[i].tx * 1024 * 1024);
		datalist_i = datalist_i->next;
		i--;
	}
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "ethtest", "hour", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 24);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		ck_assert_int_eq(datalist_i->rx, data.hour[i].rx * 1024);
		ck_assert_int_eq(datalist_i->tx, data.hour[i].tx * 1024);
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_getdata(&datalist, &datainfo, "ethtest", "top", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(datainfo.count, 10);
	datalist_i = datalist;
	i = 0;
	while (datalist_i != NULL) {
		ck_assert_int_eq(datalist_i->rx, data.top10[i].rx * 1024 * 1024);
		ck_assert_int_eq(datalist_i->tx, data.top10[i].tx * 1024 * 1024);
		datalist_i = datalist_i->next;
		i++;
	}
	dbdatalistfree(&datalist);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST


START_TEST(showalert_shows_nothing_with_none_type)
{
	int ret;

	defaultcfg();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("something", 100, 200);
	ck_assert_int_eq(ret, 1);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_None, AC_Total, 42);
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(showalert_can_alert_on_limit_and_show_things)
{
	int ret;

	defaultcfg();
	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("something", "anything");
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("something", 100, 200);
	ck_assert_int_eq(ret, 1);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Hour, AC_Total, 250);
	ck_assert_int_eq(ret, 1);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Day, AC_Total, 250);
	ck_assert_int_eq(ret, 1);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Month, AC_Total, 250);
	ck_assert_int_eq(ret, 1);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_Total, 250);
	ck_assert_int_eq(ret, 1);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(showalert_limit_and_conditions_matter)
{
	int ret;

	defaultcfg();
	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic("something", 100, 200);
	ck_assert_int_eq(ret, 1);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_Total, 100);
	ck_assert_int_eq(ret, 1);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_Total, 280);
	ck_assert_int_eq(ret, 1);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_Total, 350);
	ck_assert_int_eq(ret, 0);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_Total, 300);
	ck_assert_int_eq(ret, 0);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_RX, 80);
	ck_assert_int_eq(ret, 1);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_RX, 150);
	ck_assert_int_eq(ret, 0);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_TX, 180);
	ck_assert_int_eq(ret, 1);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_TX, 250);
	ck_assert_int_eq(ret, 0);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_TX, 350);
	ck_assert_int_eq(ret, 0);

	ret = showalert("something", AO_Always_Output, AE_Exit_1_On_Limit, AT_Year, AC_TX, 200);
	ck_assert_int_eq(ret, 0);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

void add_database_tests(Suite *s)
{
	TCase *tc_db = tcase_create("Database");
	tcase_add_checked_fixture(tc_db, setup, teardown);
	tcase_add_unchecked_fixture(tc_db, setup, teardown);
	tcase_add_test(tc_db, initdb_activates_database);
	tcase_add_test(tc_db, readdb_with_empty_file);
	tcase_add_test(tc_db, readdb_with_empty_file_and_backup);
	tcase_add_test(tc_db, readdb_with_nonexisting_file);
	tcase_add_test(tc_db, readdb_with_existing_dbfile);
	tcase_add_test(tc_db, readdb_with_existing_dbfile_and_max_name_length);
	tcase_add_test(tc_db, readdb_with_existing_dbfile_with_rename);
	tcase_add_test(tc_db, readdb_with_existing_dbfile_and_over_max_name_length);
	tcase_add_test(tc_db, validatedb_with_initdb);
	tcase_add_test(tc_db, validatedb_with_invalid_totals);
	tcase_add_test(tc_db, validatedb_with_top10_use);
	tcase_add_test(tc_db, database_outputs_do_not_crash);
	tcase_add_exit_test(tc_db, database_showdb_exists_for_invalid_interface, 1);
	tcase_add_exit_test(tc_db, database_showjson_exists_for_invalid_interface, 1);
	tcase_add_exit_test(tc_db, database_showxml_exists_for_invalid_interface, 1);
	tcase_add_test(tc_db, database_outputs_do_not_crash_without_traffic);
	tcase_add_test(tc_db, database_outputs_do_not_crash_without_data);
	tcase_add_test(tc_db, database_outputs_do_not_crash_without_data_if_totals_are_wrong);
	tcase_add_test(tc_db, showbar_with_zero_len_is_nothing);
	tcase_add_test(tc_db, showbar_with_zero_max);
	tcase_add_test(tc_db, showbar_with_big_max_and_small_numbers);
	tcase_add_test(tc_db, showbar_with_all_rx);
	tcase_add_test(tc_db, showbar_with_all_tx);
	tcase_add_test(tc_db, showbar_with_half_and_half);
	tcase_add_test(tc_db, showbar_with_one_tenth);
	tcase_add_test(tc_db, showbar_with_small_rx_shows_all_tx);
	tcase_add_test(tc_db, showbar_with_max_smaller_than_real_max);
	tcase_add_test(tc_db, showbar_with_half_and_half_of_half);
	tcase_add_test(tc_db, importlegacydb_does_not_overwrite_existing_interface_data);
	tcase_add_test(tc_db, importlegacydb_can_detect_when_database_read_fails);
	tcase_add_test(tc_db, importlegacydb_can_import_legacy_database);
	tcase_add_test(tc_db, showalert_shows_nothing_with_none_type);
	tcase_add_test(tc_db, showalert_can_alert_on_limit_and_show_things);
	tcase_add_test(tc_db, showalert_limit_and_conditions_matter);
	suite_add_tcase(s, tc_db);
}

int writedb(DATA *data, const char *iface, const char *dirname, int newdb)
{
	FILE *testdb;
	char file[512], backup[512];

	snprintf(file, 512, "%s/%s", dirname, iface);
	snprintf(backup, 512, "%s/.%s", dirname, iface);

	/* try to make backup of old data if this isn't a new database */
	if (!newdb && !backupdb(file, backup)) {
		snprintf(errorstring, 1024, "Unable to create database backup \"%s\".", backup);
		printe(PT_Error);
		return 0;
	}

	/* make sure version stays correct */
	data->version = LEGACYDBVERSION;

	if ((testdb = fopen(file, "w")) == NULL) {
		snprintf(errorstring, 1024, "Unable to open database \"%s\" for writing: %s", file, strerror(errno));
		printe(PT_Error);
		return 0;
	}

	/* update timestamp when not merging */
	if (newdb != 2) {
		data->lastupdated = time(NULL);
	}

	if (fwrite(data, sizeof(DATA), 1, testdb) == 0) {
		snprintf(errorstring, 1024, "Unable to write database \"%s\": %s", file, strerror(errno));
		printe(PT_Error);
		fclose(testdb);
		return 0;
	} else {
		if (debug) {
			printf("db: Database \"%s\" saved.\n", file);
		}
		fclose(testdb);
		if ((newdb) && (noexit == 0)) {
			snprintf(errorstring, 1024, "-> A new database has been created.");
			printe(PT_Info);
		}
	}

	return 1;
}

int backupdb(const char *current, const char *backup)
{
	FILE *bf;
	int c, b, bytes;
	char buffer[512];

	/* from */
	if ((c = open(current, O_RDONLY)) == -1) {
		return 0;
	}

	/* to, fopen() in order to get file mode bits correctly */
	if ((bf = fopen(backup, "w")) == NULL) {
		close(c);
		return 0;
	}
	b = fileno(bf);

	/* copy data */
	while ((bytes = (int)read(c, buffer, sizeof(buffer))) > 0) {
		if (write(b, buffer, (size_t)bytes) < 0) {
			close(c);
			fclose(bf);
			return 0;
		}
	}

	close(c);
	fclose(bf);

	return 1;
}
