#include "common.h"
#include "vnstat_tests.h"
#include "database_tests.h"
#include "dbaccess.h"
#include "ifinfo.h"
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

START_TEST(removedb_with_existing_files)
{
	int ret;

	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("ethall"), 1);
	ck_assert_int_eq(create_zerosize_dbfile(".ethall"), 1);

	ck_assert_int_eq(check_dbfile_exists("ethall", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".ethall", 0), 1);

	/* variable needed due to bug in old versions of check
	   framework: http://sourceforge.net/p/check/bugs/71/ */
	ret = removedb("ethall", TESTDBDIR);
	ck_assert_int_eq(ret, 1);

	ck_assert_int_eq(check_dbfile_exists("ethall", 0), 0);
	ck_assert_int_eq(check_dbfile_exists(".ethall", 0), 0);
}
END_TEST

START_TEST(removedb_with_nonexisting_file)
{
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("ethall"), 1);
	ck_assert_int_eq(create_zerosize_dbfile(".ethall"), 1);

	ck_assert_int_eq(check_dbfile_exists("ethall", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".ethall", 0), 1);

	ck_assert_int_eq(removedb("ethnone", TESTDBDIR), 0);

	ck_assert_int_eq(check_dbfile_exists("ethall", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".ethall", 0), 1);
}
END_TEST

START_TEST(readdb_with_empty_file)
{
	DATA data;
	disable_logprints();
	cfg.flock = 1;
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("existingdb"), 1);
	ck_assert_int_eq(readdb(&data, "existingdb", TESTDBDIR, 0), -1);
}
END_TEST

START_TEST(readdb_with_empty_file_and_backup)
{
	DATA data;
	disable_logprints();
	cfg.flock = 1;
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
	cfg.flock = 1;
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
	cfg.flock = 1;
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
	cfg.flock = 1;
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
	cfg.flock = 1;
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
	cfg.flock = 1;
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

	defaultcfg();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	for (i=1; i<100; i++) {
		ret = db_addtraffic_dated("something", i*1234, i*2345, i*85000);
		ck_assert_int_eq(ret, 1);
	}

	suppress_output();

	showdb("something", 0);
	showdb("something", 1);
	showdb("something", 2);
	showdb("something", 3);
	showdb("something", 4);
	showdb("something", 5);
	showdb("something", 6);
	showdb("something", 7);
	showdb("something", 8);
	showdb("something", 9);

	xmlheader();
	showxml("something", 'd');
	showxml("something", 'm');
	showxml("something", 't');
	showxml("something", 'h');
	showxml("something", 'a');
	xmlfooter();

	jsonheader();
	showjson("something", 0, 'd');
	showjson("something", 0, 'm');
	showjson("something", 0, 't');
	showjson("something", 0, 'h');
	showjson("something", 0, 'a');
	jsonfooter();
}
END_TEST

/* TODO: rewrite */
/*
START_TEST(database_outputs_do_not_crash_without_data)
{
	int i;

	defaultcfg();
	strcpy(data.interface, "something");
	strcpy(data.nick, "nothing");
	data.totalrx = 1;
	data.totaltx = 2;
	data.currx = 3;
	data.curtx = 4;
	data.totalrxk = 5;
	data.totaltxk = 6;
	data.btime = 7;

	for (i=0; i<30; i++) {
		data.day[i].date = i+1;
		data.day[i].used = 1;
	}

	for (i=0; i<10; i++) {
		data.top10[i].date = i+1;
		data.top10[i].used = 1;
	}

	for (i=0; i<12; i++) {
		data.month[i].month = i+1;
		data.month[i].used = 1;
	}

	for (i=0; i<24; i++) {
		data.hour[i].date = i+1;
	}

	suppress_output();

	showdb(0);
	showdb(1);
	showdb(2);
	showdb(3);
	showdb(4);
	showdb(5);
	showdb(6);
	showdb(7);
	showdb(8);
	showdb(9);

	xmlheader();
	showxml('d');
	showxml('m');
	showxml('t');
	showxml('h');
	showxml('a');
	xmlfooter();

	jsonheader();
	showjson(0, 'd');
	showjson(0, 'm');
	showjson(0, 't');
	showjson(0, 'h');
	showjson(0, 'a');
	jsonfooter();
}
END_TEST

START_TEST(database_outputs_do_not_crash_with_near_empty_database)
{
	defaultcfg();
	strcpy(data.interface, "something");
	strcpy(data.nick, "nothing");
	data.totalrx = 1;
	data.totaltx = 2;
	data.currx = 3;
	data.curtx = 4;
	data.totalrxk = 5;
	data.totaltxk = 6;
	data.btime = 7;

	suppress_output();

	showdb(0);
	showdb(1);
	showdb(2);
	showdb(3);
	showdb(4);
	showdb(5);
	showdb(6);
	showdb(7);
	showdb(8);
	showdb(9);

	xmlheader();
	showxml('d');
	showxml('m');
	showxml('t');
	showxml('h');
	showxml('a');
	xmlfooter();

	jsonheader();
	showjson(0, 'd');
	showjson(0, 'm');
	showjson(0, 't');
	showjson(0, 'h');
	showjson(0, 'a');
	jsonfooter();
}
END_TEST
*/

