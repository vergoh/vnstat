#include "common.h"
#include "vnstat_tests.h"
#include "percentile_tests.h"
#include "percentile.h"

START_TEST(compare_uint64_t_can_compare)
{
	uint64_t a = 42, b = 42;

	ck_assert_int_eq(compare_uint64_t(&a, &b), 0);
	a += 10;
	ck_assert_int_eq(compare_uint64_t(&a, &b), 1);
	b += 20;
	ck_assert_int_eq(compare_uint64_t(&a, &b), -1);
	a += 10;
	ck_assert_int_eq(compare_uint64_t(&a, &b), 0);
	a = 0;
	b = 0;
	ck_assert_int_eq(compare_uint64_t(&a, &b), 0);
}
END_TEST

START_TEST(compare_uint64_t_can_be_used_with_qsort)
{
	uint64_t l[5] = {123, 2, 0, 3, 1};

	qsort((void *)l, 5, sizeof(uint64_t), compare_uint64_t);

	ck_assert_int_eq(l[0], 0);
	ck_assert_int_eq(l[1], 1);
	ck_assert_int_eq(l[2], 2);
	ck_assert_int_eq(l[3], 3);
	ck_assert_int_eq(l[4], 123);
}
END_TEST

void add_percentile_tests(Suite *s)
{
	TCase *tc_percentile = tcase_create("Percentile");
	tcase_add_checked_fixture(tc_percentile, setup, teardown);
	tcase_add_unchecked_fixture(tc_percentile, setup, teardown);
	tcase_add_test(tc_percentile, compare_uint64_t_can_compare);
	tcase_add_test(tc_percentile, compare_uint64_t_can_be_used_with_qsort);
	suite_add_tcase(s, tc_percentile);
}
