#ifndef VNSTAT_TESTS_H
#define VNSTAT_TESTS_H

#include <check.h>

Suite *test_suite(void);
void suppress_output(void);
void disable_logprints(void);
int clean_testdbdir(void);
int create_zerosize_dbfile(const char *iface);
int check_dbfile_exists(const char *iface, const int minsize);

#define TESTDBDIR "testdbdir"

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
