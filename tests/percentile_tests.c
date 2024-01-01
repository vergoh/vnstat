#include "common.h"
#include "vnstat_tests.h"
#include "percentile_tests.h"
#include "dbsql.h"
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

START_TEST(getpercentiledata_returns_with_no_wrong_interface)
{
	int ret;
	percentiledata pdata;

	pdata.userlimitbytespersecond = 1;
	pdata.countrxoveruserlimit = 2;
	pdata.counttxoveruserlimit = 3;
	pdata.countsumoveruserlimit = 4;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	suppress_output();

	ret = getpercentiledata(&pdata, "no_interface", 1234);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(pdata.userlimitbytespersecond, 1234);
	ck_assert_int_eq(pdata.countrxoveruserlimit, 0);
	ck_assert_int_eq(pdata.counttxoveruserlimit, 0);
	ck_assert_int_eq(pdata.countsumoveruserlimit, 0);
	ck_assert_str_eq(errorstring, "Failed to fetch month data for 95th percentile.");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(getpercentiledata_returns_with_no_data)
{
	int ret;
	percentiledata pdata;

	pdata.userlimitbytespersecond = 1;
	pdata.countrxoveruserlimit = 2;
	pdata.counttxoveruserlimit = 3;
	pdata.countsumoveruserlimit = 4;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("interface");
	ck_assert_int_eq(ret, 1);

	suppress_output();

	ret = getpercentiledata(&pdata, "interface", 1234);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(pdata.userlimitbytespersecond, 1234);
	ck_assert_int_eq(pdata.countrxoveruserlimit, 0);
	ck_assert_int_eq(pdata.counttxoveruserlimit, 0);
	ck_assert_int_eq(pdata.countsumoveruserlimit, 0);
	ck_assert_str_eq(errorstring, "No month data for 95th percentile available.");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(getpercentiledata_can_provide_data_with_one_entry)
{
	int ret;
	uint64_t entry;
	percentiledata pdata;

	entry = get_timestamp(2001, 1, 1, 0, 0);

	pdata.userlimitbytespersecond = 1;
	pdata.countrxoveruserlimit = 2;
	pdata.counttxoveruserlimit = 3;
	pdata.countsumoveruserlimit = 4;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("interface");
	ck_assert_int_eq(ret, 1);

	ret = db_addtraffic_dated("interface", 1, 2, entry);
	ck_assert_int_eq(ret, 1);

	ret = db_setupdated("interface", (time_t)entry);
	ck_assert_int_eq(ret, 1);

	suppress_output();

	ret = getpercentiledata(&pdata, "interface", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(pdata.userlimitbytespersecond, 0);
	ck_assert_int_eq(pdata.countrxoveruserlimit, 0);
	ck_assert_int_eq(pdata.counttxoveruserlimit, 0);
	ck_assert_int_eq(pdata.countsumoveruserlimit, 0);
	ck_assert_int_eq(pdata.count, 1);
	ck_assert_int_eq(pdata.countexpectation, 1);
	ck_assert_int_eq(pdata.rxpercentile, 1);
	ck_assert_int_eq(pdata.txpercentile, 2);
	ck_assert_int_eq(pdata.sumpercentile, 3);
	ck_assert_int_eq(pdata.maxrx, 1);
	ck_assert_int_eq(pdata.maxtx, 2);
	ck_assert_int_eq(pdata.max, 3);
	ck_assert_int_eq(pdata.minrx, 1);
	ck_assert_int_eq(pdata.mintx, 2);
	ck_assert_int_eq(pdata.min, 3);
	ck_assert_int_eq(pdata.sumrx, 1);
	ck_assert_int_eq(pdata.sumtx, 2);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(getpercentiledata_can_provide_data_with_many_entries)
{
	int ret, i;
	uint64_t entry;
	percentiledata pdata;

	entry = get_timestamp(2001, 1, 1, 0, 0);

	pdata.userlimitbytespersecond = 1;
	pdata.countrxoveruserlimit = 2;
	pdata.counttxoveruserlimit = 3;
	pdata.countsumoveruserlimit = 4;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("interface");
	ck_assert_int_eq(ret, 1);

	for (i = 0; i < 100; i++) {
		ret = db_addtraffic_dated("interface", i * 1, i * 2, entry + i * 300);
		ck_assert_int_eq(ret, 1);
	}

	ret = db_setupdated("interface", (time_t)(entry + i * 300));
	ck_assert_int_eq(ret, 1);

	suppress_output();

	ret = getpercentiledata(&pdata, "interface", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(pdata.userlimitbytespersecond, 0);
	ck_assert_int_eq(pdata.countrxoveruserlimit, 0);
	ck_assert_int_eq(pdata.counttxoveruserlimit, 0);
	ck_assert_int_eq(pdata.countsumoveruserlimit, 0);
	ck_assert_int_eq(pdata.count, 100);
	ck_assert_int_eq(pdata.countexpectation, 100);
	ck_assert_int_eq(pdata.rxpercentile, 94);
	ck_assert_int_eq(pdata.txpercentile, 188);
	ck_assert_int_eq(pdata.sumpercentile, 282);
	ck_assert_int_eq(pdata.maxrx, 99);
	ck_assert_int_eq(pdata.maxtx, 198);
	ck_assert_int_eq(pdata.max, 297);
	ck_assert_int_eq(pdata.minrx, 0);
	ck_assert_int_eq(pdata.mintx, 0);
	ck_assert_int_eq(pdata.min, 0);
	ck_assert_int_eq(pdata.sumrx, 4950);
	ck_assert_int_eq(pdata.sumtx, 9900);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(getpercentiledata_can_provide_data_with_many_entries_and_order_does_not_matter)
{
	int ret, i;
	uint64_t entry;
	percentiledata pdata;

	entry = get_timestamp(2001, 1, 1, 0, 0);

	pdata.userlimitbytespersecond = 1;
	pdata.countrxoveruserlimit = 2;
	pdata.counttxoveruserlimit = 3;
	pdata.countsumoveruserlimit = 4;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("interface");
	ck_assert_int_eq(ret, 1);

	for (i = 0; i < 100; i++) {
		ret = db_addtraffic_dated("interface", (99 - i) * 1, (99 - i) * 2, entry + i * 300);
		ck_assert_int_eq(ret, 1);
	}

	ret = db_setupdated("interface", (time_t)(entry + i * 300));
	ck_assert_int_eq(ret, 1);

	suppress_output();

	ret = getpercentiledata(&pdata, "interface", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(pdata.userlimitbytespersecond, 0);
	ck_assert_int_eq(pdata.countrxoveruserlimit, 0);
	ck_assert_int_eq(pdata.counttxoveruserlimit, 0);
	ck_assert_int_eq(pdata.countsumoveruserlimit, 0);
	ck_assert_int_eq(pdata.count, 100);
	ck_assert_int_eq(pdata.countexpectation, 100);
	ck_assert_int_eq(pdata.rxpercentile, 94);
	ck_assert_int_eq(pdata.txpercentile, 188);
	ck_assert_int_eq(pdata.sumpercentile, 282);
	ck_assert_int_eq(pdata.maxrx, 99);
	ck_assert_int_eq(pdata.maxtx, 198);
	ck_assert_int_eq(pdata.max, 297);
	ck_assert_int_eq(pdata.minrx, 0);
	ck_assert_int_eq(pdata.mintx, 0);
	ck_assert_int_eq(pdata.min, 0);
	ck_assert_int_eq(pdata.sumrx, 4950);
	ck_assert_int_eq(pdata.sumtx, 9900);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(getpercentiledata_can_check_limit)
{
	int ret, i;
	uint64_t entry;
	percentiledata pdata;

	entry = get_timestamp(2001, 1, 1, 0, 0);

	pdata.userlimitbytespersecond = 1;
	pdata.countrxoveruserlimit = 2;
	pdata.counttxoveruserlimit = 3;
	pdata.countsumoveruserlimit = 4;

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("interface");
	ck_assert_int_eq(ret, 1);

	for (i = 0; i < 100; i++) {
		ret = db_addtraffic_dated("interface", i * 300, i * 600, entry + i * 300);
		ck_assert_int_eq(ret, 1);
	}

	ret = db_setupdated("interface", (time_t)(entry + i * 300));
	ck_assert_int_eq(ret, 1);

	suppress_output();
	debug = 1;

	ret = getpercentiledata(&pdata, "interface", 20);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(pdata.userlimitbytespersecond, 20);
	ck_assert_int_eq(pdata.countrxoveruserlimit, 79);
	ck_assert_int_eq(pdata.counttxoveruserlimit, 89);
	ck_assert_int_eq(pdata.countsumoveruserlimit, 93);
	ck_assert_int_eq(pdata.count, 100);
	ck_assert_int_eq(pdata.countexpectation, 100);
	ck_assert_int_eq(pdata.rxpercentile, 94 * 300);
	ck_assert_int_eq(pdata.txpercentile, 94 * 600);
	ck_assert_int_eq(pdata.sumpercentile, 84600);
	ck_assert_int_eq(pdata.maxrx, 99 * 300);
	ck_assert_int_eq(pdata.maxtx, 99 * 600);
	ck_assert_int_eq(pdata.max, 89100);
	ck_assert_int_eq(pdata.minrx, 0);
	ck_assert_int_eq(pdata.mintx, 0);
	ck_assert_int_eq(pdata.min, 0);
	ck_assert_int_eq(pdata.sumrx, 1485000);
	ck_assert_int_eq(pdata.sumtx, 2970000);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

void add_percentile_tests(Suite *s)
{
	TCase *tc_percentile = tcase_create("Percentile");
	tcase_add_checked_fixture(tc_percentile, setup, teardown);
	tcase_add_unchecked_fixture(tc_percentile, setup, teardown);
	tcase_add_test(tc_percentile, compare_uint64_t_can_compare);
	tcase_add_test(tc_percentile, compare_uint64_t_can_be_used_with_qsort);
	tcase_add_test(tc_percentile, getpercentiledata_returns_with_no_wrong_interface);
	tcase_add_test(tc_percentile, getpercentiledata_returns_with_no_data);
	tcase_add_test(tc_percentile, getpercentiledata_can_provide_data_with_one_entry);
	tcase_add_test(tc_percentile, getpercentiledata_can_provide_data_with_many_entries);
	tcase_add_test(tc_percentile, getpercentiledata_can_provide_data_with_many_entries_and_order_does_not_matter);
	tcase_add_test(tc_percentile, getpercentiledata_can_check_limit);
	suite_add_tcase(s, tc_percentile);
}
