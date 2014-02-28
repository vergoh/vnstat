#include "vnstat_tests.h"
#include "misc_tests.h"
#include "common.h"
#include "misc.h"

START_TEST(getbtime_does_not_return_zero)
{
	ck_assert_int_gt(getbtime(), 0);
}
END_TEST

START_TEST(addtraffic_does_not_add_zero_traffic)
{
	uint64_t srcmb, destmb;
	int srckb, destkb;

	srcmb=srckb=destmb=destkb=0;
	addtraffic(&destmb, &destkb, srcmb, srckb);

	ck_assert_int_eq(srcmb, 0);
	ck_assert_int_eq(srckb, 0);
	ck_assert_int_eq(destmb, 0);
	ck_assert_int_eq(destkb, 0);
}
END_TEST

START_TEST(addtraffic_with_simple_mb_addition)
{
	uint64_t srcmb, destmb;
	int srckb, destkb;

	destmb=destkb=0;
	srcmb=1;
	srckb=0;
	addtraffic(&destmb, &destkb, srcmb, srckb);

	ck_assert_int_eq(srcmb, 1);
	ck_assert_int_eq(srckb, 0);
	ck_assert_int_eq(destmb, 1);
	ck_assert_int_eq(destkb, 0);
}
END_TEST

START_TEST(addtraffic_with_simple_kb_addition)
{
	uint64_t srcmb, destmb;
	int srckb, destkb;

	destmb=destkb=0;
	srcmb=0;
	srckb=1;
	addtraffic(&destmb, &destkb, srcmb, srckb);

	ck_assert_int_eq(srcmb, 0);
	ck_assert_int_eq(srckb, 1);
	ck_assert_int_eq(destmb, 0);
	ck_assert_int_eq(destkb, 1);
}
END_TEST

START_TEST(addtraffic_with_simple_mixed_addition)
{
	uint64_t srcmb, destmb;
	int srckb, destkb;

	destmb=destkb=0;
	srcmb=1;
	srckb=1;
	addtraffic(&destmb, &destkb, srcmb, srckb);

	ck_assert_int_eq(srcmb, 1);
	ck_assert_int_eq(srckb, 1);
	ck_assert_int_eq(destmb, 1);
	ck_assert_int_eq(destkb, 1);
}
END_TEST

START_TEST(addtraffic_with_multiple_mixed_additions)
{
	uint64_t srcmb, destmb;
	int srckb, destkb, i;

	destmb=destkb=0;
	srcmb=1;
	srckb=1;

	for (i=1; i<=10; i++) {
		addtraffic(&destmb, &destkb, srcmb, srckb);

		ck_assert_int_eq(srcmb, 1);
		ck_assert_int_eq(srckb, 1);
		ck_assert_int_eq(destmb, (uint64_t)i);
		ck_assert_int_eq(destkb, i);
	}
}
END_TEST

START_TEST(addtraffic_with_exact_kb_to_mb_conversion)
{
	uint64_t srcmb, destmb;
	int srckb, destkb;

	destmb=destkb=0;
	srcmb=0;
	srckb=1024;
	addtraffic(&destmb, &destkb, srcmb, srckb);

	ck_assert_int_eq(srcmb, 0);
	ck_assert_int_eq(srckb, 1024);
	ck_assert_int_eq(destmb, 1);
	ck_assert_int_eq(destkb, 0);
}
END_TEST

START_TEST(addtraffic_with_inexact_kb_to_mb_conversion)
{
	uint64_t srcmb, destmb;
	int srckb, destkb;

	destmb=destkb=0;
	srcmb=0;
	srckb=1025;
	addtraffic(&destmb, &destkb, srcmb, srckb);

	ck_assert_int_eq(srcmb, 0);
	ck_assert_int_eq(srckb, 1025);
	ck_assert_int_eq(destmb, 1);
	ck_assert_int_eq(destkb, 1);
}
END_TEST

