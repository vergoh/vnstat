#include "common.h"
#include "vnstat_tests.h"
#include "image_tests.h"
#include "dbsql.h"
#include "cfg.h"
#include "image.h"

START_TEST(initimagecontent_does_not_crash)
{
	IMAGECONTENT ic;
	initimagecontent(&ic);
}
END_TEST

START_TEST(colorinit_does_not_crash)
{
	IMAGECONTENT ic;
	defaultcfg();
	ic.im = gdImageCreate(2, 2);
	colorinit(&ic);
	gdImageDestroy(ic.im);
}
END_TEST

START_TEST(layoutinit_does_not_crash)
{
	IMAGECONTENT ic;
	defaultcfg();
	initimagecontent(&ic);
	ic.im = gdImageCreate(640, 480);
	colorinit(&ic);
	ic.interface.updated = time(NULL);
	layoutinit(&ic, "testing 123", 640, 480);
	gdImageDestroy(ic.im);
}
END_TEST

START_TEST(getimagevalue_zeropadding)
{
	ck_assert_str_eq(getimagevalue(0, 0, 0), "--");
	ck_assert_str_eq(getimagevalue(0, 2, 0), "--");
	ck_assert_str_eq(getimagevalue(0, 3, 0), " --");
	ck_assert_str_eq(getimagevalue(0, 0, 1), "--");
	ck_assert_str_eq(getimagevalue(0, 2, 1), "--");
	ck_assert_str_eq(getimagevalue(0, 3, 1), " --");
}
END_TEST

START_TEST(getimagevalue_normal)
{
	ck_assert_str_eq(getimagevalue(1, 0, 0), "1");
	ck_assert_str_eq(getimagevalue(2, 0, 0), "2");
	ck_assert_str_eq(getimagevalue(1000, 0, 0), "1");
	ck_assert_str_eq(getimagevalue(1024, 0, 0), "1");
	ck_assert_str_eq(getimagevalue(2000, 0, 0), "2");
	ck_assert_str_eq(getimagevalue(2345, 0, 0), "2");
	ck_assert_str_eq(getimagevalue(123000, 0, 0), "120");
	ck_assert_str_eq(getimagevalue(1024000, 0, 0), "1");
	ck_assert_str_eq(getimagevalue(1048576, 0, 0), "1");
	ck_assert_str_eq(getimagevalue(1048576000, 0, 0), "1");
	ck_assert_str_eq(getimagevalue(1073741824, 0, 0), "1");
	ck_assert_str_eq(getimagevalue(1073741824000ULL, 0, 0), "1");
	ck_assert_str_eq(getimagevalue(1099511627776ULL, 0, 0), "1");
}
END_TEST

START_TEST(getimagevalue_rate_1024)
{
	cfg.rateunit = 0;
	cfg.rateunitmode = 0;
	ck_assert_str_eq(getimagevalue(1, 0, 1), "1");
	ck_assert_str_eq(getimagevalue(2, 0, 1), "2");
	ck_assert_str_eq(getimagevalue(1000, 0, 1), "1");
	ck_assert_str_eq(getimagevalue(2000, 0, 1), "2");
	ck_assert_str_eq(getimagevalue(1024000, 0, 1), "1");
	ck_assert_str_eq(getimagevalue(1048576000, 0, 1), "1");
	ck_assert_str_eq(getimagevalue(1073741824000ULL, 0, 1), "1");
}
END_TEST

START_TEST(getimagevalue_rate_1000)
{
	cfg.rateunit = 1;
	cfg.rateunitmode = 1;
	ck_assert_str_eq(getimagevalue(1, 0, 1), "1");
	ck_assert_str_eq(getimagevalue(2, 0, 1), "2");
	ck_assert_str_eq(getimagevalue(1000, 0, 1), "1");
	ck_assert_str_eq(getimagevalue(2000, 0, 1), "2");
	ck_assert_str_eq(getimagevalue(1000000, 0, 1), "1");
	ck_assert_str_eq(getimagevalue(1000000000, 0, 1), "1");
	ck_assert_str_eq(getimagevalue(1000000000000ULL, 0, 1), "1");
}
END_TEST

START_TEST(getimagescale_zero)
{
	ck_assert_str_eq(getimagescale(0, 0), "--");
	ck_assert_str_eq(getimagescale(0, 1), "--");
}
END_TEST

