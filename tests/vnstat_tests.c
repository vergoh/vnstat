#include "vnstat_tests.h"
#include "common_tests.h"
#include "database_tests.h"
#include "config_tests.h"
#include "ifinfo_tests.h"
#include "misc_tests.h"

void suppress_output(void)
{
	fclose(stdout);
}

Suite *test_suite(void)
{
	Suite *s = suite_create("vnStat");

	add_common_tests(s);
	add_database_tests(s);
	add_config_tests(s);
	add_ifinfo_tests(s);
	add_misc_tests(s);

	return s;
}

int main(void)
{
	int number_failed;

	Suite *s = test_suite();
	SRunner *sr = srunner_create(s);
	srunner_set_log(sr, "test.log");
	srunner_set_xml(sr, "test.xml");
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return number_failed;
}
