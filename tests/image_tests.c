#include "common.h"
#include "vnstat_tests.h"
#include "image_tests.h"
#include "dbsql.h"
#include "cfg.h"
#include "image.h"
#include "image_font.h"
#include "image_widget.h"
#include "image_support.h"
#include "image_summary.h"
#include "image_graph.h"
#include <stdlib.h>
#include <unistd.h>

static const int image_output_qmodes[] = {
	1, 2, 3, 4, 5, 51, 52, 7, 8, 9, 10, 130, 131, 132
};

static void assert_drawimage_produces_image(IMAGECONTENT *ic)
{
	drawimage(ic);
	ck_assert_ptr_ne(ic->im, NULL);
	ck_assert_int_gt(gdImageSX(ic->im), 0);
	ck_assert_int_gt(gdImageSY(ic->im), 0);
	gdImageDestroy(ic->im);
	ic->im = NULL;
}

START_TEST(initimagecontent_does_not_crash)
{
	IMAGECONTENT ic;
	initimagecontent(&ic);
}
END_TEST

START_TEST(imageinit_does_not_crash)
{
	IMAGECONTENT ic;
	imageinit(&ic, 2, 2);
	gdImageDestroy(ic.im);
}
END_TEST

START_TEST(layoutinit_does_not_crash)
{
	IMAGECONTENT ic;
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

/* this function needs to match the logic used in image_graph.c drawhours() */
/* in order to test the right thing */
char *hourly_imagescale_logic(const uint64_t max, const int rate)
{
	// int i, prev = 0;
	int step = 0, s, extray = 0;
	uint64_t scaleunit;

	scaleunit = getscale(max, rate);

	s = (int)lrint(((double)scaleunit / (double)max) * (124 + extray));
	if (s == 0) {
		s = 1;
	}
	while (s * step < SCALEMINPIXELS) {
		step++;
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
	imagestringup(&ic, FONT_ROLE_AXIS, 1, y + 105, "libgd bug workaround", ic.ctext);
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)0, (float)0, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)50, (float)50, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "0/0 - 50/50", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)100, (float)0, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)0, (float)100, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "100/0 - 0/100", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)60, (float)40, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)40, (float)60, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "60/40 - 40/60", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)75, (float)25, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)25, (float)75, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "75/25 - 25/75", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)90, (float)10, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)10, (float)90, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "90/10 - 10/90", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)95, (float)5, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)5, (float)95, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "95/5 - 5/95", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)99, (float)1, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)1, (float)99, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "99/1 - 1/99", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)99.9, (float)0.1, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)0.1, (float)99.9, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "99.9/0.1 - 0.1/99.9", ic.ctext);

	/* line 2 */
	x = 40;
	y = 160;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)0, (float)0, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)25, (float)25, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "0/0 - 25/25", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)50, (float)0, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)0, (float)50, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "50/0 - 0/50", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)40, (float)30, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)30, (float)40, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "40/30 - 30/40", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)30, (float)20, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)20, (float)30, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "30/20 - 20/30", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)20, (float)10, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)10, (float)20, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "20/10 - 10/20", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)15, (float)5, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)5, (float)15, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "15/5 - 5/15", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)10, (float)1, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)1, (float)10, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "10/1 - 1/10", ic.ctext);

	x += 130;
	drawdonut_libgd_bug_workaround(&ic, x, y, (float)1, (float)0.1, 49, 15);
	drawdonut_libgd_bug_workaround(&ic, x + 55, y, (float)0.1, (float)1, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "1.0/0.1 - 0.1/1.0", ic.ctext);

	/* line 3 */
	x = 40;
	y = 270;
	imagestringup(&ic, FONT_ROLE_AXIS, 1, y + 105, "libgd native", ic.ctext);
	drawdonut_libgd_native(&ic, x, y, (float)0, (float)0, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)50, (float)50, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "0/0 - 50/50", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)100, (float)0, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)0, (float)100, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "100/0 - 0/100", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)60, (float)40, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)40, (float)60, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "60/40 - 40/60", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)75, (float)25, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)25, (float)75, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "75/25 - 25/75", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)90, (float)10, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)10, (float)90, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "90/10 - 10/90", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)95, (float)5, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)5, (float)95, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "95/5 - 5/95", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)99, (float)1, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)1, (float)99, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "99/1 - 1/99", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)99.9, (float)0.1, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)0.1, (float)99.9, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "99.9/0.1 - 0.1/99.9", ic.ctext);

	/* line 4 */
	x = 40;
	y = 350;
	drawdonut_libgd_native(&ic, x, y, (float)0, (float)0, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)25, (float)25, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "0/0 - 25/25", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)50, (float)0, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)0, (float)50, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "50/0 - 0/50", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)40, (float)30, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)30, (float)40, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "40/30 - 30/40", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)30, (float)20, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)20, (float)30, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "30/20 - 20/30", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)20, (float)10, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)10, (float)20, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "20/10 - 10/20", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)15, (float)5, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)5, (float)15, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "15/5 - 5/15", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)10, (float)1, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)1, (float)10, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "10/1 - 1/10", ic.ctext);

	x += 130;
	drawdonut_libgd_native(&ic, x, y, (float)1, (float)0.1, 49, 15);
	drawdonut_libgd_native(&ic, x + 55, y, (float)0.1, (float)1, 49, 15);
	imagestring(&ic, FONT_ROLE_BODY, x - 20, y + 30, "1.0/0.1 - 0.1/1.0", ic.ctext);

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

	initimagecontent(&ic);
	imageinit(&ic, x, y);
	ic.interface.updated = (time_t)get_timestamp(2012, 3, 4, 5, 6);
	layoutinit(&ic, "donut with 0.2% input steps and other elements", x, y);

	pngout = fopen("vnstati_element_check.png", "w");
	ck_assert_ptr_ne(pngout, NULL);

	x = 40;
	y = 70;

	imagestringup(&ic, FONT_ROLE_AXIS, 1, y + 15, "50.0%", ic.ctext);

	for (f = 50.0; f >= 0; f -= (float)0.2) {

		drawdonut(&ic, x, y, f, f, 49, 15);
		x += 55;

		if (x > 1000) {
			x = 40;
			y += 60;

			snprintf(buffer, 6, "%3.1f%%", (double)f - 0.2);
			imagestringup(&ic, FONT_ROLE_AXIS, 1, y + 15, buffer, ic.ctext);
		}
	}

	gdImageString(ic.im, gdFontGetGiant(), 1020, 40, (unsigned char *)"Giant - The quick brown fox jumps over the lazy dog", ic.ctext);
	gdImageString(ic.im, gdFontGetLarge(), 1020, 60, (unsigned char *)"Large - The quick brown fox jumps over the lazy dog", ic.ctext);
	gdImageString(ic.im, gdFontGetMediumBold(), 1020, 80, (unsigned char *)"MediumBold - The quick brown fox jumps over the lazy dog", ic.ctext);
	gdImageString(ic.im, gdFontGetSmall(), 1020, 100, (unsigned char *)"Small - The quick brown fox jumps over the lazy dog", ic.ctext);
	gdImageString(ic.im, gdFontGetTiny(), 1020, 120, (unsigned char *)"Tiny - The quick brown fox jumps over the lazy dog", ic.ctext);

	drawlegend(&ic, 1130, 140, 0);
	drawlegend(&ic, 1330, 140, 1);

	drawbar(&ic, 1050, 160, 100, 50, 50, 100, 0);
	drawbar(&ic, 1050, 180, 100, 25, 75, 100, 0);
	drawbar(&ic, 1050, 200, 100, 75, 25, 100, 0);
	drawbar(&ic, 1050, 220, 100, 0, 100, 100, 0);
	drawbar(&ic, 1050, 240, 100, 100, 0, 100, 0);

	drawbar(&ic, 1050, 260, 100, 1, 99, 100, 0);
	drawbar(&ic, 1050, 280, 100, 2, 98, 100, 0);

	drawbar(&ic, 1050, 300, 100, 99, 1, 100, 0);
	drawbar(&ic, 1050, 320, 100, 98, 2, 100, 0);

	drawbar(&ic, 1200, 160, 100, 1, 0, 100, 0);
	drawbar(&ic, 1200, 180, 100, 2, 0, 100, 0);
	drawbar(&ic, 1200, 200, 100, 3, 0, 100, 0);

	drawbar(&ic, 1200, 220, 100, 0, 1, 100, 0);
	drawbar(&ic, 1200, 240, 100, 0, 2, 100, 0);
	drawbar(&ic, 1200, 260, 100, 0, 3, 100, 0);

	drawbar(&ic, 1200, 280, 100, 25, 25, 100, 0);
	drawbar(&ic, 1200, 300, 100, 10, 30, 100, 0);
	drawbar(&ic, 1200, 320, 100, 30, 10, 100, 0);

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

	imagestring(&ic, FONT_ROLE_BODY, 1280, 400, "Color: ctext", ic.ctext);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 420, "Color: cedge", ic.cedge);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 440, "Color: cheader", ic.cheader);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 460, "Color: cheadertitle", ic.cheadertitle);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 480, "Color: cheaderdate", ic.cheaderdate);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 500, "Color: cline", ic.cline);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 520, "Color: clinel", ic.clinel);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 540, "Color: cbackground", ic.cbackground);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 560, "Color: cvnstat", ic.cvnstat);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 580, "Color: cbgoffset", ic.cbgoffset);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 600, "Color: crx", ic.crx);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 620, "Color: crxd", ic.crxd);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 640, "Color: ctx", ic.ctx);
	imagestring(&ic, FONT_ROLE_BODY, 1280, 660, "Color: ctxd", ic.ctxd);

	gdImagePng(ic.im, pngout);
	ret = fclose(pngout);
	ck_assert_int_eq(ret, 0);
	gdImageDestroy(ic.im);
}
END_TEST

