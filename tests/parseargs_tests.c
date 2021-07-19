#include "common.h"
#include "vnstat_tests.h"
#include "vnstat_func.h"
#include "cfg.h"
#include "dbsql.h"
#include "parseargs_tests.h"

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
					"--oneline", "b", "--xml", "h", "7", "--json", "d", "7", "-ru", "--rateunit", "0",
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
	ck_assert_int_eq(cfg.listjsonxml, 7);
	ck_assert_str_eq(p.databegin, "2000-01-01");
	ck_assert_str_eq(p.dataend, "2001-01-01");
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

START_TEST(vnstat_parseargs_xml_without_extra_params)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--xml", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	cfg.qmode = 0;
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
	ck_assert_int_eq(cfg.qmode, 8);
	ck_assert_int_eq(p.xmlmode, 'a');
	ck_assert_int_eq(cfg.listjsonxml, 0);
}
END_TEST

START_TEST(vnstat_parseargs_xml_with_mode)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--xml", "m", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	cfg.qmode = 0;
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
	ck_assert_int_eq(cfg.qmode, 8);
	ck_assert_int_eq(p.xmlmode, 'm');
	ck_assert_int_eq(cfg.listjsonxml, 0);
}
END_TEST

START_TEST(vnstat_parseargs_xml_with_limit)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--xml", "1231", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	cfg.qmode = 0;
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
	ck_assert_int_eq(cfg.qmode, 8);
	ck_assert_int_eq(p.xmlmode, 'a');
	ck_assert_int_eq(cfg.listjsonxml, 1231);
}
END_TEST

START_TEST(vnstat_parseargs_xml_with_mode_and_limit)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--xml", "t", "2", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	cfg.qmode = 0;
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
	ck_assert_int_eq(cfg.qmode, 8);
	ck_assert_int_eq(p.xmlmode, 't');
	ck_assert_int_eq(cfg.listjsonxml, 2);
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

START_TEST(vnstat_parseargs_json_without_extra_params)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--json", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	cfg.qmode = 0;
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
	ck_assert_int_eq(cfg.qmode, 10);
	ck_assert_int_eq(p.jsonmode, 'a');
	ck_assert_int_eq(cfg.listjsonxml, 0);
}
END_TEST

START_TEST(vnstat_parseargs_json_with_mode)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--json", "m", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	cfg.qmode = 0;
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
	ck_assert_int_eq(cfg.qmode, 10);
	ck_assert_int_eq(p.jsonmode, 'm');
	ck_assert_int_eq(cfg.listjsonxml, 0);
}
END_TEST

START_TEST(vnstat_parseargs_json_with_limit)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--json", "1232", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	cfg.qmode = 0;
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
	ck_assert_int_eq(cfg.qmode, 10);
	ck_assert_int_eq(p.jsonmode, 'a');
	ck_assert_int_eq(cfg.listjsonxml, 1232);
}
END_TEST

START_TEST(vnstat_parseargs_json_with_mode_and_limit)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--json", "d", "3", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	cfg.qmode = 0;
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
	ck_assert_int_eq(cfg.qmode, 10);
	ck_assert_int_eq(p.jsonmode, 'd');
	ck_assert_int_eq(cfg.listjsonxml, 3);
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

START_TEST(vnstat_parseargs_setalias_still_supports_nick)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--nick", "Underground", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
	ck_assert_int_eq(p.setalias, 1);
	ck_assert_str_eq(p.alias, "Underground");
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

START_TEST(vnstat_parseargs_limit_requires_parameter)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--limit", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_limit_cannot_be_negative)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--limit", "-1", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_limit_changes_defaults)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--limit", "1234", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	defaultcfg();
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);

	ck_assert_int_eq(cfg.listfivemins, 1234);
	ck_assert_int_eq(cfg.listhours, 1234);
	ck_assert_int_eq(cfg.listdays, 1234);
	ck_assert_int_eq(cfg.listmonths, 1234);
	ck_assert_int_eq(cfg.listyears, 1234);
	ck_assert_int_eq(cfg.listtop, 1234);
	ck_assert_int_eq(cfg.listjsonxml, 1234);
}
END_TEST

START_TEST(vnstat_parseargs_limit_overrides)
{
	PARAMS p;
	char *argv[] = {"vnstat", "-d", "5", "--limit", "234", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	defaultcfg();
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);

	ck_assert_int_eq(cfg.listfivemins, 234);
	ck_assert_int_eq(cfg.listhours, 234);
	ck_assert_int_eq(cfg.listdays, 234);
	ck_assert_int_eq(cfg.listmonths, 234);
	ck_assert_int_eq(cfg.listyears, 234);
	ck_assert_int_eq(cfg.listtop, 234);
	ck_assert_int_eq(cfg.listjsonxml, 234);
}
END_TEST

START_TEST(vnstat_parseargs_limit_overrides_regardless_of_position)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--limit", "345", "-d", "5", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	defaultcfg();
	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);

	ck_assert_int_eq(cfg.listfivemins, 345);
	ck_assert_int_eq(cfg.listhours, 345);
	ck_assert_int_eq(cfg.listdays, 345);
	ck_assert_int_eq(cfg.listmonths, 345);
	ck_assert_int_eq(cfg.listyears, 345);
	ck_assert_int_eq(cfg.listtop, 345);
	ck_assert_int_eq(cfg.listjsonxml, 345);
}
END_TEST

