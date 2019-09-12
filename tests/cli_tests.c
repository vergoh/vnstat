#include "common.h"
#include "vnstat_tests.h"
#include "vnstat_func.h"
#include "cfg.h"
#include "dbsql.h"
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
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_can_help)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--help", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

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
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_can_show_version)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--version", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_detects_unknown_parameters)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--something_fishy", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_can_modify_settings)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--debug", "--traffic", "12", "--add", "--rename", "aname", "--config",
					"does_nothing", "-l", "1", "--remove", "-i", "ethsomething", "--style", "0", "--dbdir",
					"dbsomewhere", "-q", "-d", "1", "-m", "2", "-t", "3", "-s", "-y", "4", "-hg", "-h", "5", "-5", "6",
					"--oneline", "b", "--xml", "h", "--json", "d", "-ru", "--rateunit", "0",
					"--force", "--setalias", "super", "--begin", "2000-01-01",
					"--end", "2001-01-01", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	defaultcfg();
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);

	ck_assert_str_eq(p.interface, "ethsomething");
	ck_assert_int_eq(p.defaultiface, 0);
	ck_assert_int_eq(cfg.ostyle, 4);
	ck_assert_str_eq(cfg.dbdir, "dbsomewhere");
	ck_assert_int_eq(p.query, 1);
	ck_assert_int_eq(p.force, 1);
	ck_assert_int_eq(cfg.qmode, 10);
	ck_assert_int_eq(p.jsonmode, 'd');
	ck_assert_int_eq(p.xmlmode, 'h');
	ck_assert_int_eq(cfg.rateunit, 0);
	ck_assert_int_eq(p.setalias, 1);
	ck_assert_str_eq(p.alias, "super");
	ck_assert_str_eq(p.newifname, "aname");
	ck_assert_int_eq(p.renameiface, 1);
	ck_assert_int_eq(p.removeiface, 1);
	ck_assert_int_eq(p.traffic, 1);
	ck_assert_int_eq(cfg.sampletime, 12);
	ck_assert_int_eq(p.livetraffic, 1);
	ck_assert_int_eq(p.livemode, 1);
	ck_assert_int_eq(cfg.listdays, 1);
	ck_assert_int_eq(cfg.listmonths, 2);
	ck_assert_int_eq(cfg.listtop, 3);
	ck_assert_int_eq(cfg.listyears, 4);
	ck_assert_int_eq(cfg.listhours, 5);
	ck_assert_int_eq(cfg.listfivemins, 6);
}
END_TEST

START_TEST(vnstat_parseargs_does_not_allow_too_long_interface_names)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--iface", "12345678901234567890123456789012", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_style_requires_parameter)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--style", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_style_checks_parameter)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--style", "9001", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_knows_that_update_is_not_supported)
{
	PARAMS p;
	char *argv[] = {"vnstat", "-u", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_dbdir_requires_a_directory)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--dbdir", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_oneline_gives_help)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--oneline", "a", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_xml_gives_help)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--xml", "b", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_json_gives_help)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--json", "b", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_rateunit_gives_help)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--rateunit", "2", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_live_gives_help)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--live", "2", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_begin_gives_help)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--begin", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_end_gives_help)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--end", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_begin_validates_input)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--begin", "world_domination", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_end_validates_input)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--end", "what?", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_can_show_config)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--showconfig", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_iface_requires_parameter)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--iface", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_locale_requires_parameter)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--locale", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_setalias_requires_parameter)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--setalias", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_rename_requires_parameter)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--rename", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_handleremoveinterface_exits_if_no_interface_has_been_specified)
{
	PARAMS p;
	initparams(&p);
	p.removeiface = 1;

	suppress_output();
	handleremoveinterface(&p);
}
END_TEST

START_TEST(vnstat_handleremoveinterface_exits_if_given_interface_does_not_exist)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	p.removeiface = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "unknown", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("known");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handleremoveinterface(&p);
}
END_TEST

START_TEST(vnstat_handleremoveinterface_exits_if_force_is_not_used)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	p.removeiface = 1;
	p.defaultiface = 0;
	p.force = 0;
	strncpy_nt(p.interface, "known", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("known");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handleremoveinterface(&p);
}
END_TEST

START_TEST(vnstat_handleremoveinterface_exits_after_interface_removal)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	p.removeiface = 1;
	p.defaultiface = 0;
	p.force = 1;
	strncpy_nt(p.interface, "known", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("known");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handleremoveinterface(&p);
	/* TODO: add way to check that removal did happen */
}
END_TEST

START_TEST(vnstat_handlerenameinterface_exits_if_no_interface_has_been_specified)
{
	PARAMS p;
	initparams(&p);
	p.renameiface = 1;

	suppress_output();
	handlerenameinterface(&p);
}
END_TEST

