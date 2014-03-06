#include "vnstat_tests.h"
#include "database_tests.h"
#include "common.h"
#include "dbaccess.h"
#include "dbcache.h"

START_TEST(initdb_activates_database)
{
	initdb();
	ck_assert_int_eq(data.active, 1);
}
END_TEST

START_TEST(cleanhours_really_cleans_hours)
{
	int i;
	initdb();
	for (i=0; i<24; i++) {
		ck_assert_int_eq(data.hour[i].rx, 0);
		ck_assert_int_eq(data.hour[i].tx, 0);
	}
	for (i=0; i<24; i++) {
		data.hour[i].rx = data.hour[i].tx = i+1;
		data.hour[i].date = 1;
	}
	cleanhours();
	for (i=0; i<24; i++) {
		ck_assert_int_eq(data.hour[i].rx, 0);
		ck_assert_int_eq(data.hour[i].tx, 0);
	}
}
END_TEST

START_TEST(cleanhours_does_not_remove_current_data)
{
	int i;
	initdb();
	for (i=0; i<24; i++) {
		ck_assert_int_eq(data.hour[i].rx, 0);
		ck_assert_int_eq(data.hour[i].tx, 0);
	}
	for (i=0; i<24; i++) {
		data.hour[i].rx = data.hour[i].tx = i+1;
		data.hour[i].date = time(NULL);
	}
	cleanhours();
	for (i=0; i<24; i++) {
		ck_assert_int_ne(data.hour[i].rx, 0);
		ck_assert_int_ne(data.hour[i].tx, 0);
	}
}
END_TEST

START_TEST(rotatedays_really_rotates_days)
{
	int i;
	initdb();
	for (i=0; i<30; i++) {
		ck_assert_int_eq(data.day[i].rx, 0);
		ck_assert_int_eq(data.day[i].tx, 0);
		ck_assert_int_eq(data.day[i].rxk, 0);
		ck_assert_int_eq(data.day[i].txk, 0);
	}
	data.day[0].rx = data.day[0].tx = 1;
	data.day[0].rxk = data.day[0].txk = 1;
	data.day[0].date = 1;
	data.day[0].used = 1;

	rotatedays();

	ck_assert_int_eq(data.day[0].rx, 0);
	ck_assert_int_eq(data.day[0].tx, 0);
	ck_assert_int_eq(data.day[0].rxk, 0);
	ck_assert_int_eq(data.day[0].txk, 0);
	ck_assert_int_ne(data.day[0].date, 0);

	ck_assert_int_eq(data.day[1].rx, 1);
	ck_assert_int_eq(data.day[1].tx, 1);
	ck_assert_int_eq(data.day[1].rxk, 1);
	ck_assert_int_eq(data.day[1].txk, 1);
	ck_assert_int_eq(data.day[1].date, 1);
	ck_assert_int_eq(data.day[1].used, 1);
}
END_TEST

