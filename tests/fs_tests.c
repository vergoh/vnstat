#include "vnstat_tests.h"
#include "fs_tests.h"
#include "common.h"
#include "cfg.h"
#include "fs.h"

START_TEST(fileexists_with_no_file)
{
	char testfile[512];
	defaultcfg();

	snprintf(testfile, 512, "%s/no_file", TESTDIR);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(fileexists(""), 0);
	ck_assert_int_eq(fileexists(testfile), 0);
}
END_TEST

START_TEST(fileexists_with_file)
{
	char testfile[512];
	defaultcfg();

	snprintf(testfile, 512, "%s/dummy_file", TESTDBDIR);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(create_zerosize_dbfile("dummy_file"), 1);
	ck_assert_int_eq(fileexists(testfile), 1);
}
END_TEST

START_TEST(direxists_with_no_dir)
{
	defaultcfg();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(direxists(""), 0);
	ck_assert_int_eq(direxists(TESTDIR), 0);
}
END_TEST

START_TEST(direxists_with_dir)
{
	defaultcfg();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(clean_testdbdir(), 1);
	ck_assert_int_eq(direxists(TESTDIR), 1);
	ck_assert_int_eq(direxists(TESTDBDIR), 1);
}
END_TEST

START_TEST(mkpath_with_no_dir)
{
	defaultcfg();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(mkpath("", 0775), 0);
}
END_TEST

START_TEST(mkpath_with_dir)
{
	defaultcfg();
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(direxists(TESTDIR), 0);
	ck_assert_int_eq(direxists(TESTDBDIR), 0);
	ck_assert_int_eq(mkpath(TESTDIR, 0775), 1);
	ck_assert_int_eq(direxists(TESTDIR), 1);
	ck_assert_int_eq(direxists(TESTDBDIR), 0);
	ck_assert_int_eq(mkpath(TESTDBDIR, 0775), 1);
	ck_assert_int_eq(direxists(TESTDBDIR), 1);
	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(direxists(TESTDBDIR), 0);
	ck_assert_int_eq(mkpath(TESTDBDIR, 0775), 1);
	ck_assert_int_eq(direxists(TESTDBDIR), 1);
}
END_TEST

START_TEST(preparevnstatdir_with_no_vnstat)
{
	char testdir[512], testpath[512];
	defaultcfg();
	cfg.updatefileowner = 0;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(direxists(TESTDIR), 0);
	snprintf(testdir, 512, "%s/here/be/dragons", TESTDIR);
	snprintf(testpath, 512, "%s/or_something.txt", testdir);
	preparevnstatdir(testpath, "user", "group");
	ck_assert_int_eq(direxists(TESTDIR), 0);
	ck_assert_int_eq(direxists(testdir), 0);

	snprintf(testdir, 512, "%s/here/be/vnstat/dragons", TESTDIR);
	snprintf(testpath, 512, "%s/or_something.txt", testdir);
	preparevnstatdir(testpath, "user", "group");
	ck_assert_int_eq(direxists(TESTDIR), 0);
	ck_assert_int_eq(direxists(testdir), 0);

	snprintf(testdir, 512, "%s/here/be/vnstati", TESTDIR);
	snprintf(testpath, 512, "%s/or_something.txt", testdir);
	preparevnstatdir(testpath, "user", "group");
	ck_assert_int_eq(direxists(TESTDIR), 0);
	ck_assert_int_eq(direxists(testdir), 0);
}
END_TEST

START_TEST(preparevnstatdir_with_vnstat)
{
	char testdir[512], testpath[512];
	defaultcfg();
	cfg.updatefileowner = 0;

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(direxists(TESTDIR), 0);
	snprintf(testdir, 512, "%s/here/be/vnstat", TESTDIR);
	snprintf(testpath, 512, "%s/or_something.txt", testdir);
	preparevnstatdir(testpath, "user", "group");
	ck_assert_int_eq(direxists(TESTDIR), 1);
	ck_assert_int_eq(direxists(testdir), 1);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	ck_assert_int_eq(direxists(TESTDIR), 0);
	snprintf(testdir, 512, "%s/here/be/vnstatd", TESTDIR);
	snprintf(testpath, 512, "%s/or_something.txt", testdir);
	preparevnstatdir(testpath, "user", "group");
	ck_assert_int_eq(direxists(TESTDIR), 1);
	ck_assert_int_eq(direxists(testdir), 1);
}
END_TEST

void add_fs_tests(Suite *s)
{
	TCase *tc_fs = tcase_create("FS");
	tcase_add_test(tc_fs, fileexists_with_no_file);
	tcase_add_test(tc_fs, fileexists_with_file);
	tcase_add_test(tc_fs, direxists_with_no_dir);
	tcase_add_test(tc_fs, direxists_with_dir);
	tcase_add_test(tc_fs, mkpath_with_no_dir);
	tcase_add_test(tc_fs, mkpath_with_dir);
	tcase_add_test(tc_fs, preparevnstatdir_with_no_vnstat);
	tcase_add_test(tc_fs, preparevnstatdir_with_vnstat);
	suite_add_tcase(s, tc_fs);
}
