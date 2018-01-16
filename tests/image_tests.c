#include "vnstat_tests.h"
#include "image_tests.h"
#include "common.h"
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

START_TEST(getimagevalue_rate)
{
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

void add_image_tests(Suite *s)
{
	TCase *tc_image = tcase_create("Image");
	tcase_add_test(tc_image, initimagecontent_does_not_crash);
	tcase_add_test(tc_image, colorinit_does_not_crash);
	tcase_add_test(tc_image, layoutinit_does_not_crash);
	tcase_add_test(tc_image, getimagevalue_zeropadding);
	tcase_add_test(tc_image, getimagevalue_normal);
	tcase_add_test(tc_image, getimagevalue_rate);
	tcase_add_test(tc_image, getimagescale_zero);
	tcase_add_test(tc_image, getimagescale_normal);
	tcase_add_test(tc_image, getimagescale_rate);
	tcase_add_test(tc_image, getscale_zero);
	tcase_add_test(tc_image, getscale_nonzero);
	suite_add_tcase(s, tc_image);
}
