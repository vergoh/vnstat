#include "common.h"
#include "vnstat_tests.h"
#include "common_tests.h"
#include "dbaccess.h"
#include "cfg.h"

START_TEST(printe_options)
{
	noexit = 2;
	cfg.uselogging = 0;
	ck_assert_int_eq(printe(PT_Info), 1);

	cfg.uselogging = 1;
	ck_assert_int_eq(printe(PT_Multiline), 1);

	noexit = 0;
	strcpy(errorstring, "dummy string");
	suppress_output();
	ck_assert_int_eq(printe(PT_Info), 1);
	ck_assert_int_eq(printe(PT_Error), 1);
	ck_assert_int_eq(printe(PT_Config), 1);
	ck_assert_int_eq(printe(PT_Multiline), 1);
	ck_assert_int_eq(printe(PT_ShortMultiline), 1);
	ck_assert_int_eq(printe(5), 1);
}
END_TEST

START_TEST(logprint_options)
{
	cfg.uselogging = 0;
	ck_assert_int_eq(logprint(PT_Info), 0);

	cfg.uselogging = 1;
	strcpy(cfg.logfile, "/dev/null");
	strcpy(errorstring, "dummy string");
	ck_assert_int_eq(logprint(PT_Info), 1);
	ck_assert_int_eq(logprint(PT_Error), 1);
	ck_assert_int_eq(logprint(PT_Config), 1);
	ck_assert_int_eq(logprint(PT_Multiline), 0);
	ck_assert_int_eq(logprint(PT_ShortMultiline), 1);
	ck_assert_int_eq(logprint(5), 1);
}
END_TEST

START_TEST(dmonth_return_within_range)
{
	int m;
	m = dmonth(_i);
	ck_assert_int_ge(m, 28);
	ck_assert_int_le(m, 31);
}
END_TEST

START_TEST(leapyears_are_known)
{
	ck_assert_int_eq(isleapyear(1995), 0);
	ck_assert_int_eq(isleapyear(1996), 1);
	ck_assert_int_eq(isleapyear(1997), 0);
	ck_assert_int_eq(isleapyear(1998), 0);
	ck_assert_int_eq(isleapyear(1999), 0);
	ck_assert_int_eq(isleapyear(2000), 1);
	ck_assert_int_eq(isleapyear(2001), 0);
	ck_assert_int_eq(isleapyear(2002), 0);
	ck_assert_int_eq(isleapyear(2003), 0);
	ck_assert_int_eq(isleapyear(2004), 1);
	ck_assert_int_eq(isleapyear(2005), 0);
	ck_assert_int_eq(isleapyear(2006), 0);
	ck_assert_int_eq(isleapyear(2007), 0);
	ck_assert_int_eq(isleapyear(2008), 1);
	ck_assert_int_eq(isleapyear(2009), 0);
	ck_assert_int_eq(isleapyear(2010), 0);
	ck_assert_int_eq(isleapyear(2011), 0);
	ck_assert_int_eq(isleapyear(2012), 1);
	ck_assert_int_eq(isleapyear(2013), 0);
	ck_assert_int_eq(isleapyear(2014), 0);
	ck_assert_int_eq(isleapyear(2015), 0);
	ck_assert_int_eq(isleapyear(2016), 1);
	ck_assert_int_eq(isleapyear(2017), 0);
	ck_assert_int_eq(isleapyear(2018), 0);
	ck_assert_int_eq(isleapyear(2019), 0);
	ck_assert_int_eq(isleapyear(2020), 1);
	ck_assert_int_eq(isleapyear(2021), 0);
}
END_TEST

#if defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE) || defined(__linux__)
START_TEST(mosecs_return_values)
{
	defaultcfg();
	ck_assert_int_eq(cfg.monthrotate, 1);
	ck_assert_int_eq(mosecs(0, 0), 1);
	ck_assert_int_eq(mosecs(172800, 173000), 173000);
}
END_TEST
#else
START_TEST(mosecs_return_values_without_timezone)
{
	defaultcfg();
	ck_assert_int_eq(cfg.monthrotate, 1);
	ck_assert_int_eq(mosecs(0, 0), 1);
	ck_assert_int_gt(mosecs(172800, 173000), 1);
}
END_TEST
#endif

START_TEST(mosecs_does_not_change_tz)
{
#if defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE) || defined(__linux__)
	extern long timezone;
#else
	long timezone = 0;
#endif
	long timezone_before_call;

	tzset();
	timezone_before_call = timezone;

	defaultcfg();
	ck_assert_int_eq(cfg.monthrotate, 1);
	ck_assert_int_ne(mosecs(1, 2), 0);
	ck_assert_int_ne(mosecs(1, 2), 1);
	ck_assert_int_eq(timezone_before_call, timezone);
}
END_TEST

START_TEST(mosecs_does_not_change_struct_tm_pointer_content)
{
	struct tm *stm;
	time_t current;

	current = time(NULL);
	stm = localtime(&current);

	defaultcfg();
	ck_assert_int_eq(cfg.monthrotate, 1);
	ck_assert_int_eq(current, timelocal(stm));
	ck_assert_int_ne(mosecs(1, 2), 0);
	ck_assert_int_ne(mosecs(1, 2), 1);
	ck_assert_int_eq(current, timelocal(stm));
}
END_TEST