START_TEST(addtraffic_with_multiple_kb_to_mb_conversions)
{
	uint64_t destmb;
	int destkb;

	destmb=destkb=0;

	addtraffic(&destmb, &destkb, 0, 1023);
	ck_assert_int_eq(destmb, 0);
	ck_assert_int_eq(destkb, 1023);

	addtraffic(&destmb, &destkb, 0, 1);
	ck_assert_int_eq(destmb, 1);
	ck_assert_int_eq(destkb, 0);

	addtraffic(&destmb, &destkb, 0, 1);
	ck_assert_int_eq(destmb, 1);
	ck_assert_int_eq(destkb, 1);

	addtraffic(&destmb, &destkb, 0, 1023);
	ck_assert_int_eq(destmb, 2);
	ck_assert_int_eq(destkb, 0);

	addtraffic(&destmb, &destkb, 0, 1024);
	ck_assert_int_eq(destmb, 3);
	ck_assert_int_eq(destkb, 0);

	addtraffic(&destmb, &destkb, 0, 1025);
	ck_assert_int_eq(destmb, 4);
	ck_assert_int_eq(destkb, 1);

	addtraffic(&destmb, &destkb, 0, 512);
	ck_assert_int_eq(destmb, 4);
	ck_assert_int_eq(destkb, 513);

	addtraffic(&destmb, &destkb, 0, 512);
	ck_assert_int_eq(destmb, 5);
	ck_assert_int_eq(destkb, 1);

	addtraffic(&destmb, &destkb, 0, 2048);
	ck_assert_int_eq(destmb, 7);
	ck_assert_int_eq(destkb, 1);
}
END_TEST

START_TEST(getunit_returns_something_with_all_cfg_combinations)
{
	char *string;
	int j;

	cfg.unit = _i;
	for (j=1; j<=(UNITCOUNT+1); j++) {
		string = getunit(j);
		ck_assert_int_gt(strlen(string), 0);
	}
}
END_TEST

START_TEST(getrateunit_returns_something_with_all_cfg_combinations)
{
	char *string;
	int j;

	for (j=1; j<=(UNITCOUNT+1); j++) {
		string = getrateunit(_i, j);
		ck_assert_int_gt(strlen(string), 0);
	}
}
END_TEST

START_TEST(getunitdivider_returns_something_with_all_cfg_combinations)
{
	int j;

	for (j=1; j<=(UNITCOUNT+1); j++) {
		if (j==1 || j>UNITCOUNT) {
			ck_assert_int_eq(getunitdivider(_i, j), 0);
		} else {
			ck_assert_int_ne(getunitdivider(_i, j), 0);
		}
	}
}
END_TEST

START_TEST(spacecheck_does_not_check_when_not_configured)
{
	cfg.spacecheck = 0;
	ck_assert_int_eq(spacecheck("/nonexistentpath"), 1);
}
END_TEST

START_TEST(spacecheck_checks_space)
{
	cfg.spacecheck = 1;
	/* it's assumed that /tmp isn't full */
	ck_assert_int_eq(spacecheck("/tmp"), 1);
}
END_TEST

START_TEST(spacecheck_fails_with_invalid_path)
{
	noexit = 1;
	cfg.spacecheck = 1;
	ck_assert_int_eq(spacecheck("/nonexistentpath"), 0);
}
END_TEST

START_TEST(getvalue_normal)
{
	cfg.unit = 0;
	ck_assert_str_eq(getvalue(0, 100, 0, 1), "100 KiB");
	ck_assert_str_eq(getvalue(1, 0, 0, 1), "1.00 MiB");
	ck_assert_str_eq(getvalue(1024, 0, 0, 1), "1.00 GiB");
	ck_assert_str_eq(getvalue(1048576, 0, 0, 2), "1.00 TiB");
	cfg.unit = 1;
	ck_assert_str_eq(getvalue(0, 100, 0, 1), "100 KB");
	ck_assert_str_eq(getvalue(1, 0, 0, 1), "1.00 MB");
	ck_assert_str_eq(getvalue(1024, 0, 0, 1), "1.00 GB");
	ck_assert_str_eq(getvalue(1048576, 0, 0, 2), "1.00 TB");
}
END_TEST

START_TEST(getvalue_estimate)
{
	cfg.unit = 0;
	ck_assert_str_eq(getvalue(0, 100, 0, 2), "100 KiB");
	ck_assert_str_eq(getvalue(1, 0, 0, 2), "1 MiB");
	ck_assert_str_eq(getvalue(1024, 0, 0, 2), "1.00 GiB");
	ck_assert_str_eq(getvalue(1048576, 0, 0, 2), "1.00 TiB");
	cfg.unit = 1;
	ck_assert_str_eq(getvalue(0, 100, 0, 2), "100 KB");
	ck_assert_str_eq(getvalue(1, 0, 0, 2), "1 MB");
	ck_assert_str_eq(getvalue(1024, 0, 0, 2), "1.00 GB");
	ck_assert_str_eq(getvalue(1048576, 0, 0, 2), "1.00 TB");
}
END_TEST