START_TEST(rotatedays_updates_top10)
{
	int i;
	initdb();

	for (i=0; i<10 ; i++) {
		ck_assert_int_eq(data.top10[i].rx, 0);
		ck_assert_int_eq(data.top10[i].tx, 0);
		ck_assert_int_eq(data.top10[i].rxk, 0);
		ck_assert_int_eq(data.top10[i].txk, 0);
	}

	data.day[0].rx = data.day[0].tx = 1;
	data.day[0].rxk = data.day[0].txk = 1;
	data.day[0].date = 1;
	data.day[0].used = 1;

	rotatedays();

	ck_assert_int_eq(data.top10[0].rx, 1);
	ck_assert_int_eq(data.top10[0].tx, 1);
	ck_assert_int_eq(data.top10[0].rxk, 1);
	ck_assert_int_eq(data.top10[0].txk, 1);
	for (i=1; i<10 ; i++) {
		ck_assert_int_eq(data.top10[i].rx, 0);
		ck_assert_int_eq(data.top10[i].tx, 0);
		ck_assert_int_eq(data.top10[i].rxk, 0);
		ck_assert_int_eq(data.top10[i].txk, 0);
	}

	data.day[0].rx = data.day[0].tx = 3;
	data.day[0].rxk = data.day[0].txk = 3;
	data.day[0].date = 3;
	data.day[0].used = 3;

	rotatedays();

	ck_assert_int_eq(data.top10[0].rx, 3);
	ck_assert_int_eq(data.top10[0].tx, 3);
	ck_assert_int_eq(data.top10[0].rxk, 3);
	ck_assert_int_eq(data.top10[0].txk, 3);
	ck_assert_int_eq(data.top10[1].rx, 1);
	ck_assert_int_eq(data.top10[1].tx, 1);
	ck_assert_int_eq(data.top10[1].rxk, 1);
	ck_assert_int_eq(data.top10[1].txk, 1);
	for (i=2; i<10 ; i++) {
		ck_assert_int_eq(data.top10[i].rx, 0);
		ck_assert_int_eq(data.top10[i].tx, 0);
		ck_assert_int_eq(data.top10[i].rxk, 0);
		ck_assert_int_eq(data.top10[i].txk, 0);
	}

	data.day[0].rx = data.day[0].tx = 2;
	data.day[0].rxk = data.day[0].txk = 2;
	data.day[0].date = 2;
	data.day[0].used = 2;

	rotatedays();

	ck_assert_int_eq(data.top10[0].rx, 3);
	ck_assert_int_eq(data.top10[0].tx, 3);
	ck_assert_int_eq(data.top10[0].rxk, 3);
	ck_assert_int_eq(data.top10[0].txk, 3);
	ck_assert_int_eq(data.top10[1].rx, 2);
	ck_assert_int_eq(data.top10[1].tx, 2);
	ck_assert_int_eq(data.top10[1].rxk, 2);
	ck_assert_int_eq(data.top10[1].txk, 2);
	ck_assert_int_eq(data.top10[2].rx, 1);
	ck_assert_int_eq(data.top10[2].tx, 1);
	ck_assert_int_eq(data.top10[2].rxk, 1);
	ck_assert_int_eq(data.top10[2].txk, 1);
	for (i=3; i<10 ; i++) {
		ck_assert_int_eq(data.top10[i].rx, 0);
		ck_assert_int_eq(data.top10[i].tx, 0);
		ck_assert_int_eq(data.top10[i].rxk, 0);
		ck_assert_int_eq(data.top10[i].txk, 0);
	}
}
END_TEST

START_TEST(rotatemonths_really_rotates_months)
{
	int i;
	initdb();
	for (i=0; i<12; i++) {
		ck_assert_int_eq(data.month[i].rx, 0);
		ck_assert_int_eq(data.month[i].tx, 0);
		ck_assert_int_eq(data.month[i].rxk, 0);
		ck_assert_int_eq(data.month[i].txk, 0);
	}
	data.month[0].rx = data.month[0].tx = 1;
	data.month[0].rxk = data.month[0].txk = 1;
	data.month[0].month = 1;
	data.month[0].used = 1;

	rotatemonths();

	ck_assert_int_eq(data.month[0].rx, 0);
	ck_assert_int_eq(data.month[0].tx, 0);
	ck_assert_int_eq(data.month[0].rxk, 0);
	ck_assert_int_eq(data.month[0].txk, 0);
	ck_assert_int_ne(data.month[0].month, 0);

	ck_assert_int_eq(data.month[1].rx, 1);
	ck_assert_int_eq(data.month[1].tx, 1);
	ck_assert_int_eq(data.month[1].rxk, 1);
	ck_assert_int_eq(data.month[1].txk, 1);
	ck_assert_int_eq(data.month[1].month, 1);
	ck_assert_int_eq(data.month[1].used, 1);
}
END_TEST

