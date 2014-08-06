#include "vnstat_tests.h"
#include "common_tests.h"
#include "database_tests.h"
#include "config_tests.h"
#include "ifinfo_tests.h"
#include "misc_tests.h"
#include "daemon_tests.h"
#include "common.h"

int main(void)
{
	int number_failed = 0;
	debug = 0;

	Suite *s = test_suite();
	SRunner *sr = srunner_create(s);
	srunner_set_log(sr, "test.log");
	srunner_set_xml(sr, "test.xml");
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	if (number_failed == 0) {
		remove_directory(TESTDIR);
	}

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
	add_daemon_tests(s);

	return s;
}

void suppress_output(void)
{
	fclose(stdout);
}

void disable_logprints(void)
{
	noexit = 2;
	cfg.uselogging = 0;
}

int clean_testdbdir(void)
{
	struct stat statbuf;

	create_testdir();

	if (stat(TESTDBDIR, &statbuf)!=0) {
		if (errno==ENOENT) {
			if (mkdir(TESTDBDIR, 0755)==0) {
				return 1;
			}
		}
		ck_abort_msg("error \"%s\" while creating directory \"%s\"", strerror(errno), TESTDBDIR);
	}

	if (!remove_directory(TESTDBDIR)) {
		ck_abort_msg("error \"%s\" while removing directory \"%s\", please remove it manually", strerror(errno), TESTDBDIR);
	}

	if (mkdir(TESTDBDIR, 0755)!=0) {
		ck_abort_msg("error \"%s\" while creating directory \"%s\"", strerror(errno), TESTDBDIR);
	}

	return 1;
}

int create_testdir(void)
{
	struct stat statbuf;

	if (stat(TESTDIR, &statbuf)!=0) {
		if (errno==ENOENT) {
			if (mkdir(TESTDIR, 0755)==0) {
				return 1;
			}
		}
		ck_abort_msg("error \"%s\" while creating directory \"%s\"", strerror(errno), TESTDIR);
	}

	return 1;
}

int remove_directory(const char *directory)
{
	DIR *dir = NULL;
	struct dirent *di = NULL;
	char entryname[512];

	if ((dir=opendir(directory))==NULL) {
		if (errno==ENOENT) {
			return 1;
		} else {
			return 0;
		}
	}

	while ((di=readdir(dir))) {
		switch (di->d_type) {
			case DT_REG:
				snprintf(entryname, 512, "%s/%s", directory, di->d_name);
				if (unlink(entryname)!=0) {
					return 0;
				}
				break;
			case DT_DIR:
				if (strlen(di->d_name)>2) {
					snprintf(entryname, 512, "%s/%s", directory, di->d_name);
					if (!remove_directory(entryname)) {
						return 0;
					}
				}
				break;
			default:
				return 0;
		}
	}
	closedir(dir);
	if (rmdir(directory)!=0) {
		return 0;
	}

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

int fake_proc_net_dev(const char *mode, const char *iface, const int rx, const int tx, const int rxp, const int txp)
{
	FILE *devfp;
	char filename[512];
	struct stat statbuf;

	if (strcmp(mode, "w") != 0  && strcmp(mode, "a") != 0) {
		ck_abort_msg("error: only w and a modes are supported");
	}

	create_testdir();

	if (stat(TESTPROCDIR, &statbuf)!=0) {
		if (errno==ENOENT) {
			if (mkdir(TESTPROCDIR, 0755) != 0) {
				ck_abort_msg("error \"%s\" while creating directory \"%s\"", strerror(errno), TESTPROCDIR);
			}
		} else {
			ck_abort_msg("error \"%s\" while creating directory \"%s\"", strerror(errno), TESTPROCDIR);
		}
	}

	snprintf(filename, 512, "%s/dev", TESTPROCDIR);
	if ((devfp=fopen(filename, mode))==NULL) {
		ck_abort_msg("error \"%s\" while opening file \"%s\" for writing", strerror(errno), filename);
	}

	if (strcmp(mode, "w") == 0) {
		fprintf(devfp, "Inter-|   Receive                                                |  Transmit\n");
		fprintf(devfp, " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n");
		fprintf(devfp, "    lo:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0\n");
	}
	fprintf(devfp, "%6s: %7d %7d    0    0    0     0          0         0  %7d %7d    0    0    0     0       0          0\n", iface, rx, rxp, tx, txp);

	fclose(devfp);

	return 1;
}
