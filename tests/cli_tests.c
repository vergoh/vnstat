#include "common.h"
#include "vnstat_tests.h"
#include "vnstat_func.h"
#include "cfg.h"
#include "cli_tests.h"

START_TEST(vnstat_can_init_params)
{
    PARAMS p;

    initparams(&p);
}
END_TEST

START_TEST(vnstat_showhelp_does_not_crash)
{
    PARAMS p;

    initparams(&p);
    strncpy_nt(p.definterface, "ethgone", 32);
    suppress_output();
    showhelp(&p);
}
END_TEST

START_TEST(vnstat_showlonghelp_does_not_crash)
{
    PARAMS p;

    initparams(&p);
    strncpy_nt(p.definterface, "ethgone", 32);
    suppress_output();
    showlonghelp(&p);
}
END_TEST

START_TEST(vnstat_handlers_do_nothing_by_default)
{
    PARAMS p, b;

    initparams(&p);
    memcpy(&b, &p, sizeof(PARAMS));
    handleremoveinterface(&p);
    handlerenameinterface(&p);
    handleaddinterface(&p);
    handlesetalias(&p);
    handletrafficmeters(&p);
    ck_assert_int_eq(memcmp(&p, &b, sizeof(PARAMS)), 0);
}
END_TEST

START_TEST(vnstat_handletrafficmeters_exists_when_interface_is_not_available)
{
    PARAMS p;

    ck_assert_int_eq(remove_directory(TESTDIR), 1);
    defaultcfg();
    initparams(&p);
    strncpy_nt(cfg.iface, "ethfoo", 32);
	strncpy_nt(p.interface, "default", 32);
	strncpy_nt(p.definterface, cfg.iface, 32);
    p.livetraffic = 1;
    suppress_output();

    handletrafficmeters(&p);
}
END_TEST

START_TEST(vnstat_parseargs_does_nothing_without_args)
{
    PARAMS p;
    char *argv[] = {"vnstat", "-h", NULL};
    int argc = sizeof(argv) / sizeof(char*) - 1;

    initparams(&p);
    parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_can_help)
{
    PARAMS p;
    char *argv[] = {"vnstat", "--help", NULL};
    int argc = sizeof(argv) / sizeof(char*) - 1;

    initparams(&p);
    debug = 1;
    suppress_output();
    parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_can_longhelp)
{
    PARAMS p;
    char *argv[] = {"vnstat", "--longhelp", NULL};
    int argc = sizeof(argv) / sizeof(char*) - 1;

    initparams(&p);
    suppress_output();
    parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_can_show_version)
{
    PARAMS p;
    char *argv[] = {"vnstat", "--version", NULL};
    int argc = sizeof(argv) / sizeof(char*) - 1;

    initparams(&p);
    suppress_output();
    parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_detects_unknown_parameters)
{
    PARAMS p;
    char *argv[] = {"vnstat", "--something_fishy", NULL};
    int argc = sizeof(argv) / sizeof(char*) - 1;

    initparams(&p);
    suppress_output();
    parseargs(&p, argc, argv);
}
END_TEST

void add_cli_tests(Suite *s)
{
	TCase *tc_cli = tcase_create("CLI");
	tcase_add_checked_fixture(tc_cli, setup, teardown);
	tcase_add_unchecked_fixture(tc_cli, setup, teardown);
    tcase_add_test(tc_cli, vnstat_can_init_params);
	tcase_add_test(tc_cli, vnstat_showhelp_does_not_crash);
	tcase_add_test(tc_cli, vnstat_showlonghelp_does_not_crash);
    tcase_add_test(tc_cli, vnstat_handlers_do_nothing_by_default);
    tcase_add_exit_test(tc_cli, vnstat_handletrafficmeters_exists_when_interface_is_not_available, 1);
    tcase_add_test(tc_cli, vnstat_parseargs_does_nothing_without_args);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_can_help, 0);
    tcase_add_exit_test(tc_cli, vnstat_parseargs_can_longhelp, 0);
    tcase_add_exit_test(tc_cli, vnstat_parseargs_can_show_version, 0);
    tcase_add_exit_test(tc_cli, vnstat_parseargs_detects_unknown_parameters, 1);
    suite_add_tcase(s, tc_cli);
}