START_TEST(hextorgb_can_convert)
{
	int rgb[3];

	debug = 1;
	suppress_output();

	rgb[0] = 1;
	rgb[1] = 2;
	rgb[2] = 3;
	hextorgb("000000", rgb);
	ck_assert_int_eq(rgb[0], 0);
	ck_assert_int_eq(rgb[1], 0);
	ck_assert_int_eq(rgb[2], 0);

	rgb[0] = 1;
	rgb[1] = 2;
	rgb[2] = 3;
	hextorgb("#000000", rgb);
	ck_assert_int_eq(rgb[0], 0);
	ck_assert_int_eq(rgb[1], 0);
	ck_assert_int_eq(rgb[2], 0);

	rgb[0] = 1;
	rgb[1] = 2;
	rgb[2] = 3;
	hextorgb("FFFFFF", rgb);
	ck_assert_int_eq(rgb[0], 255);
	ck_assert_int_eq(rgb[1], 255);
	ck_assert_int_eq(rgb[2], 255);

	rgb[0] = 1;
	rgb[1] = 2;
	rgb[2] = 3;
	hextorgb("#FFFFFF", rgb);
	ck_assert_int_eq(rgb[0], 255);
	ck_assert_int_eq(rgb[1], 255);
	ck_assert_int_eq(rgb[2], 255);

	rgb[0] = 1;
	rgb[1] = 2;
	rgb[2] = 3;
	hextorgb("ABCABBA", rgb);
	ck_assert_int_eq(rgb[0], 171);
	ck_assert_int_eq(rgb[1], 202);
	ck_assert_int_eq(rgb[2], 187);

	rgb[0] = 1;
	rgb[1] = 2;
	rgb[2] = 3;
	hextorgb("#ABCABBA", rgb);
	ck_assert_int_eq(rgb[0], 171);
	ck_assert_int_eq(rgb[1], 202);
	ck_assert_int_eq(rgb[2], 187);
}
END_TEST