START_TEST(getimagescale_normal)
{
	ck_assert_str_eq(getimagescale(1, 0), "B");
	ck_assert_str_eq(getimagescale(2, 0), "B");
	ck_assert_str_eq(getimagescale(10, 0), "B");
	ck_assert_str_eq(getimagescale(100, 0), "B");
	ck_assert_str_eq(getimagescale(1000, 0), "KiB");
	ck_assert_str_eq(getimagescale(1024, 0), "KiB");
	ck_assert_str_eq(getimagescale(1030, 0), "KiB");
	ck_assert_str_eq(getimagescale(1024000, 0), "MiB");
	ck_assert_str_eq(getimagescale(1048576000, 0), "GiB");
	ck_assert_str_eq(getimagescale(1073741824000ULL, 0), "TiB");
}
END_TEST

START_TEST(getimagescale_rate)
{
	ck_assert_str_eq(getimagescale(1, 1), "B/s");
	ck_assert_str_eq(getimagescale(2, 1), "B/s");
	ck_assert_str_eq(getimagescale(10, 1), "B/s");
	ck_assert_str_eq(getimagescale(100, 1), "B/s");
	ck_assert_str_eq(getimagescale(1000, 1), "KiB/s");
	ck_assert_str_eq(getimagescale(1024, 1), "KiB/s");
	ck_assert_str_eq(getimagescale(1030, 1), "KiB/s");
	ck_assert_str_eq(getimagescale(1024000, 1), "MiB/s");
	ck_assert_str_eq(getimagescale(1048576000, 1), "GiB/s");
	ck_assert_str_eq(getimagescale(1073741824000ULL, 1), "TiB/s");
}
END_TEST

START_TEST(getscale_zero)
{
	ck_assert_int_eq(getscale(0, 0), 1);
}
END_TEST

START_TEST(getscale_nonzero_1024)
{
	cfg.rateunit = 0;
	cfg.rateunitmode = 0;
	ck_assert_int_eq(getscale(1, 0), 1);
	ck_assert_int_eq(getscale(2, 0), 1);
	ck_assert_int_eq(getscale(10, 0), 2);
	ck_assert_int_eq(getscale(20, 0), 5);
	ck_assert_int_eq(getscale(50, 0), 20);
	ck_assert_int_eq(getscale(1000, 0), 300);
	ck_assert_int_eq(getscale(1023, 0), 300);
	ck_assert_int_eq(getscale(1024, 0), 1024);
	ck_assert_int_eq(getscale(1025, 0), 1024);
	ck_assert_int_eq(getscale(1026, 0), 1024);
	ck_assert_int_eq(getscale(1500, 0), 1024);
	ck_assert_int_eq(getscale(2047, 0), 1024);
	ck_assert_int_eq(getscale(2048, 0), 1024);
	ck_assert_int_eq(getscale(2049, 0), 1024);
	ck_assert_int_eq(getscale(8191, 0), 1024);
	ck_assert_int_eq(getscale(8192, 0), 2048);
	ck_assert_int_eq(getscale(8193, 0), 2048);
	ck_assert_int_eq(getscale(20000, 0), 4096);

	ck_assert_int_eq(getscale(1, 1), 1);
	ck_assert_int_eq(getscale(2, 1), 1);
	ck_assert_int_eq(getscale(10, 1), 2);
	ck_assert_int_eq(getscale(20, 1), 5);
	ck_assert_int_eq(getscale(50, 1), 20);
	ck_assert_int_eq(getscale(1000, 1), 300);
	ck_assert_int_eq(getscale(1023, 1), 300);
	ck_assert_int_eq(getscale(1024, 1), 1024);
	ck_assert_int_eq(getscale(1025, 1), 1024);
	ck_assert_int_eq(getscale(1026, 1), 1024);
	ck_assert_int_eq(getscale(1500, 1), 1024);
	ck_assert_int_eq(getscale(2047, 1), 1024);
	ck_assert_int_eq(getscale(2048, 1), 1024);
	ck_assert_int_eq(getscale(2049, 1), 1024);
	ck_assert_int_eq(getscale(8191, 1), 1024);
	ck_assert_int_eq(getscale(8192, 1), 2048);
	ck_assert_int_eq(getscale(8193, 1), 2048);
	ck_assert_int_eq(getscale(20000, 1), 4096);
	ck_assert_int_eq(getscale(720000, 1), 204800);
}
END_TEST

