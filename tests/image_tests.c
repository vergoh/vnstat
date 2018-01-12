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

void add_image_tests(Suite *s)
{
	TCase *tc_image = tcase_create("Image");
	tcase_add_test(tc_image, initimagecontent_does_not_crash);
	tcase_add_test(tc_image, colorinit_does_not_crash);
	tcase_add_test(tc_image, layoutinit_does_not_crash);
	suite_add_tcase(s, tc_image);
}
