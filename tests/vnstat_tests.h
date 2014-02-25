#ifndef VNSTAT_TESTS_H
#define VNSTAT_TESTS_H

#include <stdio.h>
#include <check.h>

void suppress_output(void);

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