START_TEST(getscale_nonzero_1000)
{
	cfg.rateunit = 1;
	cfg.rateunitmode = 1;
	ck_assert_int_eq(getscale(1, 0), 1);
	ck_assert_int_eq(getscale(2, 0), 1);
	ck_assert_int_eq(getscale(10, 0), 2);
	ck_assert_int_eq(getscale(20, 0), 5);
	ck_assert_int_eq(getscale(50, 0), 20);
	ck_assert_int_eq(getscale(1000, 0), 300);
	ck_assert_int_eq(getscale(1023, 0), 300);
	ck_assert_int_eq(getscale(1024, 0), 1024);
	ck_assert_int_eq(getscale(1025, 0), 1024);
	ck_assert_int_eq(getscale(1026, 0), 1024);
	ck_assert_int_eq(getscale(1500, 0), 1024);
	ck_assert_int_eq(getscale(2047, 0), 1024);
	ck_assert_int_eq(getscale(2048, 0), 1024);
	ck_assert_int_eq(getscale(2049, 0), 1024);
	ck_assert_int_eq(getscale(8191, 0), 1024);
	ck_assert_int_eq(getscale(8192, 0), 2048);
	ck_assert_int_eq(getscale(8193, 0), 2048);
	ck_assert_int_eq(getscale(20000, 0), 4096);

	ck_assert_int_eq(getscale(1, 1), 1);
	ck_assert_int_eq(getscale(2, 1), 1);
	ck_assert_int_eq(getscale(10, 1), 2);
	ck_assert_int_eq(getscale(20, 1), 5);
	ck_assert_int_eq(getscale(50, 1), 20);
	ck_assert_int_eq(getscale(1000, 1), 1000);
	ck_assert_int_eq(getscale(1023, 1), 1000);
	ck_assert_int_eq(getscale(1024, 1), 1000);
	ck_assert_int_eq(getscale(1025, 1), 1000);
	ck_assert_int_eq(getscale(1026, 1), 1000);
	ck_assert_int_eq(getscale(1500, 1), 1000);
	ck_assert_int_eq(getscale(2047, 1), 1000);
	ck_assert_int_eq(getscale(2048, 1), 1000);
	ck_assert_int_eq(getscale(2049, 1), 1000);
	ck_assert_int_eq(getscale(8191, 1), 2000);
	ck_assert_int_eq(getscale(8192, 1), 2000);
	ck_assert_int_eq(getscale(8193, 1), 2000);
	ck_assert_int_eq(getscale(20000, 1), 5000);
}
END_TEST

/* this function needs to match the logic used in image.c drawhours() */
/* in order to test the right thing */
char *hourly_imagescale_logic(const uint64_t max, const int rate)
{
	int i, step, s, prev = 0;
	uint64_t scaleunit;

	scaleunit = getscale(max, rate);
	if (max / scaleunit > 4) {
		step = 2;
	} else {
		step = 1;
	}

	for (i=step; (uint64_t)(scaleunit * i) <= max; i=i+step) {
		s = 121 * ((scaleunit * i) / (float)max);
		prev = s;
	}

	s = 121 * ((scaleunit * i) / (float)max);
	if ( ((s+prev)/2) <= 128 ) {
		;
	} else {
		i = i - step;
	}

	/* debug for times when things don't appear to make sense */
	/*printf("\nmax:        %"PRIu64"\n", max);
	printf("scaleunit:  %"PRIu64"\n", scaleunit);
	printf("old 2.0:    %"PRIu64" (i: %d, step: %d)\n", scaleunit * (i - step), i, step);
	printf("now:        %"PRIu64" (i: %d, step: %d)\n", scaleunit * i, i, step);*/

	return getimagescale(scaleunit * i, rate);
}

START_TEST(hourly_imagescaling_normal)
{
	char *unittext;

	unittext = hourly_imagescale_logic(1, 0);
	ck_assert_str_eq(unittext, "B");

	unittext = hourly_imagescale_logic(100, 0);
	ck_assert_str_eq(unittext, "B");

	unittext = hourly_imagescale_logic(981, 0);
	ck_assert_str_eq(unittext, "B");

	unittext = hourly_imagescale_logic(1000, 0);
	ck_assert_str_eq(unittext, "KiB");

	unittext = hourly_imagescale_logic(1024, 0);
	ck_assert_str_eq(unittext, "KiB");

	unittext = hourly_imagescale_logic(2000, 0);
	ck_assert_str_eq(unittext, "KiB");

	unittext = hourly_imagescale_logic(1000000, 0);
	ck_assert_str_eq(unittext, "KiB");

	unittext = hourly_imagescale_logic(1024000, 0);
	ck_assert_str_eq(unittext, "MiB");

	unittext = hourly_imagescale_logic(1300000, 0);
	ck_assert_str_eq(unittext, "MiB");

	unittext = hourly_imagescale_logic(2000000, 0);
	ck_assert_str_eq(unittext, "MiB");

	unittext = hourly_imagescale_logic(1000000000, 0);
	ck_assert_str_eq(unittext, "MiB");

	unittext = hourly_imagescale_logic(2000000000, 0);
	ck_assert_str_eq(unittext, "GiB");

	unittext = hourly_imagescale_logic(2000000000000ULL, 0);
	ck_assert_str_eq(unittext, "TiB");
}
END_TEST

