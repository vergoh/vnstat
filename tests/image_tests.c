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

START_TEST(imageinit_does_not_crash)
{
	IMAGECONTENT ic;
	defaultcfg();
	imageinit(&ic, 2, 2);
	gdImageDestroy(ic.im);
}
END_TEST

START_TEST(layoutinit_does_not_crash)
{
	IMAGECONTENT ic;
	defaultcfg();
	initimagecontent(&ic);
	imageinit(&ic, 640, 480);
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
	cfg.rateunit = 0;
	ck_assert_str_eq(getimagescale(0, 0), "--");
	ck_assert_str_eq(getimagescale(0, 1), "--");
}
END_TEST

START_TEST(getimagescale_normal)
{
	cfg.rateunit = 0;
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
	cfg.rateunit = 0;
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
	// int i, prev = 0;
	int step, s, extray = 0;
	uint64_t scaleunit;

	scaleunit = getscale(max, rate);

	s = (int)lrint(((double)scaleunit / (double)max) * (124 + extray));
	if (s < SCALEMINPIXELS) {
		step = 2;
	} else {
		step = 1;
	}
	/*
	for (i = step; i * s <= (124 + extray + 4); i = i + step) {
		prev = i * s;
	}
	*/
	/* debug for times when things don't appear to make sense */
	/*printf("\nrate:       %d\n", rate);
	printf("lines:      %d\n", i-1);
	printf("max:        %"PRIu64"\n", max);
	printf("scaleunit:  %"PRIu64"\n", scaleunit);
	printf("old 2.0:    %"PRIu64" (i: %d, step: %d)\n", scaleunit * (i - step), i, step);
	printf("old 2.6:    %"PRIu64" (i: %d, step: %d)\n", scaleunit * i, i, step);
	printf("now:        %"PRIu64" (i: %d, step: %d)\n", scaleunit * step, i, step);
	fflush(stdout);*/

	return getimagescale(scaleunit * (unsigned int)step, rate);
}

