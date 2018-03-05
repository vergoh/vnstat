#include "common.h"
#include "vnstat_tests.h"
#include "misc_tests.h"
#include "misc.h"

START_TEST(getbtime_does_not_return_zero)
{
	ck_assert_int_gt(getbtime(), 0);
}
END_TEST

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wused-but-marked-unused"
#endif
START_TEST(getunitprefix_returns_something_with_all_cfg_combinations)
{
	const char *string;
	int j;

	cfg.unitmode = _i;
	for (j=1; j<=(UNITPREFIXCOUNT+1); j++) {
		string = getunitprefix(j);
		ck_assert_int_gt(strlen(string), 0);
	}
}
END_TEST

START_TEST(getrateunitprefix_returns_something_with_all_cfg_combinations)
{
	const char *string;
	int j;

	for (j=1; j<=(UNITPREFIXCOUNT+1); j++) {
		string = getrateunitprefix(_i, j);
		ck_assert_int_gt(strlen(string), 0);
	}
}
END_TEST

START_TEST(getunitdivisor_returns_something_with_all_cfg_combinations)
{
	int j;
	char div[16];

	for (j=1; j<=(UNITPREFIXCOUNT+1); j++) {
		snprintf(div, 15, "%"PRIu64"", getunitdivisor(_i, j));
		if (j>UNITPREFIXCOUNT) {
			ck_assert_str_eq(div, "1");
		} else {
			ck_assert_str_ne(div, "0");
		}
	}
}
END_TEST
#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

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
	cfg.defaultdecimals = 2;
	cfg.unitmode = 0;
	ck_assert_str_eq(getvalue(100, 0, 1), "100 B");
	ck_assert_str_eq(getvalue(1024, 0, 1), "1.00 KiB");
	ck_assert_str_eq(getvalue(1048576, 0, 1), "1.00 MiB");
	ck_assert_str_eq(getvalue(1073741824, 0, 1), "1.00 GiB");
	ck_assert_str_eq(getvalue(1099511627776ULL, 0, 1), "1.00 TiB");
	ck_assert_str_eq(getvalue(1125899906842624ULL, 0, 1), "1.00 PiB");
	cfg.unitmode = 1;
	ck_assert_str_eq(getvalue(100, 0, 1), "100 B");
	ck_assert_str_eq(getvalue(1024, 0, 1), "1.00 KB");
	ck_assert_str_eq(getvalue(1048576, 0, 1), "1.00 MB");
	ck_assert_str_eq(getvalue(1073741824, 0, 1), "1.00 GB");
	ck_assert_str_eq(getvalue(1099511627776ULL, 0, 1), "1.00 TB");
	ck_assert_str_eq(getvalue(1125899906842624ULL, 0, 1), "1.00 PB");
}
END_TEST

START_TEST(getvalue_estimate)
{
	cfg.defaultdecimals = 2;
	cfg.unitmode = 0;
	ck_assert_str_eq(getvalue(100, 0, 2), "100 B");
	ck_assert_str_eq(getvalue(1024, 0, 2), "1 KiB");
	ck_assert_str_eq(getvalue(1048576, 0, 2), "1.00 MiB");
	ck_assert_str_eq(getvalue(1073741824, 0, 2), "1.00 GiB");
	ck_assert_str_eq(getvalue(1099511627776ULL, 0, 2), "1.00 TiB");
	ck_assert_str_eq(getvalue(1125899906842624ULL, 0, 2), "1.00 PiB");
	cfg.unitmode = 1;
	ck_assert_str_eq(getvalue(100, 0, 2), "100 B");
	ck_assert_str_eq(getvalue(1024, 0, 2), "1 KB");
	ck_assert_str_eq(getvalue(1048576, 0, 2), "1.00 MB");
	ck_assert_str_eq(getvalue(1073741824, 0, 2), "1.00 GB");
	ck_assert_str_eq(getvalue(1099511627776ULL, 0, 2), "1.00 TB");
	ck_assert_str_eq(getvalue(1125899906842624ULL, 0, 2), "1.00 PB");
}
END_TEST

START_TEST(getvalue_imagescale)
{
	cfg.defaultdecimals = 2;
	cfg.unitmode = 0;
	ck_assert_str_eq(getvalue(100, 0, 3), "100 B");
	ck_assert_str_eq(getvalue(1024, 0, 3), "1 KiB");
	ck_assert_str_eq(getvalue(1048576, 0, 3), "1 MiB");
	ck_assert_str_eq(getvalue(1073741824, 0, 3), "1 GiB");
	ck_assert_str_eq(getvalue(1099511627776ULL, 0,3), "1 TiB");
	ck_assert_str_eq(getvalue(1125899906842624ULL, 0, 3), "1 PiB");
	cfg.unitmode = 1;
	ck_assert_str_eq(getvalue(100, 0, 3), "100 B");
	ck_assert_str_eq(getvalue(1024, 0, 3), "1 KB");
	ck_assert_str_eq(getvalue(1048576, 0, 3), "1 MB");
	ck_assert_str_eq(getvalue(1073741824, 0, 3), "1 GB");
	ck_assert_str_eq(getvalue(1099511627776ULL, 0, 3), "1 TB");
	ck_assert_str_eq(getvalue(1125899906842624ULL, 0, 3), "1 PB");
}
END_TEST

