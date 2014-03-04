#include "vnstat_tests.h"
#include "common_tests.h"
#include "database_tests.h"
#include "config_tests.h"
#include "ifinfo_tests.h"
#include "misc_tests.h"

int main(void)
{
	int number_failed = 0;

	Suite *s = test_suite();
	SRunner *sr = srunner_create(s);
	srunner_set_log(sr, "test.log");
	srunner_set_xml(sr, "test.xml");
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return number_failed;
}

Suite *test_suite(void)
{
	Suite *s = suite_create("vnStat");

	add_common_tests(s);
	add_database_tests(s);
	add_config_tests(s);
	add_ifinfo_tests(s);
	add_misc_tests(s);

	return s;
}

void suppress_output(void)
{
	fclose(stdout);
}

int clean_testdbdir(void)
{
	DIR *dir = NULL;
	struct dirent *di = NULL;
	char filename[512];

	if ((dir=opendir(TESTDBDIR))==NULL) {
		if (errno==ENOENT) {
			if (mkdir(TESTDBDIR, 0755)==0) {
				return 1;
			}
			ck_abort_msg("error \"%s\" while creating directory \"%s\", please remove it manually", strerror(errno), TESTDBDIR);
		}
		ck_abort_msg("error \"%s\" while handling directory \"%s\", please remove it manually", strerror(errno), TESTDBDIR);
	}

	while ((di=readdir(dir))) {
		if (di->d_type==DT_REG) {
			snprintf(filename, 512, "%s/%s", TESTDBDIR, di->d_name);
			if (unlink(filename)!=0) {
				ck_abort_msg("error \"%s: %s\" while cleaning directory \"%s\", please remove it manually", filename, strerror(errno), TESTDBDIR);
			}
		}
	}

	closedir(dir);
	return 1;
}

int create_zerosize_dbfile(const char *iface)
{
	FILE *fp;
	char filename[512];

	snprintf(filename, 512, "%s/%s", TESTDBDIR, iface);
	if ((fp=fopen(filename, "w"))==NULL) {
		ck_abort_msg("error \"%s\" while opening file \"%s\" for writing", strerror(errno), filename);
	}
	fclose(fp);

	return 1;
}

int check_dbfile_exists(const char *iface, const int minsize)
{
	struct stat statbuf;
	char filename[512];

	snprintf(filename, 512, "%s/%s", TESTDBDIR, iface);
	if (stat(filename, &statbuf)!=0) {
		if (errno==ENOENT) {
			return 0;
		}
		ck_abort_msg("error \"%s\" while inspecting file \"%s\"", strerror(errno), filename);
	}

	if (statbuf.st_size < minsize) {
		ck_abort_msg("file \"%s\" is smaller (%d) then given minimum %d", filename, (int)statbuf.st_size, minsize);
	}

	return 1;
}