START_TEST(cachecount_when_empty)
{
	ck_assert_int_eq(cachecount(), 0);
	ck_assert_int_eq(cacheactivecount(), 0);
}
END_TEST

START_TEST(cacheadd_success)
{
	ck_assert_int_eq(cacheadd("name1", 0), 1);
	ck_assert_int_eq(cacheadd("name1", 1), 1);
	ck_assert_int_eq(cacheadd("name2", 1), 1);
	ck_assert_int_eq(cacheadd("name2", 1), 1);
}
END_TEST

START_TEST(cachecount_when_filled)
{
	ck_assert_int_eq(cachecount(), 0);
	ck_assert_int_eq(cacheactivecount(), 0);

	ck_assert_int_eq(cacheadd("name1", 0), 1);

	ck_assert_int_eq(cachecount(), 1);
	ck_assert_int_eq(cacheactivecount(), 1);

	ck_assert_int_eq(cacheadd("name2", 0), 1);

	ck_assert_int_eq(cachecount(), 2);
	ck_assert_int_eq(cacheactivecount(), 2);

	ck_assert_int_eq(cacheadd("name1", 0), 1);

	ck_assert_int_eq(cachecount(), 2);
	ck_assert_int_eq(cacheactivecount(), 2);
}
END_TEST

START_TEST(cacheupdate_when_empty)
{
	initdb();
	strcpy(data.interface, "name1");
	ck_assert_int_eq(cacheupdate(), 1);
	ck_assert_int_eq(cachecount(), 1);
	ck_assert_int_eq(cacheactivecount(), 1);
}
END_TEST

START_TEST(cacheupdate_when_filled)
{
	initdb();
	strcpy(data.interface, "name1");
	ck_assert_int_eq(cacheupdate(), 1);
	ck_assert_int_eq(cachecount(), 1);
	ck_assert_int_eq(cacheactivecount(), 1);

	strcpy(data.interface, "name2");
	ck_assert_int_eq(cacheupdate(), 1);
	ck_assert_int_eq(cachecount(), 2);
	ck_assert_int_eq(cacheactivecount(), 2);

	strcpy(data.interface, "name1");
	data.active = 0;
	ck_assert_int_eq(cacheupdate(), 1);
	ck_assert_int_eq(cachecount(), 2);
	ck_assert_int_eq(cacheactivecount(), 1);
}
END_TEST

START_TEST(cacheget_when_empty)
{
	ck_assert_int_eq(cacheget(NULL), 0);
}
END_TEST

START_TEST(cacheget_when_filled)
{
	initdb();
	ck_assert_int_eq(cachecount(), 0);
	strcpy(data.interface, "name1");
	ck_assert_int_eq(cacheupdate(), 1);
	ck_assert_int_eq(cachecount(), 1);
	strcpy(data.interface, "empty");
	ck_assert_int_eq(cacheget(dataptr), 1);
	ck_assert_str_eq(data.interface, "name1");
}
END_TEST

START_TEST(cacheremove_when_empty)
{
	ck_assert_int_eq(cacheremove("does_not_exist"), NULL);
}
END_TEST