START_TEST(modcolor_mods_colors)
{
	int rgb[3];

	debug = 1;
	suppress_output();

	rgb[0] = 10;
	rgb[1] = 20;
	rgb[2] = 30;
	modcolor(rgb, 10, 0);
	ck_assert_int_eq(rgb[0], 20);
	ck_assert_int_eq(rgb[1], 30);
	ck_assert_int_eq(rgb[2], 40);

	rgb[0] = 10;
	rgb[1] = 20;
	rgb[2] = 30;
	modcolor(rgb, -10, 0);
	ck_assert_int_eq(rgb[0], 0);
	ck_assert_int_eq(rgb[1], 10);
	ck_assert_int_eq(rgb[2], 20);

	rgb[0] = 10;
	rgb[1] = 20;
	rgb[2] = 30;
	modcolor(rgb, -20, 0);
	ck_assert_int_eq(rgb[0], 0);
	ck_assert_int_eq(rgb[1], 0);
	ck_assert_int_eq(rgb[2], 10);

	rgb[0] = 10;
	rgb[1] = 20;
	rgb[2] = 30;
	modcolor(rgb, -30, 0);
	ck_assert_int_eq(rgb[0], 40);
	ck_assert_int_eq(rgb[1], 50);
	ck_assert_int_eq(rgb[2], 60);

	rgb[0] = 10;
	rgb[1] = 20;
	rgb[2] = 30;
	modcolor(rgb, -30, 1);
	ck_assert_int_eq(rgb[0], 0);
	ck_assert_int_eq(rgb[1], 0);
	ck_assert_int_eq(rgb[2], 0);

	rgb[0] = 10;
	rgb[1] = 250;
	rgb[2] = 30;
	modcolor(rgb, 30, 0);
	ck_assert_int_eq(rgb[0], 40);
	ck_assert_int_eq(rgb[1], 255);
	ck_assert_int_eq(rgb[2], 60);

	rgb[0] = 10;
	rgb[1] = 250;
	rgb[2] = 30;
	modcolor(rgb, 30, 1);
	ck_assert_int_eq(rgb[0], 40);
	ck_assert_int_eq(rgb[1], 255);
	ck_assert_int_eq(rgb[2], 60);

	rgb[0] = 10;
	rgb[1] = 250;
	rgb[2] = 251;
	modcolor(rgb, 30, 0);
	ck_assert_int_eq(rgb[0], 0);
	ck_assert_int_eq(rgb[1], 220);
	ck_assert_int_eq(rgb[2], 221);

	rgb[0] = 10;
	rgb[1] = 250;
	rgb[2] = 251;
	modcolor(rgb, 30, 1);
	ck_assert_int_eq(rgb[0], 40);
	ck_assert_int_eq(rgb[1], 255);
	ck_assert_int_eq(rgb[2], 255);

	rgb[0] = 10;
	rgb[1] = 20;
	rgb[2] = 251;
	modcolor(rgb, -30, 0);
	ck_assert_int_eq(rgb[0], 40);
	ck_assert_int_eq(rgb[1], 50);
	ck_assert_int_eq(rgb[2], 255);
}
END_TEST

START_TEST(invertcolor_inverts_colors)
{
	int rgb[3];

	debug = 1;
	suppress_output();

	rgb[0] = 0;
	rgb[1] = 10;
	rgb[2] = 255;
	invertcolor(rgb);
	ck_assert_int_eq(rgb[0], 255);
	ck_assert_int_eq(rgb[1], 245);
	ck_assert_int_eq(rgb[2], 0);

	rgb[0] = 50;
	rgb[1] = 150;
	rgb[2] = 200;
	invertcolor(rgb);
	ck_assert_int_eq(rgb[0], 205);
	ck_assert_int_eq(rgb[1], 105);
	ck_assert_int_eq(rgb[2], 55);
}
END_TEST

/* return path of a readable system sans TTF, or NULL if none found,
 * order prefers fonts commonly shipped by major Linux distros */
static const char *find_test_ttf(void)
{
	static const char *candidates[] = {
		/* Noto Sans: often the default on Fedora / Debian / Ubuntu desktops */
		"/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
		"/usr/share/fonts/google-noto/NotoSans-Regular.ttf",
		"/usr/share/fonts/noto/NotoSans-Regular.ttf",
		/* Liberation Sans: common desktop / LibreOffice seed */
		"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
		"/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
		"/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
		/* DejaVu Sans: still common on servers and older defaults */
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/TTF/DejaVuSans.ttf",
		"/usr/local/share/fonts/dejavu/DejaVuSans.ttf",
		/* Droid: often present via fonts-droid-fallback */
		"/usr/share/fonts/truetype/droid/DroidSans.ttf",
		"/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
		"/usr/share/fonts-droid-fallback/truetype/DroidSansFallback.ttf",
		/* FreeSans: another alternative */
		"/usr/share/fonts/truetype/freefont/FreeSans.ttf",
		NULL};
	int i;

	for (i = 0; candidates[i] != NULL; i++) {
		if (access(candidates[i], R_OK) == 0) {
			return candidates[i];
		}
	}
	return NULL;
}

START_TEST(imagefontinit_builtin_small)
{
	IMAGECONTENT ic;
	gdFontPtr small, tiny, large, giant;

	cfg.fontfile[0] = '\0';
	ck_assert_int_eq(imagefontinit(&ic, 0), 1);

	small = gdFontGetSmall();
	tiny = gdFontGetTiny();
	large = gdFontGetLarge();
	giant = gdFontGetGiant();

	ck_assert_int_eq(ic.fontctx.mode, FONT_BUILTIN);
	ck_assert_ptr_eq(ic.fontctx.body, small);
	ck_assert_ptr_eq(ic.fontctx.axis, tiny);
	ck_assert_ptr_eq(ic.fontctx.title, large);
	ck_assert_ptr_eq(ic.fontctx.header, giant);
	ck_assert_ptr_eq(ic.fontctx.footer, tiny);
	ck_assert_int_eq(ic.fontctx.cw, small->w);
	ck_assert_int_eq(ic.fontctx.ch, small->h);
	ck_assert_int_eq(ic.fontctx.header_h, 24);
	ck_assert_int_eq(ic.fontctx.axis_num5_w, 5 * tiny->w);
	ck_assert_int_eq(ic.lineheight, 12);
	ck_assert(ic.fontctx.scale > 0.0);

	ck_assert_int_eq(imagetextwidth(&ic, FONT_ROLE_BODY, NULL), 0);
	ck_assert_int_eq(imagetextwidth(&ic, FONT_ROLE_BODY, ""), 0);
	ck_assert_int_eq(imagetextwidth(&ic, FONT_ROLE_BODY, "ab"), 2 * imagefontwidth(&ic, FONT_ROLE_BODY));
	ck_assert_int_eq(imagefontwidth(&ic, FONT_ROLE_FOOTER), tiny->w);
	ck_assert_int_eq(imagefontheight(&ic, FONT_ROLE_FOOTER), tiny->h);
	ck_assert_int_eq(imagefontwidth(&ic, FONT_ROLE_BODY), small->w);
	ck_assert_int_eq(imagefontheight(&ic, FONT_ROLE_BODY), small->h);

	imagefontcleanup();
}
END_TEST