START_TEST(hourly_imagescaling_normal)
{
	char *unittext;

	cfg.unitmode = 0;
	cfg.rateunit = 1;
	cfg.rateunitmode = 1;

	unittext = hourly_imagescale_logic(1, 0);
	ck_assert_str_eq(unittext, "B");

	unittext = hourly_imagescale_logic(100, 0);
	ck_assert_str_eq(unittext, "B");

	unittext = hourly_imagescale_logic(981, 0);
	ck_assert_str_eq(unittext, "B");

	unittext = hourly_imagescale_logic(1000, 0);
	ck_assert_str_eq(unittext, "B");

	unittext = hourly_imagescale_logic(1024, 0);
	ck_assert_str_eq(unittext, "KiB");

	unittext = hourly_imagescale_logic(2000, 0);
	ck_assert_str_eq(unittext, "KiB");

	unittext = hourly_imagescale_logic(1000000, 0);
	ck_assert_str_eq(unittext, "KiB");

	unittext = hourly_imagescale_logic(1024000, 0);
	ck_assert_str_eq(unittext, "KiB");

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

	cfg.unitmode = 0;
	cfg.rateunit = 0;
	cfg.rateunitmode = 0;

	unittext = hourly_imagescale_logic(1, 1);
	ck_assert_str_eq(unittext, "B/s");

	unittext = hourly_imagescale_logic(100, 1);
	ck_assert_str_eq(unittext, "B/s");

	unittext = hourly_imagescale_logic(981, 1);
	ck_assert_str_eq(unittext, "B/s");

	unittext = hourly_imagescale_logic(1000, 1);
	ck_assert_str_eq(unittext, "B/s");

	unittext = hourly_imagescale_logic(1024, 1);
	ck_assert_str_eq(unittext, "KiB/s");

	unittext = hourly_imagescale_logic(2000, 1);
	ck_assert_str_eq(unittext, "KiB/s");

	unittext = hourly_imagescale_logic(1000000, 1);
	ck_assert_str_eq(unittext, "KiB/s");

	unittext = hourly_imagescale_logic(1024000, 1);
	ck_assert_str_eq(unittext, "KiB/s");

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

	cfg.unitmode = 0;
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

START_TEST(libgd_output_comparison)
{
	int ret, x, y;
	IMAGECONTENT ic;
	FILE *pngout;

	x = 1060;
	y = 420;

	defaultcfg();
	initimagecontent(&ic);
	imageinit(&ic, x, y);
	ic.interface.updated = (time_t)get_timestamp(2001, 2, 3, 4, 5);
	layoutinit(&ic, "vnstati libgd output comparison", x, y);

	pngout = fopen("vnstati_libgd_comparison_check.png", "w");
	ck_assert_ptr_ne(pngout, NULL);

	drawlegend(&ic, 40, 30, 0);
	drawlegend(&ic, 240, 30, 1);

	/* line 1 */
	x = 40;
	y = 80;
	gdImageStringUp(ic.im, gdFontGetSmall(), 1, y + 105, (unsigned char *)"libgd bug workaround", ic.ctext);
	drawdonut(&ic, x, y, (float)0, (float)0, 49, 15);
	drawdonut(&ic, x + 55, y, (float)50, (float)50, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"0/0 - 50/50", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)100, (float)0, 49, 15);
	drawdonut(&ic, x + 55, y, (float)0, (float)100, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"100/0 - 0/100", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)60, (float)40, 49, 15);
	drawdonut(&ic, x + 55, y, (float)40, (float)60, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"60/40 - 40/60", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)75, (float)25, 49, 15);
	drawdonut(&ic, x + 55, y, (float)25, (float)75, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"75/25 - 25/75", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)90, (float)10, 49, 15);
	drawdonut(&ic, x + 55, y, (float)10, (float)90, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"90/10 - 10/90", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)95, (float)5, 49, 15);
	drawdonut(&ic, x + 55, y, (float)5, (float)95, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"95/5 - 5/95", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)99, (float)1, 49, 15);
	drawdonut(&ic, x + 55, y, (float)1, (float)99, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"99/1 - 1/99", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)99.9, (float)0.1, 49, 15);
	drawdonut(&ic, x + 55, y, (float)0.1, (float)99.9, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"99.9/0.1 - 0.1/99.9", ic.ctext);

	/* line 2 */
	x = 40;
	y = 160;
	drawdonut(&ic, x, y, (float)0, (float)0, 49, 15);
	drawdonut(&ic, x + 55, y, (float)25, (float)25, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"0/0 - 25/25", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)50, (float)0, 49, 15);
	drawdonut(&ic, x + 55, y, (float)0, (float)50, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"50/0 - 0/50", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)40, (float)30, 49, 15);
	drawdonut(&ic, x + 55, y, (float)30, (float)40, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"40/30 - 30/40", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)30, (float)20, 49, 15);
	drawdonut(&ic, x + 55, y, (float)20, (float)30, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"30/20 - 20/30", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)20, (float)10, 49, 15);
	drawdonut(&ic, x + 55, y, (float)10, (float)20, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"20/10 - 10/20", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)15, (float)5, 49, 15);
	drawdonut(&ic, x + 55, y, (float)5, (float)15, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"15/5 - 5/15", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)10, (float)1, 49, 15);
	drawdonut(&ic, x + 55, y, (float)1, (float)10, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"10/1 - 1/10", ic.ctext);

	x += 130;
	drawdonut(&ic, x, y, (float)1, (float)0.1, 49, 15);
	drawdonut(&ic, x + 55, y, (float)0.1, (float)1, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"1.0/0.1 - 0.1/1.0", ic.ctext);

	/* line 3 */
	x = 40;
	y = 270;
	gdImageStringUp(ic.im, gdFontGetSmall(), 1, y + 105, (unsigned char *)"libgd native", ic.ctext);
	drawdonut_libgd_native(&ic, x, y, (float)0, (float)0, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)50, (float)50, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"0/0 - 50/50", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)100, (float)0, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)0, (float)100, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"100/0 - 0/100", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)60, (float)40, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)40, (float)60, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"60/40 - 40/60", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)75, (float)25, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)25, (float)75, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"75/25 - 25/75", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)90, (float)10, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)10, (float)90, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"90/10 - 10/90", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)95, (float)5, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)5, (float)95, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"95/5 - 5/95", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)99, (float)1, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)1, (float)99, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"99/1 - 1/99", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)99.9, (float)0.1, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)0.1, (float)99.9, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"99.9/0.1 - 0.1/99.9", ic.ctext);

	/* line 4 */
	x = 40;
	y = 350;
	drawdonut_libgd_native(&ic, x, y, (float)0, (float)0, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)25, (float)25, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"0/0 - 25/25", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)50, (float)0, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)0, (float)50, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"50/0 - 0/50", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)40, (float)30, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)30, (float)40, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"40/30 - 30/40", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)30, (float)20, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)20, (float)30, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"30/20 - 20/30", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)20, (float)10, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)10, (float)20, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"20/10 - 10/20", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)15, (float)5, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)5, (float)15, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"15/5 - 5/15", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)10, (float)1, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)1, (float)10, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"10/1 - 1/10", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)1, (float)0.1, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)0.1, (float)1, 49, 15);
	gdImageString(ic.im, gdFontGetSmall(), x - 20, y + 30, (unsigned char *)"1.0/0.1 - 0.1/1.0", ic.ctext);

	gdImagePng(ic.im, pngout);
	ret = fclose(pngout);
	ck_assert_int_eq(ret, 0);
	gdImageDestroy(ic.im);
}
END_TEST