START_TEST(cacheremove_when_filled)
{
	initdb();
	ck_assert_int_eq(cachecount(), 0);
	ck_assert_int_eq(cacheadd("name4", 0), 1);
	ck_assert_int_eq(cacheadd("name3", 0), 1);
	ck_assert_int_eq(cacheadd("name2", 0), 1);
	ck_assert_int_eq(cacheadd("name1", 0), 1);
	ck_assert_int_eq(cachecount(), 4);

	ck_assert(cacheremove("does_not_exist")==NULL);
	ck_assert_int_eq(cachecount(), 4);

	ck_assert(cacheremove("name1")!=NULL);
	ck_assert_int_eq(cachecount(), 3);

	ck_assert(cacheremove("name1")==NULL);
	ck_assert_int_eq(cachecount(), 3);

	ck_assert(cacheremove("name3")!=NULL);
	ck_assert_int_eq(cachecount(), 2);

	ck_assert(cacheremove("name4")==NULL);
	ck_assert_int_eq(cachecount(), 1);

	ck_assert(cacheremove("name2")==NULL);
	ck_assert_int_eq(cachecount(), 0);
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

START_TEST(cacheshow_empty)
{
	suppress_output();
	cacheshow();
}
END_TEST

START_TEST(cacheshow_filled)
{
	initdb();
	ck_assert_int_eq(cachecount(), 0);
	ck_assert_int_eq(cacheadd("name4", 0), 1);
	ck_assert_int_eq(cacheadd("name3", 0), 1);
	ck_assert_int_eq(cacheadd("name2", 0), 1);
	ck_assert_int_eq(cacheadd("name1", 0), 1);
	ck_assert_int_eq(cachecount(), 4);
	suppress_output();
	cacheshow();
}
END_TEST

START_TEST(cachestatus_empty)
{
	disable_logprints();
	cachestatus();
}
END_TEST

START_TEST(cachestatus_filled)
{
	initdb();
	disable_logprints();
	ck_assert_int_eq(cachecount(), 0);
	ck_assert_int_eq(cacheadd("name4", 0), 1);
	ck_assert_int_eq(cacheadd("name3", 0), 1);
	ck_assert_int_eq(cacheadd("name2", 0), 1);
	ck_assert_int_eq(cacheadd("name1", 0), 1);
	ck_assert_int_eq(cachecount(), 4);
	cachestatus();
}
END_TEST

START_TEST(cacheflush_flushes_cache)
{
	initdb();
	disable_logprints();
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("name1"), 1);
	ck_assert_int_eq(create_zerosize_dbfile("name2"), 1);
	ck_assert_int_eq(check_dbfile_exists("name1", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".name1", 0), 0);
	ck_assert_int_eq(check_dbfile_exists("name2", 0), 1);
	ck_assert_int_eq(check_dbfile_exists(".name2", 0), 0);

	ck_assert_int_eq(cachecount(), 0);
	strcpy(data.interface, "name1");
	ck_assert_int_eq(cacheupdate(), 1);
	strcpy(data.interface, "name2");
	ck_assert_int_eq(cacheupdate(), 1);
	ck_assert_int_eq(cacheadd("notfilled", 0), 1);
	ck_assert_int_eq(cachecount(), 3);
	ck_assert_int_eq(cacheactivecount(), 3);

	cacheflush(TESTDBDIR);

	ck_assert_int_eq(cachecount(), 0);
	ck_assert_int_eq(cacheactivecount(), 0);
	ck_assert_int_eq(check_dbfile_exists("name1", sizeof(DATA)), 1);
	ck_assert_int_eq(check_dbfile_exists(".name1", 0), 1);
	ck_assert_int_eq(check_dbfile_exists("name2", sizeof(DATA)), 1);
	ck_assert_int_eq(check_dbfile_exists(".name2", 0), 1);
	ck_assert_int_eq(check_dbfile_exists("notfilled", 0), 0);
	ck_assert_int_eq(check_dbfile_exists(".notfilled", 0), 0);
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

START_TEST(checkdb_finds_existing_file)
{
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("existingdb"), 1);
	ck_assert_int_eq(checkdb("existingdb", TESTDBDIR), 1);
}
END_TEST

START_TEST(checkdb_does_not_find_nonexisting_file)
{
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(checkdb("nonexistingdb", TESTDBDIR), 0);
}
END_TEST

START_TEST(readdb_with_empty_file)
{
	disable_logprints();
	cfg.flock = 1;
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("existingdb"), 1);
	ck_assert_int_eq(readdb("existingdb", TESTDBDIR), -1);
}
END_TEST

START_TEST(readdb_with_empty_file_and_backup)
{
	disable_logprints();
	cfg.flock = 1;
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("existingdb"), 1);
	ck_assert_int_eq(create_zerosize_dbfile(".existingdb"), 1);
	ck_assert_int_eq(readdb("existingdb", TESTDBDIR), -1);
}
END_TEST

