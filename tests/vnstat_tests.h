#ifndef VNSTAT_TESTS_H
#define VNSTAT_TESTS_H

#include <check.h>

extern int output_suppressed;

Suite *test_suite(void);
void verify_fork_status(void);
void setup(void);
void teardown(void);
void suppress_output(void);
void restore_output(void);
int pipe_output(void);
void disable_logprints(void);
int clean_testdbdir(void);
int create_testdir(void);
int create_directory(const char *directory);
int remove_directory(const char *directory);
int create_zerosize_dbfile(const char *iface);
int check_dbfile_exists(const char *iface, const int minsize);
int fake_proc_net_dev(const char *mode, const char *iface, const int rx, const int tx, const int rxp, const int txp);
int fake_sys_class_net(const char *iface, const int rx, const int tx, const int rxp, const int txp, const int speed);
uint64_t get_timestamp(const int year, const int month, const int day, const int hour, const int minute);

#ifndef TESTDIR
#define TESTDIR "testdir"
#endif

#define TESTDBDIR          TESTDIR"/database"
#define TESTPROCDIR        TESTDIR"/proc"
#define TESTSYSCLASSNETDIR TESTDIR"/sysclassnet"

#if !defined(CFGFILE)
#define CFGFILE            "tests/vnstat.conf"
#endif

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

#ifndef _ck_assert_ptr
#define _ck_assert_ptr(X, OP, Y) do { \
  void* _ck_x = (X); \
  void* _ck_y = (Y); \
  ck_assert_msg(_ck_x OP _ck_y, "Assertion '"#X#OP#Y"' failed: "#X"==%p, "#Y"==%p", _ck_x, _ck_y); \
} while (0)
#define ck_assert_ptr_eq(X, Y) _ck_assert_ptr(X, ==, Y)
#define ck_assert_ptr_ne(X, Y) _ck_assert_ptr(X, !=, Y)
#endif

#endif