START_TEST(getvalue_padding)
{
	cfg.defaultdecimals = 2;
	cfg.unitmode = 0;
	ck_assert_str_eq(getvalue(1024, 10, 1), "  1.00 KiB");
	cfg.unitmode = 1;
	ck_assert_str_eq(getvalue(1024, 10, 1), "   1.00 KB");
}
END_TEST

START_TEST(getvalue_zero_values)
{
	cfg.unitmode = 0;
	ck_assert_str_eq(getvalue(0, 0, 1), "0 B");
	ck_assert_str_eq(getvalue(0, 10, 2), "   --     ");
	ck_assert_int_eq((int)strlen(getvalue(0, 10, 2)), 10);
	ck_assert_int_eq((int)strlen(getvalue(0, 20, 2)), 20);
	ck_assert_str_eq(getvalue(0, 0, 3), "0 B");
	cfg.unitmode = 1;
	ck_assert_str_eq(getvalue(0, 0, 1), "0 B");
	ck_assert_str_eq(getvalue(0, 10, 2), "    --    ");
	ck_assert_int_eq((int)strlen(getvalue(0, 10, 2)), 10);
	ck_assert_int_eq((int)strlen(getvalue(0, 20, 2)), 20);
	ck_assert_str_eq(getvalue(0, 0, 3), "0 B");
}
END_TEST

START_TEST(gettrafficrate_zero_interval)
{
	int i, j;

	for (i=0; i<=1; i++) {
		cfg.rateunit = i;
		for (j=0; j<=2; j++) {
			cfg.unitmode = j;
			ck_assert_str_eq(gettrafficrate(1, 0, 0), "n/a");
		}
	}
}
END_TEST

START_TEST(gettrafficrate_bytes)
{
	cfg.defaultdecimals = 2;
	cfg.rateunit = 0;
	cfg.unitmode = 0;
	ck_assert_str_eq(gettrafficrate(900, 1, 0), "900 B/s");
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "100.00 KiB/s");
	ck_assert_str_eq(gettrafficrate(1048576, 1, 0), "1.00 MiB/s");
	ck_assert_str_eq(gettrafficrate(1073741824, 1, 0), "1.00 GiB/s");
	ck_assert_str_eq(gettrafficrate(1099511627776ULL, 1, 0), "1.00 TiB/s");
	ck_assert_str_eq(gettrafficrate(1125899906842624ULL, 1, 0), "1.00 PiB/s");
	cfg.unitmode = 1;
	ck_assert_str_eq(gettrafficrate(900, 1, 0), "900 B/s");
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "100.00 KB/s");
	ck_assert_str_eq(gettrafficrate(1048576, 1, 0), "1.00 MB/s");
	ck_assert_str_eq(gettrafficrate(1073741824, 1, 0), "1.00 GB/s");
	ck_assert_str_eq(gettrafficrate(1099511627776ULL, 1, 0), "1.00 TB/s");
	ck_assert_str_eq(gettrafficrate(1125899906842624ULL, 1, 0), "1.00 PB/s");
}
END_TEST

START_TEST(gettrafficrate_bits)
{
	cfg.defaultdecimals = 2;
	cfg.rateunit = 1;

	cfg.rateunitmode = 1;
	cfg.unitmode = 0;
	ck_assert_str_eq(gettrafficrate(100, 1, 0), "800 bit/s");
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "819.20 kbit/s");
	ck_assert_str_eq(gettrafficrate(1048576, 1, 0), "8.39 Mbit/s");
	ck_assert_str_eq(gettrafficrate(1073741824, 1, 0), "8.59 Gbit/s");
	ck_assert_str_eq(gettrafficrate(1099511627776ULL, 1, 0), "8.80 Tbit/s");
	ck_assert_str_eq(gettrafficrate(1125899906842624ULL, 1, 0), "9.01 Pbit/s");
	cfg.unitmode = 1;
	ck_assert_str_eq(gettrafficrate(100, 1, 0), "800 bit/s");
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "819.20 kbit/s");
	ck_assert_str_eq(gettrafficrate(1048576, 1, 0), "8.39 Mbit/s");
	ck_assert_str_eq(gettrafficrate(1073741824, 1, 0), "8.59 Gbit/s");
	ck_assert_str_eq(gettrafficrate(1099511627776ULL, 1, 0), "8.80 Tbit/s");
	ck_assert_str_eq(gettrafficrate(1125899906842624ULL, 1, 0), "9.01 Pbit/s");

	cfg.rateunitmode = 0;
	cfg.unitmode = 0;
	ck_assert_str_eq(gettrafficrate(100, 1, 0), "800 bit/s");
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "800.00 Kibit/s");
	ck_assert_str_eq(gettrafficrate(1048576, 1, 0), "8.00 Mibit/s");
	ck_assert_str_eq(gettrafficrate(1073741824, 1, 0), "8.00 Gibit/s");
	ck_assert_str_eq(gettrafficrate(1099511627776ULL, 1, 0), "8.00 Tibit/s");
	ck_assert_str_eq(gettrafficrate(1125899906842624ULL, 1, 0), "8.00 Pibit/s");
	cfg.unitmode = 1;
	ck_assert_str_eq(gettrafficrate(100, 1, 0), "800 bit/s");
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "800.00 Kibit/s");
	ck_assert_str_eq(gettrafficrate(1048576, 1, 0), "8.00 Mibit/s");
	ck_assert_str_eq(gettrafficrate(1073741824, 1, 0), "8.00 Gibit/s");
	ck_assert_str_eq(gettrafficrate(1099511627776ULL, 1, 0), "8.00 Tibit/s");
	ck_assert_str_eq(gettrafficrate(1125899906842624ULL, 1, 0), "8.00 Pibit/s");
}
END_TEST