START_TEST(showbar_with_zero_len_is_nothing)
{
	int len;
	suppress_output();
	len = showbar(1, 2, 3, 0);
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

	defaultcfg();
	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(1, 0, 1, 10);
	ck_assert_int_eq(len, 10);
	fflush(stdout);

	len = read(pipe, buffer, 512);
	ck_assert_str_eq(buffer, "  rrrrrrrrrr");
}
END_TEST

START_TEST(showbar_with_all_tx)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	defaultcfg();
	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(0, 1, 1, 10);
	ck_assert_int_eq(len, 10);
	fflush(stdout);

	len = read(pipe, buffer, 512);
	ck_assert_str_eq(buffer, "  tttttttttt");
}
END_TEST

START_TEST(showbar_with_half_and_half)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	defaultcfg();
	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(1, 1, 2, 10);
	ck_assert_int_eq(len, 10);
	fflush(stdout);

	len = read(pipe, buffer, 512);
	ck_assert_str_eq(buffer, "  rrrrrttttt");
}
END_TEST

START_TEST(showbar_with_one_tenth)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	defaultcfg();
	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(1, 9, 10, 10);
	ck_assert_int_eq(len, 10);
	fflush(stdout);

	len = read(pipe, buffer, 512);
	ck_assert_str_eq(buffer, "  rttttttttt");
}
END_TEST

START_TEST(showbar_with_small_rx_shows_all_tx)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	defaultcfg();
	cfg.rxchar[0] = 'r';
	cfg.txchar[0] = 't';
	pipe = pipe_output();
	len = showbar(1, 1000, 1001, 10);
	ck_assert_int_eq(len, 10);
	fflush(stdout);

	len = read(pipe, buffer, 512);
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

void add_database_tests(Suite *s)
{
	TCase *tc_db = tcase_create("Database");
	tcase_add_test(tc_db, initdb_activates_database);
	tcase_add_test(tc_db, removedb_with_existing_files);
	tcase_add_test(tc_db, removedb_with_nonexisting_file);
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
	/* TODO: rewrite */
	/*tcase_add_test(tc_db, database_outputs_do_not_crash_without_data);
	tcase_add_test(tc_db, database_outputs_do_not_crash_with_near_empty_database);*/
	tcase_add_test(tc_db, showbar_with_zero_len_is_nothing);
	tcase_add_test(tc_db, showbar_with_big_max_and_small_numbers);
	tcase_add_test(tc_db, showbar_with_all_rx);
	tcase_add_test(tc_db, showbar_with_all_tx);
	tcase_add_test(tc_db, showbar_with_half_and_half);
	tcase_add_test(tc_db, showbar_with_one_tenth);
	tcase_add_test(tc_db, showbar_with_small_rx_shows_all_tx);
	tcase_add_test(tc_db, showbar_with_max_smaller_than_real_max);
	suite_add_tcase(s, tc_db);
}

int writedb(DATA *data, const char *iface, const char *dirname, int newdb)
{
	FILE *db;
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
	data->version=LEGACYDBVERSION;

	if ((db=fopen(file,"w"))==NULL) {
		snprintf(errorstring, 1024, "Unable to open database \"%s\" for writing: %s", file, strerror(errno));
		printe(PT_Error);
		return 0;
	}

	/* lock file */
	if (!lockdb(fileno(db), 1)) {
		fclose(db);
		return 0;
	}

	/* update timestamp when not merging */
	if (newdb!=2) {
		data->lastupdated=time(NULL);
	}

	if (fwrite(data,sizeof(DATA),1,db)==0) {
		snprintf(errorstring, 1024, "Unable to write database \"%s\": %s", file, strerror(errno));
		printe(PT_Error);
		fclose(db);
		return 0;
	} else {
		if (debug) {
			printf("db: Database \"%s\" saved.\n", file);
		}
		fclose(db);
		if ((newdb) && (noexit==0)) {
			snprintf(errorstring, 1024, "-> A new database has been created.");
			printe(PT_Info);
			matchdbownerwithdirowner(dirname);
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
	while((bytes = (int)read(c, buffer, sizeof(buffer))) > 0) {
		if (write(b, buffer, bytes) < 0) {
			close(c);
			fclose(bf);
			return 0;
		}
	}

	close(c);
	fclose(bf);

	return 1;
}