START_TEST(element_output_check)
{
	int ret, x, y, i;
	float f;
	char buffer[6];
	IMAGECONTENT ic;
	FILE *pngout;

	x = 1500;
	y = 900;

	defaultcfg();
	initimagecontent(&ic);
	imageinit(&ic, x, y);
	ic.interface.updated = (time_t)get_timestamp(2012, 3, 4, 5, 6);
	layoutinit(&ic, "donut with 0.2% input steps and other elements", x, y);

	pngout = fopen("vnstati_element_check.png", "w");
	ck_assert_ptr_ne(pngout, NULL);

	x = 40;
	y = 70;

	gdImageStringUp(ic.im, gdFontGetSmall(), 1, y + 15, (unsigned char *)"50.0%", ic.ctext);

	for (f = 50.0; f >= 0; f -= (float)0.2) {

		drawdonut(&ic, x, y, f, f, 49, 15);
		x += 55;

		if (x > 1000) {
			x = 40;
			y += 60;

			snprintf(buffer, 6, "%3.1f%%", (double)f - 0.2);
			gdImageStringUp(ic.im, gdFontGetSmall(), 1, y + 15, (unsigned char *)buffer, ic.ctext);
		}
	}

	gdImageString(ic.im, gdFontGetGiant(), 1020, 40, (unsigned char *)"Giant - The quick brown fox jumps over the lazy dog", ic.ctext);
	gdImageString(ic.im, gdFontGetLarge(), 1020, 60, (unsigned char *)"Large - The quick brown fox jumps over the lazy dog", ic.ctext);
	gdImageString(ic.im, gdFontGetMediumBold(), 1020, 80, (unsigned char *)"MediumBold - The quick brown fox jumps over the lazy dog", ic.ctext);
	gdImageString(ic.im, gdFontGetSmall(), 1020, 100, (unsigned char *)"Small - The quick brown fox jumps over the lazy dog", ic.ctext);
	gdImageString(ic.im, gdFontGetTiny(), 1020, 120, (unsigned char *)"Tiny - The quick brown fox jumps over the lazy dog", ic.ctext);

	drawlegend(&ic, 1130, 140, 0);
	drawlegend(&ic, 1330, 140, 1);

	drawbar(&ic, 1050, 160, 400, 50, 50, 100, 0);
	drawbar(&ic, 1050, 180, 400, 25, 75, 100, 0);
	drawbar(&ic, 1050, 200, 400, 75, 25, 100, 0);
	drawbar(&ic, 1050, 220, 400, 0, 100, 100, 0);
	drawbar(&ic, 1050, 240, 400, 100, 0, 100, 0);

	drawbar(&ic, 1050, 260, 400, 50, 50, 130, 0);
	drawbar(&ic, 1050, 280, 400, 25, 75, 130, 0);
	drawbar(&ic, 1050, 300, 400, 75, 25, 130, 0);
	drawbar(&ic, 1050, 320, 400, 0, 100, 130, 0);
	drawbar(&ic, 1050, 340, 400, 100, 0, 130, 0);

	gdImageLine(ic.im, 1040, 360, 1260, 360, ic.ctext);
	gdImageLine(ic.im, 1040, 760, 1260, 760, ic.ctext);

	gdImageLine(ic.im, 1250, 350, 1250, 770, ic.ctext);

	drawarrowup(&ic, 1250, 350);
	drawarrowright(&ic, 1260, 360);

	drawpoles(&ic, 1050, 360, 400, 50, 50, 100);
	drawpoles(&ic, 1070, 360, 400, 25, 75, 100);
	drawpoles(&ic, 1090, 360, 400, 75, 25, 100);
	drawpoles(&ic, 1110, 360, 400, 0, 100, 100);
	drawpoles(&ic, 1130, 360, 400, 100, 0, 100);

	drawpoles(&ic, 1150, 360, 400, 50, 50, 130);
	drawpoles(&ic, 1170, 360, 400, 25, 75, 130);
	drawpoles(&ic, 1190, 360, 400, 75, 25, 130);
	drawpoles(&ic, 1210, 360, 400, 0, 100, 130);
	drawpoles(&ic, 1230, 360, 400, 100, 0, 130);

	gdImageLine(ic.im, 1040, 870, 1160, 870, ic.ctext);
	gdImageLine(ic.im, 1040, 820, 1160, 820, ic.ctext);
	gdImageLine(ic.im, 1040, 770, 1160, 770, ic.ctext);

	for (i = 0; i < 100; i++) {
		drawpole(&ic, 1050 + i, 819, i % 50, 1, ic.crx);
		drawpole(&ic, 1050 + i, 821, i % 50, 2, ic.ctx);
	}

	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 400, (unsigned char *)"Color: ctext", ic.ctext);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 420, (unsigned char *)"Color: cedge", ic.cedge);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 440, (unsigned char *)"Color: cheader", ic.cheader);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 460, (unsigned char *)"Color: cheadertitle", ic.cheadertitle);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 480, (unsigned char *)"Color: cheaderdate", ic.cheaderdate);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 500, (unsigned char *)"Color: cline", ic.cline);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 520, (unsigned char *)"Color: clinel", ic.clinel);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 540, (unsigned char *)"Color: cbackground", ic.cbackground);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 560, (unsigned char *)"Color: cvnstat", ic.cvnstat);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 580, (unsigned char *)"Color: cbgoffset", ic.cbgoffset);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 600, (unsigned char *)"Color: crx", ic.crx);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 620, (unsigned char *)"Color: crxd", ic.crxd);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 640, (unsigned char *)"Color: ctx", ic.ctx);
	gdImageString(ic.im, gdFontGetMediumBold(), 1280, 660, (unsigned char *)"Color: ctxd", ic.ctxd);

	gdImagePng(ic.im, pngout);
	ret = fclose(pngout);
	ck_assert_int_eq(ret, 0);
	gdImageDestroy(ic.im);
}
END_TEST

void add_image_tests(Suite *s)
{
	TCase *tc_image = tcase_create("Image");
	tcase_add_checked_fixture(tc_image, setup, teardown);
	tcase_add_unchecked_fixture(tc_image, setup, teardown);
	tcase_set_timeout(tc_image, 10);
	tcase_add_test(tc_image, initimagecontent_does_not_crash);
	tcase_add_test(tc_image, imageinit_does_not_crash);
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
	tcase_add_test(tc_image, libgd_output_comparison);
	tcase_add_test(tc_image, element_output_check);
	suite_add_tcase(s, tc_image);
}