START_TEST(getvalue_imagescale)
{
	cfg.unit = 0;
	ck_assert_str_eq(getvalue(0, 100, 0, 3), "100 KiB");
	ck_assert_str_eq(getvalue(1, 0, 0, 3), "1 MiB");
	ck_assert_str_eq(getvalue(1024, 0, 0, 3), "1 GiB");
	ck_assert_str_eq(getvalue(1048576, 0, 0, 3), "1 TiB");
	cfg.unit = 1;
	ck_assert_str_eq(getvalue(0, 100, 0, 3), "100 KB");
	ck_assert_str_eq(getvalue(1, 0, 0, 3), "1 MB");
	ck_assert_str_eq(getvalue(1024, 0, 0, 3), "1 GB");
	ck_assert_str_eq(getvalue(1048576, 0, 0, 3), "1 TB");
}
END_TEST

START_TEST(getvalue_padding)
{
	cfg.unit = 0;
	ck_assert_str_eq(getvalue(0, 100, 10, 1), "   100 KiB");
	cfg.unit = 1;
	ck_assert_str_eq(getvalue(0, 100, 10, 1), "    100 KB");
}
END_TEST

START_TEST(getvalue_mb_kb_mixed)
{
	cfg.unit = 0;
	ck_assert_str_eq(getvalue(1, 3210, 0, 1), "4.13 MiB");
	cfg.unit = 1;
	ck_assert_str_eq(getvalue(1, 3210, 0, 1), "4.13 MB");
}
END_TEST

START_TEST(getvalue_zero_values)
{
	cfg.unit = 0;
	ck_assert_str_eq(getvalue(0, 0, 0, 1), "0 KiB");
	ck_assert_str_eq(getvalue(0, 0, 0, 2), "--    ");
	ck_assert_str_eq(getvalue(0, 0, 0, 3), "0 KiB");
	cfg.unit = 1;
	ck_assert_str_eq(getvalue(0, 0, 0, 1), "0 KB");
	ck_assert_str_eq(getvalue(0, 0, 0, 2), "--    ");
	ck_assert_str_eq(getvalue(0, 0, 0, 3), "0 KB");
}
END_TEST

START_TEST(getrate_zero_interval)
{
	int i, j;

	for (i=0; i<=1; i++) {
		cfg.rateunit = i;
		for (j=0; j<=2; j++) {
			cfg.unit = j;
			ck_assert_str_eq(getrate(1, 0, 0, 0), "n/a");
		}
	}
}
END_TEST

START_TEST(getrate_bytes)
{
	cfg.rateunit = 0;
	cfg.unit = 0;
	ck_assert_str_eq(getrate(0, 100, 1, 0), "100.00 KiB/s");
	ck_assert_str_eq(getrate(1, 3210, 1, 0), "4.13 MiB/s");
	ck_assert_str_eq(getrate(1024, 0, 1, 0), "1.00 GiB/s");
	ck_assert_str_eq(getrate(1048576, 0, 1, 0), "1.00 TiB/s");
	cfg.unit = 1;
	ck_assert_str_eq(getrate(0, 100, 1, 0), "100.00 KB/s");
	ck_assert_str_eq(getrate(1, 3210, 1, 0), "4.13 MB/s");
	ck_assert_str_eq(getrate(1024, 0, 1, 0), "1.00 GB/s");
	ck_assert_str_eq(getrate(1048576, 0, 1, 0), "1.00 TB/s");
}
END_TEST

START_TEST(getrate_bits)
{
	cfg.rateunit = 1;
	cfg.unit = 0;
	ck_assert_str_eq(getrate(0, 100, 1, 0), "800 kbit/s");
	ck_assert_str_eq(getrate(1, 3210, 1, 0), "33.87 Mbit/s");
	ck_assert_str_eq(getrate(1024, 0, 1, 0), "8.39 Gbit/s");
	ck_assert_str_eq(getrate(1048576, 0, 1, 0), "8.59 Tbit/s");
	cfg.unit = 1;
	ck_assert_str_eq(getrate(0, 100, 1, 0), "800 kbit/s");
	ck_assert_str_eq(getrate(1, 3210, 1, 0), "33.87 Mbit/s");
	ck_assert_str_eq(getrate(1024, 0, 1, 0), "8.39 Gbit/s");
	ck_assert_str_eq(getrate(1048576, 0, 1, 0), "8.59 Tbit/s");
}
END_TEST