START_TEST(readdb_with_nonexisting_file)
{
	disable_logprints();
	cfg.flock = 1;
	strcpy(data.interface, "none");
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(readdb("existingdb", TESTDBDIR), 1);
	ck_assert_str_eq(data.interface, "existingdb");
	ck_assert_str_eq(data.nick, "existingdb");
}
END_TEST

START_TEST(readdb_with_existing_dbfile)
{
	initdb();
	disable_logprints();
	cfg.flock = 1;
	strcpy(data.interface, "ethtest");
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(writedb("ethtest", TESTDBDIR, 1), 1);
	ck_assert_int_eq(check_dbfile_exists("ethtest", sizeof(DATA)), 1);

	strcpy(data.interface, "none");
	ck_assert_int_eq(readdb("ethtest", TESTDBDIR), 0);
	ck_assert_str_eq(data.interface, "ethtest");
}
END_TEST

START_TEST(readdb_with_existing_dbfile_with_rename)
{
	initdb();
	disable_logprints();
	cfg.flock = 1;
	strcpy(data.interface, "ethtest");
	strcpy(data.nick, "ethtest");
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(writedb("ethtest2", TESTDBDIR, 1), 1);
	ck_assert_int_eq(check_dbfile_exists("ethtest2", sizeof(DATA)), 1);

	strcpy(data.interface, "none");
	strcpy(data.nick, "none");
	ck_assert_int_eq(readdb("ethtest2", TESTDBDIR), 0);
	ck_assert_str_eq(data.interface, "ethtest2");
	ck_assert_str_eq(data.nick, "ethtest2");
}
END_TEST

START_TEST(cleartop10_clears_top10)
{
	int i;

	initdb();
	suppress_output();
	disable_logprints();
	cfg.flock = 1;
	strcpy(data.interface, "ethtest");
	strcpy(data.nick, "ethtest");
	for (i=0; i<10; i++) {
		data.top10[i].rx = 1;
		data.top10[i].tx = 1;
		data.top10[i].used = 1;
	}
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(writedb("ethtest", TESTDBDIR, 1), 1);

	cleartop10("ethtest", TESTDBDIR);

	ck_assert_int_eq(check_dbfile_exists("ethtest", sizeof(DATA)), 1);
	ck_assert_int_eq(check_dbfile_exists(".ethtest", sizeof(DATA)), 1);
	for (i=0; i<10; i++) {
		ck_assert_int_eq(data.top10[i].rx, 0);
		ck_assert_int_eq(data.top10[i].tx, 0);
		ck_assert_int_eq(data.top10[i].rxk, 0);
		ck_assert_int_eq(data.top10[i].txk, 0);
		ck_assert_int_eq(data.top10[i].used, 0);
	}
}
END_TEST

START_TEST(cleartop10_with_nonexisting_file)
{
	disable_logprints();
	ck_assert_int_eq(clean_testdbdir(), 1);
	cleartop10("ethtest", TESTDBDIR);
}
END_TEST

START_TEST(rebuilddbtotal_rebuilds_total)
{
	int i;

	initdb();
	suppress_output();
	disable_logprints();
	cfg.flock = 1;
	strcpy(data.interface, "ethtest");
	strcpy(data.nick, "ethtest");
	data.totalrx = 0;
	data.totaltx = 0;
	for (i=0; i<12; i++) {
		data.month[i].rx = 1;
		data.month[i].tx = 2;
		data.month[i].used = 1;
	}
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(writedb("ethtest", TESTDBDIR, 1), 1);

	rebuilddbtotal("ethtest", TESTDBDIR);

	ck_assert_int_eq(check_dbfile_exists("ethtest", sizeof(DATA)), 1);
	ck_assert_int_eq(check_dbfile_exists(".ethtest", sizeof(DATA)), 1);
	ck_assert_int_eq(data.totalrx, 12);
	ck_assert_int_eq(data.totaltx, 24);
}
END_TEST