START_TEST(vnstat_handlerenameinterface_exits_if_new_interface_name_is_not_given)
{
	PARAMS p;
	initparams(&p);
	p.renameiface = 1;
	p.defaultiface = 0;

	suppress_output();
	handlerenameinterface(&p);
}
END_TEST

START_TEST(vnstat_handlerenameinterface_exits_if_given_interface_does_not_exist)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	p.renameiface = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "oldname", 32);
	strncpy_nt(p.newifname, "newname", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("somename");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handlerenameinterface(&p);
}
END_TEST

START_TEST(vnstat_handlerenameinterface_exits_if_new_interface_name_already_exist)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	p.renameiface = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "oldname", 32);
	strncpy_nt(p.newifname, "newname", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("oldname");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("newname");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handlerenameinterface(&p);
}
END_TEST

START_TEST(vnstat_handlerenameinterface_exits_if_force_is_not_used)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);

	p.renameiface = 1;
	p.defaultiface = 0;
	p.force = 0;
	strncpy_nt(p.interface, "oldname", 32);
	strncpy_nt(p.newifname, "newname", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("oldname");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handlerenameinterface(&p);
}
END_TEST

START_TEST(vnstat_handlerenameinterface_exits_after_interface_removal)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	p.renameiface = 1;
	p.defaultiface = 0;
	p.force = 1;
	strncpy_nt(p.interface, "oldname", 32);
	strncpy_nt(p.newifname, "newname", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("oldname");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handlerenameinterface(&p);
	/* TODO: add way to check that rename did happen */
}
END_TEST

START_TEST(vnstat_handleaddinterface_exits_if_no_interface_has_been_specified)
{
	PARAMS p;
	initparams(&p);
	p.addiface = 1;

	suppress_output();
	handleaddinterface(&p);
}
END_TEST

START_TEST(vnstat_handleaddinterface_exits_if_interface_already_exist_in_database)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	p.addiface = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "newiface", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("newiface");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handleaddinterface(&p);
}
END_TEST

START_TEST(vnstat_handleaddinterface_exits_if_interface_does_not_exist)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	p.addiface = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "newiface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "notnewiface", 0, 0, 0, 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handleaddinterface(&p);
}
END_TEST

START_TEST(vnstat_handleaddinterface_exits_after_interface_is_added)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	p.addiface = 1;
	p.defaultiface = 0;
	cfg.spacecheck = 0;
	strncpy_nt(p.interface, "newiface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "newiface", 0, 0, 0, 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handleaddinterface(&p);
}
END_TEST

START_TEST(vnstat_handleaddinterface_can_be_forced_to_add_interface_that_does_not_exist)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	p.addiface = 1;
	p.defaultiface = 0;
	p.force = 1;
	cfg.spacecheck = 0;
	strncpy_nt(p.interface, "newiface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "notnewiface", 0, 0, 0, 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handleaddinterface(&p);
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
	tcase_add_test(tc_cli, vnstat_parseargs_can_modify_settings);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_does_not_allow_too_long_interface_names, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_style_requires_parameter, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_style_checks_parameter, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_knows_that_update_is_not_supported, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_dbdir_requires_a_directory, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_oneline_gives_help, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_xml_gives_help, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_json_gives_help, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_rateunit_gives_help, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_live_gives_help, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_begin_gives_help, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_end_gives_help, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_begin_validates_input, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_end_validates_input, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_can_show_config, 0);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_iface_requires_parameter, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_locale_requires_parameter, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_setalias_requires_parameter, 1);
	tcase_add_exit_test(tc_cli, vnstat_parseargs_rename_requires_parameter, 1);
	tcase_add_exit_test(tc_cli, vnstat_handleremoveinterface_exits_if_no_interface_has_been_specified, 1);
	tcase_add_exit_test(tc_cli, vnstat_handleremoveinterface_exits_if_given_interface_does_not_exist, 1);
	tcase_add_exit_test(tc_cli, vnstat_handleremoveinterface_exits_if_force_is_not_used, 1);
	tcase_add_exit_test(tc_cli, vnstat_handleremoveinterface_exits_after_interface_removal, 0);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_if_no_interface_has_been_specified, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_if_new_interface_name_is_not_given, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_if_given_interface_does_not_exist, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_if_new_interface_name_already_exist, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_if_force_is_not_used, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_after_interface_removal, 0);
	tcase_add_exit_test(tc_cli, vnstat_handleaddinterface_exits_if_no_interface_has_been_specified, 1);
	tcase_add_exit_test(tc_cli, vnstat_handleaddinterface_exits_if_interface_already_exist_in_database, 1);
	tcase_add_exit_test(tc_cli, vnstat_handleaddinterface_exits_if_interface_does_not_exist, 1);
	tcase_add_exit_test(tc_cli, vnstat_handleaddinterface_exits_after_interface_is_added, 0);
	tcase_add_exit_test(tc_cli, vnstat_handleaddinterface_can_be_forced_to_add_interface_that_does_not_exist, 0);
	suite_add_tcase(s, tc_cli);
}