START_TEST(imagefontinit_builtin_large)
{
	IMAGECONTENT ic_small, ic_large;
	gdFontPtr largefont, smallfont;

	cfg.fontfile[0] = '\0';
	ck_assert_int_eq(imagefontinit(&ic_small, 0), 1);
	ck_assert_int_eq(imagefontinit(&ic_large, 1), 1);

	largefont = gdFontGetLarge();
	smallfont = gdFontGetSmall();

	ck_assert_int_eq(ic_large.fontctx.mode, FONT_BUILTIN);
	ck_assert_ptr_eq(ic_large.fontctx.body, largefont);
	ck_assert_ptr_eq(ic_large.fontctx.axis, smallfont);
	ck_assert_ptr_eq(ic_large.fontctx.title, gdFontGetGiant());
	ck_assert_int_eq(ic_large.fontctx.cw, largefont->w);
	ck_assert_int_eq(ic_large.fontctx.ch, largefont->h);
	ck_assert_int_eq(ic_large.lineheight, 16);
	ck_assert(ic_large.fontctx.scale > ic_small.fontctx.scale);

	imagefontcleanup();
}
END_TEST

START_TEST(imageextrapx_and_imageuipx_builtin)
{
	IMAGECONTENT ic;

	cfg.fontfile[0] = '\0';
	ck_assert_int_eq(imagefontinit(&ic, 0), 1);

	ic.large = 0;
	ck_assert_int_eq(imageextrapx(&ic, 0), 0);
	ck_assert_int_eq(imageextrapx(&ic, 14), 0);
	ck_assert_int_eq(imageuipx(&ic, 0), 0);
	ck_assert_int_eq(imageuipx(&ic, -1), 0);
	ck_assert_int_eq(imageuipx(&ic, 1), 1);
	ck_assert_int_eq(imageuipx(&ic, 2), 2);

	ic.large = 1;
	ck_assert_int_eq(imageextrapx(&ic, 14), 14);
	ck_assert_int_eq(imageextrapx(&ic, 0), 0);
	/* built-in UI thickness is not fattened by LargeFonts */
	ck_assert_int_eq(imageuipx(&ic, 1), 1);

	imagefontcleanup();
}
END_TEST

START_TEST(graph_geometry_helpers_builtin)
{
	IMAGECONTENT ic;
	int small_hourly, large_hourly;

	cfg.fontfile[0] = '\0';
	ck_assert_int_eq(imagefontinit(&ic, 0), 1);

	ic.large = 0;
	ck_assert_int_eq(graph_axis_left(&ic), GRAPH_AXIS_BASE);
	ck_assert_int_eq(graph_xpos_margin(&ic), 8);
	ck_assert_int_eq(graph_extra_space(&ic), FIVEMINEXTRASPACE);
	small_hourly = hourly_graph_width(&ic);
	ck_assert_int_eq(small_hourly, HOURLY_CANVAS_BASE);

	ic.large = 1;
	ck_assert_int_eq(graph_xpos_margin(&ic), 8 + imageextrapx(&ic, 14));
	ck_assert_int_eq(graph_extra_space(&ic), FIVEMINEXTRASPACE + imageextrapx(&ic, 14));
	large_hourly = hourly_graph_width(&ic);
	ck_assert_int_gt(large_hourly, small_hourly);

	imagefontcleanup();
}
END_TEST

START_TEST(image_common_target_width_builtin)
{
	IMAGECONTENT ic;
	int list_w, summary_w, hourly_w, common;

	cfg.fontfile[0] = '\0';
	ck_assert_int_eq(imagefontinit(&ic, 0), 1);
	ic.large = 0;

	list_w = image_list_width(&ic);
	summary_w = image_summary_width(&ic, 0);
	hourly_w = hourly_graph_width(&ic);
	common = image_common_target_width(&ic);

	ck_assert_int_eq(list_w, 83 * ic.fontctx.cw + imageuipx(&ic, 2) + imageextrapx(&ic, 2));
	ck_assert_int_eq(list_w, HOURLY_CANVAS_BASE);
	ck_assert_int_eq(summary_w, list_w);
	ck_assert_int_eq(hourly_w, HOURLY_CANVAS_BASE);
	ck_assert_int_eq(common, list_w);
	ck_assert_int_ge(common, list_w);
	ck_assert_int_ge(common, summary_w);
	ck_assert_int_ge(common, hourly_w);

	ic.commonwidth = common;
	ck_assert_int_eq(image_list_bar_extra(&ic, list_w, 100), 0);
	ic.commonwidth = list_w + 1000;
	ck_assert_int_eq(image_list_bar_extra(&ic, list_w, 100), 50);
	ic.commonwidth = list_w - 80;
	ck_assert_int_eq(image_list_bar_extra(&ic, list_w, 100), -50);
	ic.commonwidth = 0;
	ck_assert_int_eq(image_list_bar_extra(&ic, list_w, 100), 0);

	ck_assert_int_eq(imagefontinit(&ic, 1), 1);
	ic.large = 1;
	list_w = image_list_width(&ic);
	summary_w = image_summary_width(&ic, 0);
	hourly_w = hourly_graph_width(&ic);
	common = image_common_target_width(&ic);

	ck_assert_int_eq(summary_w, list_w);
	ck_assert_int_ge(common, list_w);
	ck_assert_int_ge(common, summary_w);
	ck_assert_int_ge(common, hourly_w);
	/* built-in large list and hourly stay within a few pixels of each other */
	ck_assert_int_le(abs(list_w - hourly_w), 8);

	imagefontcleanup();
}
END_TEST