START_TEST(gettrafficrate_interval_divides)
{
	cfg.defaultdecimals = 2;
	cfg.unitmode = 0;
	cfg.rateunitmode = 1;
	cfg.rateunit = 0;
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "100.00 KiB/s");
	ck_assert_str_eq(gettrafficrate(102400, 2, 0), "50.00 KiB/s");
	ck_assert_str_eq(gettrafficrate(102400, 10, 0), "10.00 KiB/s");
	cfg.rateunit = 1;
	ck_assert_str_eq(gettrafficrate(102400, 1, 0), "819.20 kbit/s");
	ck_assert_str_eq(gettrafficrate(102400, 2, 0), "409.60 kbit/s");
	ck_assert_str_eq(gettrafficrate(102400, 10, 0), "81.92 kbit/s");
}
END_TEST

START_TEST(gettrafficrate_padding)
{
	cfg.defaultdecimals = 2;
	cfg.unitmode = 0;
	cfg.rateunit = 0;
	ck_assert_str_eq(gettrafficrate(102400, 1, 0),  "100.00 KiB/s");
	ck_assert_str_eq(gettrafficrate(102400, 1, 12), "100.00 KiB/s");
	ck_assert_str_eq(gettrafficrate(102400, 1, 14), "  100.00 KiB/s");

	ck_assert_str_eq(gettrafficrate(900, 1, 0),  "900 B/s");
	ck_assert_str_eq(gettrafficrate(900, 1, 7),  "900 B/s");
	ck_assert_str_eq(gettrafficrate(900, 1, 12), "     900 B/s");
	ck_assert_str_eq(gettrafficrate(900, 1, 14), "       900 B/s");
}
END_TEST

START_TEST(sighandler_sets_signal)
{
	debug = 1;
	intsignal = 0;
	disable_logprints();
	ck_assert(signal(SIGINT, sighandler)!=SIG_ERR);
	ck_assert(signal(SIGHUP, sighandler)!=SIG_ERR);
	ck_assert(signal(SIGTERM, sighandler)!=SIG_ERR);

	ck_assert_int_eq(kill(getpid(), SIGINT), 0);
	ck_assert_int_eq(intsignal, SIGINT);

	ck_assert_int_eq(kill(getpid(), SIGHUP), 0);
	ck_assert_int_eq(intsignal, SIGHUP);

	ck_assert_int_eq(kill(getpid(), SIGTERM), 0);
	ck_assert_int_eq(intsignal, SIGTERM);
}
END_TEST

void add_misc_tests(Suite *s)
{
	TCase *tc_misc = tcase_create("Misc");
	tcase_add_test(tc_misc, getbtime_does_not_return_zero);
	tcase_add_loop_test(tc_misc, getunitprefix_returns_something_with_all_cfg_combinations, 0, 2);
	tcase_add_loop_test(tc_misc, getrateunitprefix_returns_something_with_all_cfg_combinations, 0, 3);
	tcase_add_loop_test(tc_misc, getunitdivisor_returns_something_with_all_cfg_combinations, 0, 3);
	tcase_add_test(tc_misc, spacecheck_does_not_check_when_not_configured);
	tcase_add_test(tc_misc, spacecheck_checks_space);
	tcase_add_test(tc_misc, spacecheck_fails_with_invalid_path);
	tcase_add_test(tc_misc, getvalue_normal);
	tcase_add_test(tc_misc, getvalue_estimate);
	tcase_add_test(tc_misc, getvalue_imagescale);
	tcase_add_test(tc_misc, getvalue_padding);
	tcase_add_test(tc_misc, getvalue_zero_values);
	tcase_add_test(tc_misc, gettrafficrate_zero_interval);
	tcase_add_test(tc_misc, gettrafficrate_bytes);
	tcase_add_test(tc_misc, gettrafficrate_bits);
	tcase_add_test(tc_misc, gettrafficrate_interval_divides);
	tcase_add_test(tc_misc, gettrafficrate_padding);
	tcase_add_test(tc_misc, sighandler_sets_signal);
	suite_add_tcase(s, tc_misc);
}