START_TEST(rebuilddbtotal_with_nonexisting_file)
{
	disable_logprints();
	ck_assert_int_eq(clean_testdbdir(), 1);
	rebuilddbtotal("ethtest", TESTDBDIR);
}
END_TEST

START_TEST(validatedb_with_initdb)
{
	initdb();
	strcpy(data.interface, "ethtest");
	ck_assert_int_eq(validatedb(), 1);
}
END_TEST

START_TEST(validatedb_with_invalid_totals)
{
	initdb();
	strcpy(data.interface, "ethtest");
	data.day[0].rx++;
	ck_assert_int_eq(validatedb(), 0);
}
END_TEST

START_TEST(validatedb_with_top10_use)
{
	initdb();
	strcpy(data.interface, "ethtest");
	data.top10[0].used = 1;
	data.top10[1].used = 1;
	data.top10[2].used = 1;
	data.top10[5].used = 1;
	ck_assert_int_eq(validatedb(), 0);
}
END_TEST

void add_database_tests(Suite *s)
{
	/* Database test cases */
	TCase *tc_db = tcase_create("Database");
	tcase_add_test(tc_db, initdb_activates_database);
	tcase_add_test(tc_db, cleanhours_really_cleans_hours);
	tcase_add_test(tc_db, cleanhours_does_not_remove_current_data);
	tcase_add_test(tc_db, rotatedays_really_rotates_days);
	tcase_add_test(tc_db, rotatedays_updates_top10);
	tcase_add_test(tc_db, rotatemonths_really_rotates_months);
	tcase_add_test(tc_db, cachecount_when_empty);
	tcase_add_test(tc_db, cacheadd_success);
	tcase_add_test(tc_db, cachecount_when_filled);
	tcase_add_test(tc_db, cacheupdate_when_empty);
	tcase_add_test(tc_db, cacheupdate_when_filled);
	tcase_add_test(tc_db, cacheget_when_empty);
	tcase_add_test(tc_db, cacheget_when_filled);
	tcase_add_test(tc_db, cacheremove_when_empty);
	tcase_add_test(tc_db, cacheremove_when_filled);
	tcase_add_test(tc_db, simplehash_with_empty_strings);
	tcase_add_test(tc_db, simplehash_with_simple_strings);
	tcase_add_test(tc_db, cacheshow_empty);
	tcase_add_test(tc_db, cacheshow_filled);
	tcase_add_test(tc_db, cachestatus_empty);
	tcase_add_test(tc_db, cachestatus_filled);
	tcase_add_test(tc_db, cacheflush_flushes_cache);
	tcase_add_test(tc_db, removedb_with_existing_files);
	tcase_add_test(tc_db, removedb_with_nonexisting_file);
	tcase_add_test(tc_db, checkdb_finds_existing_file);
	tcase_add_test(tc_db, checkdb_does_not_find_nonexisting_file);
	tcase_add_test(tc_db, readdb_with_empty_file);
	tcase_add_test(tc_db, readdb_with_empty_file_and_backup);
	tcase_add_test(tc_db, readdb_with_nonexisting_file);
	tcase_add_test(tc_db, readdb_with_existing_dbfile);
	tcase_add_test(tc_db, readdb_with_existing_dbfile_with_rename);
	tcase_add_test(tc_db, cleartop10_clears_top10);
	tcase_add_exit_test(tc_db, cleartop10_with_nonexisting_file, 1);
	tcase_add_test(tc_db, rebuilddbtotal_rebuilds_total);
	tcase_add_exit_test(tc_db, rebuilddbtotal_with_nonexisting_file, 1);
	tcase_add_test(tc_db, validatedb_with_initdb);
	tcase_add_test(tc_db, validatedb_with_invalid_totals);
	tcase_add_test(tc_db, validatedb_with_top10_use);
	suite_add_tcase(s, tc_db);
}