#if HAVE_DECL_GDIMAGESTRINGFT
START_TEST(image_common_target_width_ttf)
{
	IMAGECONTENT ic;
	int list_w, summary_w, hourly_w, common;
	const char *font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

	if (access(font, R_OK) != 0) {
		return;
	}

	strncpy_nt(cfg.fontfile, font, 512);
	cfg.fontsize = 12;
	ck_assert_int_eq(imagefontinit(&ic, 0), 1);

	list_w = image_list_width(&ic);
	summary_w = image_summary_width(&ic, 0);
	hourly_w = hourly_graph_width(&ic);
	common = image_common_target_width(&ic);

	ck_assert_int_gt(list_w, 0);
	ck_assert_int_gt(summary_w, 0);
	ck_assert_int_gt(hourly_w, 0);
	ck_assert_int_ge(common, list_w);
	ck_assert_int_ge(common, summary_w);
	ck_assert_int_ge(common, hourly_w);
	ck_assert_int_eq(common, list_w >= summary_w
		? (list_w >= hourly_w ? list_w : hourly_w)
		: (summary_w >= hourly_w ? summary_w : hourly_w));

	imagefontcleanup();
}
END_TEST
#endif

START_TEST(imagefontinit_fails_for_missing_fontfile)
{
	IMAGECONTENT ic;

	strncpy_nt(cfg.fontfile, "/nonexistent/path/vnstat-missing-font.ttf", 512);
	cfg.fontsize = FONTSIZE;
	fclose(stderr);

	ck_assert_int_eq(imagefontinit(&ic, 0), 0);
	imagefontcleanup();
}
END_TEST

#if HAVE_DECL_GDIMAGESTRINGFT
START_TEST(imagefontinit_ttf_success_and_metrics)
{
	IMAGECONTENT ic, ic_large, ic_big;
	const char *font;
	int body_w, title_w;

	font = find_test_ttf();
	if (font == NULL) {
		return;
	}

	strncpy_nt(cfg.fontfile, font, 512);
	cfg.fontsize = FONTSIZE;

	ck_assert_int_eq(imagefontinit(&ic, 0), 1);
	ck_assert_int_eq(ic.fontctx.mode, FONT_TTF);
	ck_assert_str_eq(ic.fontctx.ttfpath, font);
	ck_assert(ic.fontctx.ptsize == (double)FONTSIZE);
	ck_assert_int_gt(ic.fontctx.cw, 0);
	ck_assert_int_gt(ic.fontctx.ch, 0);
	ck_assert_int_gt(ic.fontctx.header_h, 0);
	ck_assert_int_gt(ic.lineheight, 0);
	ck_assert(ic.fontctx.scale > 0.0);
	ck_assert_int_gt(ic.fontctx.axis_num5_w, 0);

	ck_assert_int_eq(imageuipx(&ic, 1), 1);
	ck_assert_int_eq(imageuipx(&ic, 2), 2);
	ck_assert_int_eq(imageuipx(&ic, GRAPH_AXIS_CROSS), GRAPH_AXIS_CROSS);
	ck_assert_int_eq(graph_axis_left(&ic), ic.fontctx.axis_num5_w + imageuipx(&ic, GRAPH_AXIS_LABEL_GAP));
	/* at 12pt, side pad redistributes design left vs after_tip; total chrome unchanged */
	ck_assert_int_eq(graph_xpos_margin(&ic),
					 (8 + imageuipx(&ic, GRAPH_AXIS_CROSS) + 1 + imageuipx(&ic, GRAPH_EXTRA_RIGHT)
					  - imageuipx(&ic, FIVEMINWIDTHFULLPADDING))
						 / 2);
	ck_assert_int_eq(graph_extra_space(&ic),
					 graph_xpos_margin(&ic) + graph_axis_left(&ic) + imageuipx(&ic, GRAPH_AXIS_CROSS) + 1
						 + (graph_xpos_margin(&ic) - imageuipx(&ic, GRAPH_AXIS_CROSS) - 1 + imageuipx(&ic, FIVEMINWIDTHFULLPADDING)));
	ck_assert_int_ge(hourly_graph_left(&ic), 12);
	ck_assert_int_eq(hourly_graph_width(&ic),
					 hourly_graph_left(&ic) + graph_axis_left(&ic) + HOURLY_PLOT_SPAN + hourly_plot_extrax(&ic)
						 + imageextrapx(&ic, 2) + (imageuipx(&ic, 13) - 13)
						 + hourly_graph_left(&ic) + imageuipx(&ic, HOURLY_AXIS_PAST) + imageextrapx(&ic, 2));
	ck_assert_int_eq(hourly_plot_extrax(&ic), HOURLY_HOUR_GAPS * imageextrapx(&ic, 6));

	ck_assert_int_eq(imagetextwidth(&ic, FONT_ROLE_BODY, NULL), 0);
	ck_assert_int_eq(imagetextwidth(&ic, FONT_ROLE_BODY, ""), 0);
	ck_assert_int_gt(imagetextwidth(&ic, FONT_ROLE_BODY, "longer"), imagetextwidth(&ic, FONT_ROLE_BODY, "x"));

	body_w = imagefontwidth(&ic, FONT_ROLE_BODY);
	title_w = imagefontwidth(&ic, FONT_ROLE_TITLE);
	ck_assert_int_ge(title_w, body_w);
	ck_assert_int_eq(imagefontwidth(&ic, FONT_ROLE_FOOTER), gdFontGetTiny()->w);
	ck_assert_int_eq(imagefontheight(&ic, FONT_ROLE_FOOTER), gdFontGetTiny()->h);

	ck_assert_int_eq(imagefontinit(&ic_large, 1), 1);
	ck_assert(ic_large.fontctx.ptsize == (double)FONTSIZE * 1.5);
	/* LargeFonts at 12pt -> 18pt: first expected thickness/cross step */
	ck_assert_int_eq(imageuipx(&ic_large, GRAPH_AXIS_CROSS), 6);
	ck_assert_int_eq(imageuipx(&ic_large, 1), 2);
	ck_assert_int_gt(graph_extra_space(&ic_large), graph_extra_space(&ic));

	cfg.fontsize = 18;
	{
		IMAGECONTENT ic_18;
		int extra_18, side_18, right_18;

		ck_assert_int_eq(imagefontinit(&ic_18, 0), 1);
		ck_assert_int_eq(imageuipx(&ic_18, GRAPH_AXIS_CROSS), 6);
		ck_assert_int_eq(imageuipx(&ic_18, 1), 2);
		side_18 = graph_xpos_margin(&ic_18);
		right_18 = side_18 - 6 - 1 + imageuipx(&ic_18, FIVEMINWIDTHFULLPADDING);
		extra_18 = graph_extra_space(&ic_18);
		ck_assert_int_eq(extra_18, side_18 + graph_axis_left(&ic_18) + 6 + 1 + right_18);
		ck_assert_int_gt(extra_18, graph_extra_space(&ic));
		imagefontcleanup();
	}

	cfg.fontsize = 24;
	ck_assert_int_eq(imagefontinit(&ic_big, 0), 1);
	ck_assert_int_gt(ic_big.fontctx.cw, ic.fontctx.cw);
	ck_assert_int_gt(ic_big.fontctx.ch, ic.fontctx.ch);
	ck_assert(ic_big.fontctx.scale > ic.fontctx.scale);
	ck_assert_int_eq(imageuipx(&ic_big, 1), (int)lrint(1.0 * 24.0 / (double)FONTSIZE));
	ck_assert_int_eq(imageuipx(&ic_big, GRAPH_AXIS_CROSS), 8);
	ck_assert_int_gt(graph_extra_space(&ic_big), graph_extra_space(&ic));

	cfg.fontsize = 72;
	{
		IMAGECONTENT ic_72;

		ck_assert_int_eq(imagefontinit(&ic_72, 0), 1);
		ck_assert_int_eq(imageuipx(&ic_72, GRAPH_AXIS_CROSS), 24);
		ck_assert_int_eq(imageuipx(&ic_72, 1), 6);
		ck_assert_int_eq(imageuipx(&ic_72, 2), 12);
		ck_assert_int_eq(imageuipx(&ic_72, 3), 18);
		ck_assert_int_gt(graph_extra_space(&ic_72), graph_extra_space(&ic_big));
		imagefontcleanup();
	}

	imagefontcleanup();
	imagefontcleanup();
}
END_TEST
#else
START_TEST(imagefontinit_fails_without_freetype)
{
	IMAGECONTENT ic;

	strncpy_nt(cfg.fontfile, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 512);
	cfg.fontsize = FONTSIZE;

	ck_assert_int_eq(imagefontinit(&ic, 0), 0);
	imagefontcleanup();
}
END_TEST
#endif