START_TEST(countercalc_no_change)
{
	uint64_t a, b;

	a = b = 0;
	ck_assert_int_eq(countercalc(&a, &b), 0);
	a = b = 1;
	ck_assert_int_eq(countercalc(&a, &b), 0);
}
END_TEST

START_TEST(countercalc_small_change)
{
	uint64_t a, b;

	a = 0;
	b = 1;
	ck_assert_int_eq(countercalc(&a, &b), 1);
	a = 1;
	b = 2;
	ck_assert_int_eq(countercalc(&a, &b), 1);
	b = 3;
	ck_assert_int_eq(countercalc(&a, &b), 2);
}
END_TEST

START_TEST(countercalc_32bit)
{
	uint64_t a, b;

	a = 1;
	b = 0;
	ck_assert(countercalc(&a, &b)==(MAX32-1));
}
END_TEST

START_TEST(countercalc_64bit)
{
	uint64_t a, b;

	a = MAX32+1;
	b = 0;
	ck_assert(countercalc(&a, &b)==(MAX64-MAX32-1));
}
END_TEST

START_TEST(strncpy_nt_with_below_maximum_length_string)
{
	char dst[6];

	strncpy_nt(dst, "123", 6);
	ck_assert_str_eq(dst, "123");
}
END_TEST

START_TEST(strncpy_nt_with_maximum_length_string)
{
	char dst[6];

	strncpy_nt(dst, "12345", 6);
	ck_assert_str_eq(dst, "12345");
}
END_TEST

START_TEST(strncpy_nt_with_over_maximum_length_string)
{
	char dst[6];

	strncpy_nt(dst, "123456", 6);
	ck_assert_str_eq(dst, "12345");

	strncpy_nt(dst, "1234567890", 6);
	ck_assert_str_eq(dst, "12345");
}
END_TEST

START_TEST(isnumeric_empty)
{
	ck_assert_int_eq(isnumeric(""), 0);
}
END_TEST

START_TEST(isnumeric_it_is)
{
	ck_assert_int_eq(isnumeric("0"), 1);
	ck_assert_int_eq(isnumeric("1"), 1);
	ck_assert_int_eq(isnumeric("12"), 1);
	ck_assert_int_eq(isnumeric("123"), 1);
}
END_TEST

START_TEST(isnumeric_it_is_not)
{
	ck_assert_int_eq(isnumeric("a"), 0);
	ck_assert_int_eq(isnumeric("abc"), 0);
	ck_assert_int_eq(isnumeric("a1"), 0);
	ck_assert_int_eq(isnumeric("1a"), 0);
	ck_assert_int_eq(isnumeric("123abc"), 0);
	ck_assert_int_eq(isnumeric("/"), 0);
	ck_assert_int_eq(isnumeric("-"), 0);
}
END_TEST

START_TEST(getversion_returns_a_version)
{
	ck_assert_int_gt((int)strlen(getversion()), 1);
	ck_assert(strchr(getversion(), '_') == NULL);
	ck_assert(strchr(getversion(), '.') != NULL);
}
END_TEST

START_TEST(timeused_outputs_something_expected)
{
	int pipe, len;
	char buffer[512];
	memset(&buffer, '\0', sizeof(buffer));

	defaultcfg();
	debug = 1;
	pipe = pipe_output();
	/* the assumption here is that the next two steps
	   can always execute in less than one second resulting
	   in a duration that starts with a zero */
	timeused("nothing", 1);
	timeused("something", 0);
	fflush(stdout);

	len = read(pipe, buffer, 512);
	ck_assert_int_gt(len, 0);
	ck_assert_ptr_ne(strstr(buffer, "something() in 0"), NULL);
}
END_TEST

START_TEST(can_panic)
{
	suppress_output();
	fclose(stderr);
	panicexit(__FILE__, __LINE__);
}
END_TEST

void add_common_tests(Suite *s)
{
	TCase *tc_common = tcase_create("Common");
	tcase_add_test(tc_common, printe_options);
	tcase_add_test(tc_common, logprint_options);
	tcase_add_loop_test(tc_common, dmonth_return_within_range, 0, 12);
	tcase_add_test(tc_common, leapyears_are_known);
#if defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE) || defined(__linux__)
	tcase_add_test(tc_common, mosecs_return_values);
#else
	tcase_add_test(tc_common, mosecs_return_values_without_timezone);
#endif
	tcase_add_test(tc_common, mosecs_does_not_change_tz);
	tcase_add_test(tc_common, mosecs_does_not_change_struct_tm_pointer_content);
	tcase_add_test(tc_common, countercalc_no_change);
	tcase_add_test(tc_common, countercalc_small_change);
	tcase_add_test(tc_common, countercalc_32bit);
	tcase_add_test(tc_common, countercalc_64bit);
	tcase_add_test(tc_common, strncpy_nt_with_below_maximum_length_string);
	tcase_add_test(tc_common, strncpy_nt_with_maximum_length_string);
	tcase_add_test(tc_common, strncpy_nt_with_over_maximum_length_string);
	tcase_add_test(tc_common, isnumeric_empty);
	tcase_add_test(tc_common, isnumeric_it_is);
	tcase_add_test(tc_common, isnumeric_it_is_not);
	tcase_add_test(tc_common, getversion_returns_a_version);
	tcase_add_test(tc_common, timeused_outputs_something_expected);
	tcase_add_exit_test(tc_common, can_panic, 1);
	suite_add_tcase(s, tc_common);
}