START_TEST(hourly_imagescaling_rate_1024)
{
	char *unittext;

	cfg.rateunit = 0;
	cfg.rateunitmode = 0;

	unittext = hourly_imagescale_logic(1, 1);
	ck_assert_str_eq(unittext, "B/s");

	unittext = hourly_imagescale_logic(100, 1);
	ck_assert_str_eq(unittext, "B/s");

	unittext = hourly_imagescale_logic(981, 1);
	ck_assert_str_eq(unittext, "B/s");

	unittext = hourly_imagescale_logic(1000, 1);
	ck_assert_str_eq(unittext, "KiB/s");

	unittext = hourly_imagescale_logic(1024, 1);
	ck_assert_str_eq(unittext, "KiB/s");

	unittext = hourly_imagescale_logic(2000, 1);
	ck_assert_str_eq(unittext, "KiB/s");

	unittext = hourly_imagescale_logic(1000000, 1);
	ck_assert_str_eq(unittext, "KiB/s");

	unittext = hourly_imagescale_logic(1024000, 1);
	ck_assert_str_eq(unittext, "MiB/s");

	unittext = hourly_imagescale_logic(1300000, 1);
	ck_assert_str_eq(unittext, "MiB/s");

	unittext = hourly_imagescale_logic(2000000, 1);
	ck_assert_str_eq(unittext, "MiB/s");

	unittext = hourly_imagescale_logic(1000000000, 1);
	ck_assert_str_eq(unittext, "MiB/s");

	unittext = hourly_imagescale_logic(2000000000, 1);
	ck_assert_str_eq(unittext, "GiB/s");

	unittext = hourly_imagescale_logic(2000000000000ULL, 1);
	ck_assert_str_eq(unittext, "TiB/s");
}
END_TEST

START_TEST(hourly_imagescaling_rate_1000)
{
	char *unittext;

	cfg.rateunit = 1;
	cfg.rateunitmode = 1;

	unittext = hourly_imagescale_logic(1, 1);
	ck_assert_str_eq(unittext, "bit/s");

	unittext = hourly_imagescale_logic(100, 1);
	ck_assert_str_eq(unittext, "bit/s");

	unittext = hourly_imagescale_logic(981, 1);
	ck_assert_str_eq(unittext, "bit/s");

	unittext = hourly_imagescale_logic(1000, 1);
	ck_assert_str_eq(unittext, "kbit/s");

	unittext = hourly_imagescale_logic(1024, 1);
	ck_assert_str_eq(unittext, "kbit/s");

	unittext = hourly_imagescale_logic(2000, 1);
	ck_assert_str_eq(unittext, "kbit/s");

	unittext = hourly_imagescale_logic(1000000, 1);
	ck_assert_str_eq(unittext, "Mbit/s");

	unittext = hourly_imagescale_logic(1024000, 1);
	ck_assert_str_eq(unittext, "Mbit/s");

	unittext = hourly_imagescale_logic(1300000, 1);
	ck_assert_str_eq(unittext, "Mbit/s");

	unittext = hourly_imagescale_logic(2000000, 1);
	ck_assert_str_eq(unittext, "Mbit/s");

	unittext = hourly_imagescale_logic(1000000000, 1);
	ck_assert_str_eq(unittext, "Gbit/s");

	unittext = hourly_imagescale_logic(2000000000, 1);
	ck_assert_str_eq(unittext, "Gbit/s");

	unittext = hourly_imagescale_logic(2000000000000ULL, 1);
	ck_assert_str_eq(unittext, "Tbit/s");
}
END_TEST

void add_image_tests(Suite *s)
{
	TCase *tc_image = tcase_create("Image");
	tcase_add_test(tc_image, initimagecontent_does_not_crash);
	tcase_add_test(tc_image, colorinit_does_not_crash);
	tcase_add_test(tc_image, layoutinit_does_not_crash);
	tcase_add_test(tc_image, getimagevalue_zeropadding);
	tcase_add_test(tc_image, getimagevalue_normal);
	tcase_add_test(tc_image, getimagevalue_rate_1024);
	tcase_add_test(tc_image, getimagevalue_rate_1000);
	tcase_add_test(tc_image, getimagescale_zero);
	tcase_add_test(tc_image, getimagescale_normal);
	tcase_add_test(tc_image, getimagescale_rate);
	tcase_add_test(tc_image, getscale_zero);
	tcase_add_test(tc_image, getscale_nonzero_1024);
	tcase_add_test(tc_image, getscale_nonzero_1000);
	tcase_add_test(tc_image, hourly_imagescaling_normal);
	tcase_add_test(tc_image, hourly_imagescaling_rate_1024);
	tcase_add_test(tc_image, hourly_imagescaling_rate_1000);
	suite_add_tcase(s, tc_image);
}