START_TEST(imagestring_and_draw_helpers_smoke)
{
	IMAGECONTENT ic;

	cfg.fontfile[0] = '\0';
	initimagecontent(&ic);
	imageinit(&ic, 80, 60);

	imagestring(&ic, FONT_ROLE_BODY, 2, 2, NULL, ic.ctext);
	imagestring(&ic, FONT_ROLE_BODY, 2, 2, "", ic.ctext);
	imagestring(&ic, FONT_ROLE_BODY, 2, 2, "ok", ic.ctext);
	imagestringup(&ic, FONT_ROLE_AXIS, 2, 40, NULL, ic.ctext);
	imagestringup(&ic, FONT_ROLE_AXIS, 2, 40, "", ic.ctext);
	imagestringup(&ic, FONT_ROLE_AXIS, 2, 40, "up", ic.ctext);

	graph_draw_axis_value(&ic, 40, 20, NULL, 10, 18);
	graph_draw_axis_value(&ic, 40, 20, "", 10, 18);
	graph_draw_axis_value(&ic, 40, 20, "12", 10, 18);
	graph_draw_axis_unit(&ic, 4, 4, 50, "KiB");

	imagedrawhline(&ic, 5, 50, 10, ic.cline);
	imagedrawvline(&ic, 20, 5, 40, ic.cline);
	imagedrawrect(&ic, 5, 5, 40, 30, ic.cedge);
	imagedrawdashedhline(&ic, 5, 50, 35, ic.clinel);

	ck_assert_int_eq(gdImageGetPixel(ic.im, 10, 10), ic.cline);

	drawpercentilelegend(&ic, 10, 45, 0, 1000);

	gdImageDestroy(ic.im);
	imagefontcleanup();
}
END_TEST

START_TEST(rtrimspaces_trims_trailing_spaces)
{
	char buf[32];

	rtrimspaces(NULL);

	buf[0] = '\0';
	rtrimspaces(buf);
	ck_assert_str_eq(buf, "");

	strncpy_nt(buf, "   --     ", 32);
	rtrimspaces(buf);
	ck_assert_str_eq(buf, "   --");

	strncpy_nt(buf, "    --    ", 32);
	rtrimspaces(buf);
	ck_assert_str_eq(buf, "    --");

	strncpy_nt(buf, "  1.00 GiB", 32);
	rtrimspaces(buf);
	ck_assert_str_eq(buf, "  1.00 GiB");

	strncpy_nt(buf, "     ", 32);
	rtrimspaces(buf);
	ck_assert_str_eq(buf, "");

	strncpy_nt(buf, "notrail", 32);
	rtrimspaces(buf);
	ck_assert_str_eq(buf, "notrail");
}
END_TEST