START_TEST(getrate_interval_divides)
{
	cfg.unit = 0;
	cfg.rateunit = 0;
	ck_assert_str_eq(getrate(0, 100, 1, 0), "100.00 KiB/s");
	ck_assert_str_eq(getrate(0, 100, 2, 0), "50.00 KiB/s");
	ck_assert_str_eq(getrate(0, 100, 10, 0), "10.00 KiB/s");
	cfg.rateunit = 1;
	ck_assert_str_eq(getrate(0, 100, 1, 0), "800 kbit/s");
	ck_assert_str_eq(getrate(0, 100, 2, 0), "400 kbit/s");
	ck_assert_str_eq(getrate(0, 100, 10, 0), "80.00 kbit/s");
}
END_TEST

START_TEST(getrate_padding)
{
	cfg.unit = 0;
	cfg.rateunit = 0;
	ck_assert_str_eq(getrate(0, 100, 1, 0), "100.00 KiB/s");
	ck_assert_str_eq(getrate(0, 100, 1, 12), "100.00 KiB/s");
	ck_assert_str_eq(getrate(0, 100, 1, 14), "  100.00 KiB/s");
}
END_TEST

START_TEST(gettrafficrate_zero_interval)
{
	int i, j;

	for (i=0; i<=1; i++) {
		cfg.rateunit = i;
		for (j=0; j<=2; j++) {
			cfg.unit = j;
			ck_assert_str_eq(gettrafficrate(1, 0, 0), "n/a");
		}
	}
}
END_TEST

START_TEST(gettrafficrate_bytes)
{
	cfg.rateunit = 0;
	cfg.unit = 0;
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "100.00 KiB/s");
	ck_assert_str_eq(gettrafficrate(1048576, 1, 0), "1.00 MiB/s");
	ck_assert_str_eq(gettrafficrate(1073741824, 1, 0), "1.00 GiB/s");
	ck_assert_str_eq(gettrafficrate(1099511627776, 1, 0), "1.00 TiB/s");
	cfg.unit = 1;
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "100.00 KB/s");
	ck_assert_str_eq(gettrafficrate(1048576, 1, 0), "1.00 MB/s");
	ck_assert_str_eq(gettrafficrate(1073741824, 1, 0), "1.00 GB/s");
	ck_assert_str_eq(gettrafficrate(1099511627776, 1, 0), "1.00 TB/s");
}
END_TEST

START_TEST(gettrafficrate_bits)
{
	cfg.rateunit = 1;
	cfg.unit = 0;
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "800 kbit/s");
	ck_assert_str_eq(gettrafficrate(1048576, 1, 0), "8.19 Mbit/s");
	ck_assert_str_eq(gettrafficrate(1073741824, 1, 0), "8.39 Gbit/s");
	ck_assert_str_eq(gettrafficrate(1099511627776, 1, 0), "8.59 Tbit/s");
	cfg.unit = 1;
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "800 kbit/s");
	ck_assert_str_eq(gettrafficrate(1048576, 1, 0), "8.19 Mbit/s");
	ck_assert_str_eq(gettrafficrate(1073741824, 1, 0), "8.39 Gbit/s");
	ck_assert_str_eq(gettrafficrate(1099511627776, 1, 0), "8.59 Tbit/s");
}
END_TEST

START_TEST(gettrafficrate_interval_divides)
{
	cfg.unit = 0;
	cfg.rateunit = 0;
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "100.00 KiB/s");
	ck_assert_str_eq(gettrafficrate(102400, 2, 0), "50.00 KiB/s");
	ck_assert_str_eq(gettrafficrate(102400, 10, 0), "10.00 KiB/s");
	cfg.rateunit = 1;
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "800 kbit/s");
	ck_assert_str_eq(gettrafficrate(102400, 2, 0), "400 kbit/s");
	ck_assert_str_eq(gettrafficrate(102400, 10, 0), "80.00 kbit/s");
}
END_TEST