START_TEST(vnstat_parseargs_iflist_gives_help)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--iflist", "notknown", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_iflist_without_parameters_and_no_interfaces)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--iflist", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	remove_directory(TESTDIR);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_iflist_without_parameters_and_only_lo)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--iflist", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	fake_proc_net_dev("w", "lo", 0, 0, 0, 0);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_iflist_without_parameters)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--iflist", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	fake_proc_net_dev("w", "ethsomething", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethanything", 0, 0, 0, 0);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_iflist_with_verbose_mode)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--iflist", "0", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	fake_proc_net_dev("w", "ethsomething", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethanything", 0, 0, 0, 0);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_iflist_with_parseable_mode)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--iflist", "1", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	fake_proc_net_dev("w", "ethsomething", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethanything", 0, 0, 0, 0);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_dbiflist_gives_help)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--dbiflist", "unknown", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_dbiflist_with_no_database)
{
	PARAMS p;
	char *argv[] = {"vnstat", "--dbiflist", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	suppress_output();
	parseargs(&p, argc, argv);
}
END_TEST

START_TEST(vnstat_parseargs_dbiflist_without_parameters_and_no_interfaces)
{
	int ret;
	PARAMS p;
	char *argv[] = {"vnstat", "--dbiflist", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	suppress_output();
	parseargs(&p, argc, argv);
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(vnstat_parseargs_dbiflist_without_parameters)
{
	int ret;
	PARAMS p;
	char *argv[] = {"vnstat", "--dbiflist", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("etheverything");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethnothing");
	ck_assert_int_eq(ret, 1);
	suppress_output();
	parseargs(&p, argc, argv);
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(vnstat_parseargs_dbiflist_with_verbose_mode)
{
	int ret;
	PARAMS p;
	char *argv[] = {"vnstat", "--dbiflist", "0", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("etheverything");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethnothing");
	ck_assert_int_eq(ret, 1);
	suppress_output();
	parseargs(&p, argc, argv);
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(vnstat_parseargs_dbiflist_with_parseable_mode)
{
	int ret;
	PARAMS p;
	char *argv[] = {"vnstat", "--dbiflist", "1", NULL};
	int argc = sizeof(argv) / sizeof(char *) - 1;

	initparams(&p);
	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("etheverything");
	ck_assert_int_eq(ret, 1);
	ret = db_addinterface("ethnothing");
	ck_assert_int_eq(ret, 1);
	suppress_output();
	parseargs(&p, argc, argv);
	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(vnstat_ishelprequest_knows_what_a_help_request_is)
{
	ck_assert_int_eq(ishelprequest("--help"), 1);
	ck_assert_int_eq(ishelprequest("-?"), 1);
	ck_assert_int_eq(ishelprequest("?"), 1);
	ck_assert_int_eq(ishelprequest("help"), 0);
	ck_assert_int_eq(ishelprequest("-h"), 0);
	ck_assert_int_eq(ishelprequest("--helpme"), 0);
	ck_assert_int_eq(ishelprequest(""), 0);
	ck_assert_int_eq(ishelprequest("1"), 0);
	ck_assert_int_eq(ishelprequest("a"), 0);
}
END_TEST

void add_parseargs_tests(Suite *s)
{
	TCase *tc_pa = tcase_create("ParseArgs");
	tcase_add_checked_fixture(tc_pa, setup, teardown);
	tcase_add_unchecked_fixture(tc_pa, setup, teardown);
	tcase_add_test(tc_pa, vnstat_parseargs_does_nothing_without_args);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_can_help, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_can_longhelp, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_can_show_version, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_detects_unknown_parameters, 1);
	tcase_add_test(tc_pa, vnstat_parseargs_can_modify_settings);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_does_not_allow_too_long_interface_names, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_style_requires_parameter, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_style_checks_parameter, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_knows_that_update_is_not_supported, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_dbdir_requires_a_directory, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_oneline_gives_help, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_xml_gives_help, 1);
	tcase_add_test(tc_pa, vnstat_parseargs_xml_without_extra_params);
	tcase_add_test(tc_pa, vnstat_parseargs_xml_with_mode);
	tcase_add_test(tc_pa, vnstat_parseargs_xml_with_limit);
	tcase_add_test(tc_pa, vnstat_parseargs_xml_with_mode_and_limit);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_json_gives_help, 1);
	tcase_add_test(tc_pa, vnstat_parseargs_json_without_extra_params);
	tcase_add_test(tc_pa, vnstat_parseargs_json_with_mode);
	tcase_add_test(tc_pa, vnstat_parseargs_json_with_limit);
	tcase_add_test(tc_pa, vnstat_parseargs_json_with_mode_and_limit);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_rateunit_gives_help, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_live_gives_help, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_begin_gives_help, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_end_gives_help, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_begin_validates_input, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_end_validates_input, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_can_show_config, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_iface_requires_parameter, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_locale_requires_parameter, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_setalias_requires_parameter, 1);
	tcase_add_test(tc_pa, vnstat_parseargs_setalias_still_supports_nick);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_rename_requires_parameter, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_limit_requires_parameter, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_limit_cannot_be_negative, 1);
	tcase_add_test(tc_pa, vnstat_parseargs_limit_changes_defaults);
	tcase_add_test(tc_pa, vnstat_parseargs_limit_overrides);
	tcase_add_test(tc_pa, vnstat_parseargs_limit_overrides_regardless_of_position);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_iflist_gives_help, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_iflist_without_parameters_and_no_interfaces, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_iflist_without_parameters_and_only_lo, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_iflist_without_parameters, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_iflist_with_verbose_mode, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_iflist_with_parseable_mode, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_dbiflist_gives_help, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_dbiflist_with_no_database, 1);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_dbiflist_without_parameters_and_no_interfaces, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_dbiflist_without_parameters, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_dbiflist_with_verbose_mode, 0);
	tcase_add_exit_test(tc_pa, vnstat_parseargs_dbiflist_with_parseable_mode, 0);
	tcase_add_test(tc_pa, vnstat_ishelprequest_knows_what_a_help_request_is);
	suite_add_tcase(s, tc_pa);
}