START_TEST(image_outputs_do_not_crash)
{
	IMAGECONTENT ic;
	int ret, i, o, n;
	static const int list_qmodes[] = {1, 2, 3, 4, 8, 9};
	static const int graph_qmodes[] = {7, 10};
	static const int summary_layout_qmodes[] = {51, 52};
	static const int commonwidth_qmodes[] = {1, 5, 7};

	cfg.fontfile[0] = '\0';
	initimagecontent(&ic);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	for (i = 1; i < 100; i++) {
		ret = db_addtraffic_dated("something", (uint64_t)i * 1234, (uint64_t)i * 2345, (uint64_t)i * 85000);
		ck_assert_int_eq(ret, 1);
	}
	ret = db_setupdated("something", (time_t)i * 85000);
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfaceinfo("something", &ic.interface);
	ck_assert_int_eq(ret, 1);

	/* avoid percentile graph coverage warnings on stderr */
	cfg.fiveminutehours = PERCENTILEENTRYCOUNT;

	suppress_output();

	n = (int)(sizeof(image_output_qmodes) / sizeof(image_output_qmodes[0]));
	for (i = 0; i < n; i++) {
		cfg.qmode = image_output_qmodes[i];
		cfg.ostyle = 0;
		cfg.hourlyrate = 0;
		cfg.summarygraph = 0;
		cfg.transbg = 0;
		cfg.commonwidth = 0;
		assert_drawimage_produces_image(&ic);
	}

	n = (int)(sizeof(list_qmodes) / sizeof(list_qmodes[0]));
	for (i = 0; i < n; i++) {
		for (o = 0; o <= 3; o++) {
			cfg.qmode = list_qmodes[i];
			cfg.ostyle = o;
			assert_drawimage_produces_image(&ic);
		}
	}
	cfg.ostyle = 0;

	n = (int)(sizeof(graph_qmodes) / sizeof(graph_qmodes[0]));
	for (i = 0; i < n; i++) {
		for (o = 0; o <= 1; o++) {
			cfg.qmode = graph_qmodes[i];
			cfg.hourlyrate = o;
			assert_drawimage_produces_image(&ic);
		}
	}
	cfg.hourlyrate = 0;

	n = (int)(sizeof(summary_layout_qmodes) / sizeof(summary_layout_qmodes[0]));
	for (i = 0; i < n; i++) {
		for (o = 0; o <= 1; o++) {
			cfg.qmode = summary_layout_qmodes[i];
			cfg.summarygraph = o;
			assert_drawimage_produces_image(&ic);
		}
	}
	cfg.summarygraph = 0;

	cfg.qmode = 5;
	cfg.transbg = 1;
	assert_drawimage_produces_image(&ic);
	cfg.transbg = 0;

	cfg.commonwidth = 1;
	n = (int)(sizeof(commonwidth_qmodes) / sizeof(commonwidth_qmodes[0]));
	for (i = 0; i < n; i++) {
		cfg.qmode = commonwidth_qmodes[i];
		assert_drawimage_produces_image(&ic);
	}
	cfg.commonwidth = 0;

	ret = db_close();
	ck_assert_int_eq(ret, 1);
	imagefontcleanup();
}
END_TEST

START_TEST(image_outputs_do_not_crash_without_data)
{
	IMAGECONTENT ic;
	int ret, i, n;

	cfg.fontfile[0] = '\0';
	initimagecontent(&ic);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("something");
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfaceinfo("something", &ic.interface);
	ck_assert_int_eq(ret, 1);

	cfg.fiveminutehours = PERCENTILEENTRYCOUNT;

	suppress_output();

	n = (int)(sizeof(image_output_qmodes) / sizeof(image_output_qmodes[0]));
	for (i = 0; i < n; i++) {
		cfg.qmode = image_output_qmodes[i];
		assert_drawimage_produces_image(&ic);
	}

	ret = db_close();
	ck_assert_int_eq(ret, 1);
	imagefontcleanup();
}
END_TEST

START_TEST(layoutinit_respects_chrome_flags)
{
	IMAGECONTENT ic;
	int invert;

	cfg.fontfile[0] = '\0';
	initimagecontent(&ic);
	ic.interface.updated = (time_t)get_timestamp(2012, 3, 4, 5, 6);

	for (invert = 0; invert <= 2; invert++) {
		ic.invert = invert;
		ic.showheader = (invert == 0) ? 1 : 0;
		ic.showedge = (invert == 1) ? 0 : 1;
		ic.showlegend = (invert == 2) ? 0 : 1;
		ic.altdate = (invert > 0) ? 1 : 0;

		imageinit(&ic, 320, 200);
		ck_assert_ptr_ne(ic.im, NULL);
		layoutinit(&ic, "chrome flags", 320, 200);
		drawlegend(&ic, 40, 40, 0);
		drawlegend(&ic, 40, 60, 1);
		gdImageDestroy(ic.im);
		ic.im = NULL;
	}

	imagefontcleanup();
}
END_TEST

START_TEST(drawbar_estimate_styles)
{
	IMAGECONTENT ic;
	int style;

	cfg.fontfile[0] = '\0';
	initimagecontent(&ic);
	imageinit(&ic, 120, 80);

	for (style = 0; style <= 2; style++) {
		cfg.estimatestyle = style;
		drawbar(&ic, 10, 20, 80, 40, 40, 100, 1);
	}

	cfg.estimatestyle = 1;
	drawbar(&ic, 10, 40, 80, 10, 10, 0, 0);
	drawbar(&ic, 10, 50, 80, 80, 80, 100, 0);

	gdImageDestroy(ic.im);
	imagefontcleanup();
}
END_TEST

#if HAVE_DECL_GD_NEAREST_NEIGHBOUR
START_TEST(scaleimage_behavior)
{
	IMAGECONTENT ic;
	gdImagePtr original;
	int sx, sy;

	cfg.fontfile[0] = '\0';
	initimagecontent(&ic);
	imageinit(&ic, 200, 200);
	original = ic.im;

	cfg.imagescale = 100;
	scaleimage(&ic);
	ck_assert_ptr_eq(ic.im, original);
	ck_assert_int_eq(gdImageSX(ic.im), 200);
	ck_assert_int_eq(gdImageSY(ic.im), 200);

	cfg.imagescale = 200;
	scaleimage(&ic);
	ck_assert_int_eq(gdImageSX(ic.im), 400);
	ck_assert_int_eq(gdImageSY(ic.im), 400);

	sx = gdImageSX(ic.im);
	sy = gdImageSY(ic.im);
	cfg.imagescale = 10;
	scaleimage(&ic);
	ck_assert_int_eq(gdImageSX(ic.im), sx);
	ck_assert_int_eq(gdImageSY(ic.im), sy);

	cfg.imagescale = 500;
	original = ic.im;
	sx = gdImageSX(ic.im);
	sy = gdImageSY(ic.im);
	scaleimage(&ic);
	/* 400 * 5 = 2000, within limits */
	ck_assert_int_eq(gdImageSX(ic.im), 2000);
	ck_assert_int_eq(gdImageSY(ic.im), 2000);

	cfg.imagescale = 300;
	sx = gdImageSX(ic.im);
	sy = gdImageSY(ic.im);
	scaleimage(&ic);
	/* 2000 * 3 = 6000 > 5000, early return */
	ck_assert_int_eq(gdImageSX(ic.im), sx);
	ck_assert_int_eq(gdImageSY(ic.im), sy);

	gdImageDestroy(ic.im);
	imagefontcleanup();
}
END_TEST
#endif