START_TEST(gettrafficrate_padding)
{
	cfg.unit = 0;
	cfg.rateunit = 0;
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "100.00 KiB/s");
	ck_assert_str_eq(gettrafficrate(102400, 1, 12), "100.00 KiB/s");
	ck_assert_str_eq(gettrafficrate(102400, 1, 14), "  100.00 KiB/s");
}
END_TEST

START_TEST(getscale_zero)
{
	ck_assert_int_eq(getscale(0), 1);
}
END_TEST

START_TEST(getscale_nonzero)
{
	ck_assert_int_eq(getscale(1), 1);
	ck_assert_int_eq(getscale(2), 1);
	ck_assert_int_eq(getscale(10), 2);
	ck_assert_int_eq(getscale(20), 5);
	ck_assert_int_eq(getscale(50), 20);
	ck_assert_int_eq(getscale(1023), 300);
	ck_assert_int_eq(getscale(1024), 300);
	ck_assert_int_eq(getscale(1025), 1024);
	ck_assert_int_eq(getscale(1026), 1024);
	ck_assert_int_eq(getscale(1500), 1024);
	ck_assert_int_eq(getscale(2047), 1024);
	ck_assert_int_eq(getscale(2048), 1024);
	ck_assert_int_eq(getscale(2049), 1024);
	ck_assert_int_eq(getscale(8191), 1024);
	ck_assert_int_eq(getscale(8192), 2048);
	ck_assert_int_eq(getscale(8193), 2048);
}
END_TEST

void add_misc_tests(Suite *s)
{
	/* Misc test cases */
	TCase *tc_misc = tcase_create("Misc");
	tcase_add_test(tc_misc, getbtime_does_not_return_zero);
	tcase_add_test(tc_misc, addtraffic_does_not_add_zero_traffic);
	tcase_add_test(tc_misc, addtraffic_with_simple_mb_addition);
	tcase_add_test(tc_misc, addtraffic_with_simple_kb_addition);
	tcase_add_test(tc_misc, addtraffic_with_simple_mixed_addition);
	tcase_add_test(tc_misc, addtraffic_with_multiple_mixed_additions);
	tcase_add_test(tc_misc, addtraffic_with_exact_kb_to_mb_conversion);
	tcase_add_test(tc_misc, addtraffic_with_inexact_kb_to_mb_conversion);
	tcase_add_test(tc_misc, addtraffic_with_multiple_kb_to_mb_conversions);
	tcase_add_loop_test(tc_misc, getunit_returns_something_with_all_cfg_combinations, 0, 2);
	tcase_add_loop_test(tc_misc, getrateunit_returns_something_with_all_cfg_combinations, 0, 3);
	tcase_add_loop_test(tc_misc, getunitdivider_returns_something_with_all_cfg_combinations, 0, 3);
	tcase_add_test(tc_misc, spacecheck_does_not_check_when_not_configured);
	tcase_add_test(tc_misc, spacecheck_checks_space);
	tcase_add_test(tc_misc, spacecheck_fails_with_invalid_path);
	tcase_add_test(tc_misc, getvalue_normal);
	tcase_add_test(tc_misc, getvalue_estimate);
	tcase_add_test(tc_misc, getvalue_imagescale);
	tcase_add_test(tc_misc, getvalue_padding);
	tcase_add_test(tc_misc, getvalue_mb_kb_mixed);
	tcase_add_test(tc_misc, getvalue_zero_values);
	tcase_add_test(tc_misc, getrate_zero_interval);
	tcase_add_test(tc_misc, getrate_bytes);
	tcase_add_test(tc_misc, getrate_bits);
	tcase_add_test(tc_misc, getrate_interval_divides);
	tcase_add_test(tc_misc, getrate_padding);
	tcase_add_test(tc_misc, gettrafficrate_zero_interval);
	tcase_add_test(tc_misc, gettrafficrate_bytes);
	tcase_add_test(tc_misc, gettrafficrate_bits);
	tcase_add_test(tc_misc, gettrafficrate_interval_divides);
	tcase_add_test(tc_misc, gettrafficrate_padding);
	tcase_add_test(tc_misc, getscale_zero);
	tcase_add_test(tc_misc, getscale_nonzero);
	suite_add_tcase(s, tc_misc);
}
