#ifndef VNSTAT_TESTS_H
#define VNSTAT_TESTS_H

#include <check.h>

Suite *test_suite(void);
void suppress_output(void);
void disable_logprints(void);
int clean_testdbdir(void);
int create_testdir(void);
int create_directory(const char *directory);
int remove_directory(const char *directory);
int create_zerosize_dbfile(const char *iface);
int check_dbfile_exists(const char *iface, const int minsize);
int fake_proc_net_dev(const char *mode, const char *iface, const int rx, const int tx, const int rxp, const int txp);
int fake_sys_class_net(const char *iface, const int rx, const int tx, const int rxp, const int txp, const int speed);

#define TESTDIR            "testdir"
#define TESTDBDIR          "testdir/database"
#define TESTPROCDIR        "testdir/proc"
#define TESTSYSCLASSNETDIR "testdir/sysclassnet"

#if !defined(__linux__)
#define linuxonly return
#else
#define linuxonly
#endif

#if !defined(__linux__)
#define linuxonly_exit exit(1)
#else
#define linuxonly_exit
#endif

/* for compatibility with older check framework versions */
#ifndef ck_assert_int_ge
#define ck_assert_int_ge(X, Y) _ck_assert_int(X, >=, Y)
#endif

#ifndef ck_assert_int_gt
#define ck_assert_int_gt(X, Y) _ck_assert_int(X, >, Y)
#endif

#ifndef ck_assert_int_le
#define ck_assert_int_le(X, Y) _ck_assert_int(X, <=, Y)
#endif

#ifndef ck_assert_int_lt
#define ck_assert_int_lt(X, Y) _ck_assert_int(X, <, Y)
#endif

#endif