START_TEST(image_summary_width_layouts_builtin)
{
	IMAGECONTENT ic;
	int list_w, layout0, layout1, layout2;

	cfg.fontfile[0] = '\0';
	ck_assert_int_eq(imagefontinit(&ic, 0), 1);
	ic.large = 0;

	list_w = image_list_width(&ic);
	layout0 = image_summary_width(&ic, 0);
	layout1 = image_summary_width(&ic, 1);
	layout2 = image_summary_width(&ic, 2);

	ck_assert_int_eq(layout0, list_w);
	ck_assert_int_eq(layout2, list_w);
	ck_assert_int_eq(layout1, 163 * ic.fontctx.cw + imageuipx(&ic, 2) + imageextrapx(&ic, 2));
	ck_assert_int_gt(layout1, layout0);

	cfg.summarygraph = 1;
	ck_assert_int_gt(image_summary_width(&ic, 1), 0);
	ck_assert_int_gt(image_summary_width(&ic, 2), 0);
	cfg.summarygraph = 0;

	ck_assert_int_eq(imagefontinit(&ic, 1), 1);
	ic.large = 1;
	list_w = image_list_width(&ic);
	layout0 = image_summary_width(&ic, 0);
	layout1 = image_summary_width(&ic, 1);
	layout2 = image_summary_width(&ic, 2);
	ck_assert_int_eq(layout0, list_w);
	ck_assert_int_eq(layout2, list_w);
	ck_assert_int_eq(layout1, 163 * ic.fontctx.cw + imageuipx(&ic, 2) + imageextrapx(&ic, 2));
	ck_assert_int_gt(layout1, layout0);

	imagefontcleanup();
}
END_TEST

START_TEST(fiveg_barwidth_builtin)
{
	IMAGECONTENT ic;

	cfg.fontfile[0] = '\0';
	ck_assert_int_eq(imagefontinit(&ic, 0), 1);
	ck_assert_int_eq(fiveg_barwidth(&ic), 1);
	imagefontcleanup();
}
END_TEST

START_TEST(percentilelegend_helpers)
{
	IMAGECONTENT ic;
	int mode;

	cfg.fontfile[0] = '\0';
	initimagecontent(&ic);
	imageinit(&ic, 200, 80);

	for (mode = 0; mode <= 2; mode++) {
		ck_assert_int_gt(percentilelegendwidth(&ic, mode, 1000), 0);
	}

	drawpercentilelegend(&ic, 20, 20, 1, 2000);
	drawpercentilelegend(&ic, 20, 40, 2, 3000);

	gdImageDestroy(ic.im);
	imagefontcleanup();
}
END_TEST

START_TEST(imagecentery_builtin)
{
	IMAGECONTENT ic;
	int y;

	cfg.fontfile[0] = '\0';
	ck_assert_int_eq(imagefontinit(&ic, 0), 1);

	y = imagecentery(&ic, FONT_ROLE_BODY, "Ay", 10, 40);
	ck_assert_int_ge(y, 10);
	ck_assert_int_le(y, 40);

	imagefontcleanup();
}
END_TEST

void add_image_tests(Suite *s)
{
	TCase *tc_image = tcase_create("Image");
	tcase_add_checked_fixture(tc_image, setup, teardown);
	tcase_add_unchecked_fixture(tc_image, setup, teardown);
	tcase_set_timeout(tc_image, 60);
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
	tcase_add_test(tc_image, hextorgb_can_convert);
	tcase_add_test(tc_image, modcolor_mods_colors);
	tcase_add_test(tc_image, invertcolor_inverts_colors);
	tcase_add_test(tc_image, imagefontinit_builtin_small);
	tcase_add_test(tc_image, imagefontinit_builtin_large);
	tcase_add_test(tc_image, imageextrapx_and_imageuipx_builtin);
	tcase_add_test(tc_image, graph_geometry_helpers_builtin);
	tcase_add_test(tc_image, image_common_target_width_builtin);
	tcase_add_test(tc_image, imagefontinit_fails_for_missing_fontfile);
#if HAVE_DECL_GDIMAGESTRINGFT
	tcase_add_test(tc_image, imagefontinit_ttf_success_and_metrics);
	tcase_add_test(tc_image, image_common_target_width_ttf);
#else
	tcase_add_test(tc_image, imagefontinit_fails_without_freetype);
#endif
	tcase_add_test(tc_image, imagestring_and_draw_helpers_smoke);
	tcase_add_test(tc_image, rtrimspaces_trims_trailing_spaces);
	tcase_add_test(tc_image, image_outputs_do_not_crash);
	tcase_add_test(tc_image, image_outputs_do_not_crash_without_data);
	tcase_add_test(tc_image, layoutinit_respects_chrome_flags);
	tcase_add_test(tc_image, drawbar_estimate_styles);
#if HAVE_DECL_GD_NEAREST_NEIGHBOUR
	tcase_add_test(tc_image, scaleimage_behavior);
#endif
	tcase_add_test(tc_image, image_summary_width_layouts_builtin);
	tcase_add_test(tc_image, fiveg_barwidth_builtin);
	tcase_add_test(tc_image, percentilelegend_helpers);
	tcase_add_test(tc_image, imagecentery_builtin);
	suite_add_tcase(s, tc_image);
}
