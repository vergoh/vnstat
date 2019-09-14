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
	/* TODO: add way to check that addition did happen */
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
	/* TODO: add way to check that addition did happen */
}
END_TEST

START_TEST(vnstat_handlesetalias_exits_if_no_interface_has_been_specified)
{
	PARAMS p;
	initparams(&p);
	p.setalias = 1;

	suppress_output();
	handlesetalias(&p);
}
END_TEST

START_TEST(vnstat_handlesetalias_exits_if_given_interface_does_not_exist)
{
	int ret;
	PARAMS p;
	defaultcfg();
	initparams(&p);
	p.setalias = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "ethiface", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("somename");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handlesetalias(&p);
}
END_TEST

START_TEST(vnstat_handlesetalias_exits_after_setting_alias)
{
	int ret;
	PARAMS p;
	defaultcfg();
	initparams(&p);
	p.setalias = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "ethiface", 32);
	strncpy_nt(p.alias, "The Internet", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethiface");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handlesetalias(&p);
	/* TODO: add way to check that alias change did happen */
}
END_TEST

START_TEST(vnstat_handlesetalias_exits_after_clearing_alias)
{
	int ret;
	PARAMS p;
	defaultcfg();
	initparams(&p);
	p.setalias = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "ethiface", 32);
	strncpy_nt(p.alias, "", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethiface");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handlesetalias(&p);
	/* TODO: add way to check that alias change did happen */
}
END_TEST

START_TEST(vnstat_handletrafficmeters_exits_when_interface_is_not_available)
{
	PARAMS p;
	defaultcfg();
	initparams(&p);
	p.traffic = 1;
	p.defaultiface = 1;
	strncpy_nt(p.interface, "someiface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "otheriface", 0, 0, 0, 0);

	suppress_output();
	handletrafficmeters(&p);
}
END_TEST

START_TEST(vnstat_handletrafficmeters_exits_when_interface_is_not_available_with_configuration_tips)
{
	PARAMS p;
	defaultcfg();
	initparams(&p);
	p.traffic = 1;
	p.defaultiface = 1;
	strncpy_nt(p.interface, "someiface", 32);
	strncpy_nt(cfg.cfgfile, "I_do_not_have_a_config_file_here.something", 512);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "otheriface", 0, 0, 0, 0);

	suppress_output();
	handletrafficmeters(&p);
}
END_TEST

START_TEST(vnstat_handletrafficmeters_exits_when_specific_interface_is_not_available)
{
	PARAMS p;
	defaultcfg();
	initparams(&p);
	p.traffic = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "someiface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "otheriface", 0, 0, 0, 0);

	suppress_output();
	handletrafficmeters(&p);
}
END_TEST

START_TEST(vnstat_handletrafficmeters_can_calculate_traffic)
{
	PARAMS p;
	defaultcfg();
	initparams(&p);
	p.traffic = 1;
	p.defaultiface = 0;
	cfg.qmode = 1;
	cfg.sampletime = 0;
	strncpy_nt(p.interface, "someiface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "someiface", 0, 0, 0, 0);

	suppress_output();
	handletrafficmeters(&p);
}
END_TEST

START_TEST(vnstat_handletrafficmeters_can_calculate_traffic_and_output_json)
{
	PARAMS p;
	defaultcfg();
	initparams(&p);
	p.traffic = 1;
	p.defaultiface = 0;
	cfg.qmode = 10;
	cfg.sampletime = 0;
	strncpy_nt(p.interface, "someiface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "someiface", 0, 0, 0, 0);

	suppress_output();
	handletrafficmeters(&p);
}
END_TEST

START_TEST(handleifselection_does_nothing_when_interface_has_already_been_selected)
{
	PARAMS p;
	defaultcfg();
	initparams(&p);

	p.defaultiface = 0;
	strncpy_nt(p.interface, "myiface", 32);

	handleifselection(&p);

	ck_assert_str_eq(p.interface, "myiface");
}
END_TEST

START_TEST(handleifselection_selects_default_interface_if_field_is_filled)
{
	PARAMS p;
	defaultcfg();
	initparams(&p);

	p.defaultiface = 1;
	strncpy_nt(p.interface, "default", 32);
	strncpy_nt(p.definterface, "myiface", 32);

	handleifselection(&p);

	ck_assert_str_eq(p.interface, "myiface");
	ck_assert_str_eq(p.definterface, "myiface");
}
END_TEST

START_TEST(handleifselection_exits_when_no_suitable_interface_is_available_for_query)
{
	int ret;
	PARAMS p;
	defaultcfg();
	initparams(&p);

	p.defaultiface = 1;
	p.query = 1;
	strncpy_nt(p.interface, "default", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	/* no interfaces added to database */

	suppress_output();
	handleifselection(&p);
}
END_TEST

START_TEST(handleifselection_selects_only_interface_from_database_for_query)
{
	int ret;
	PARAMS p;
	defaultcfg();
	initparams(&p);

	p.defaultiface = 1;
	p.query = 1;
	strncpy_nt(p.interface, "default", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("lettherebemagic");
	ck_assert_int_eq(ret, 1);

	handleifselection(&p);

	ck_assert_str_eq(p.interface, "lettherebemagic");
}
END_TEST

START_TEST(handleifselection_selects_highest_traffic_interface_from_database_for_query)
{
	int ret;
	PARAMS p;
	defaultcfg();
	initparams(&p);

	p.defaultiface = 1;
	p.query = 1;
	strncpy_nt(p.interface, "default", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethslow");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethfast");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethone");
	ck_assert_int_eq(ret, 1);

	ret = db_settotal("ethslow", 100, 200);
	ck_assert_int_eq(ret, 1);

	ret = db_settotal("ethfast", 80, 9001);
	ck_assert_int_eq(ret, 1);

	ret = db_settotal("ethone", 1, 1);
	ck_assert_int_eq(ret, 1);

	handleifselection(&p);

	/* interface with most traffic in database is selected */
	ck_assert_str_eq(p.interface, "ethfast");
}
END_TEST

START_TEST(handleifselection_exits_when_no_suitable_interface_is_available_for_traffic)
{
	PARAMS p;
	defaultcfg();
	initparams(&p);

	p.defaultiface = 1;
	p.query = 0;
	p.traffic = 1;
	strncpy_nt(p.interface, "default", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);

	/* database not available and interface listing provides nothing */

	suppress_output();
	handleifselection(&p);
}
END_TEST

START_TEST(handleifselection_can_select_interface_without_database_for_traffic)
{
	PARAMS p;
	defaultcfg();
	initparams(&p);

	p.defaultiface = 1;
	p.query = 0;
	p.traffic = 1;
	strncpy_nt(p.interface, "default", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "firstinterface", 0, 0, 0, 0);
	fake_proc_net_dev("a", "secondinterface", 0, 0, 0, 0);

	suppress_output();
	handleifselection(&p);

	/* first available interface is selected */
	ck_assert_str_eq(p.interface, "firstinterface");
}
END_TEST

START_TEST(handleifselection_exits_if_only_database_shows_interfaces_for_traffic)
{
	int ret;
	PARAMS p;
	defaultcfg();
	initparams(&p);

	p.defaultiface = 1;
	p.query = 0;
	p.traffic = 1;
	strncpy_nt(p.interface, "default", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethsomething");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethnothing");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handleifselection(&p);
}
END_TEST

START_TEST(handleifselection_selects_only_available_interfaces_for_traffic)
{
	int ret;
	PARAMS p;
	defaultcfg();
	initparams(&p);

	p.defaultiface = 1;
	p.query = 0;
	p.traffic = 1;
	strncpy_nt(p.interface, "default", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethnone", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethslow", 0, 0, 0, 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethslow");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethfast");
	ck_assert_int_eq(ret, 1);

	ret = db_settotal("ethslow", 100, 200);
	ck_assert_int_eq(ret, 1);

	ret = db_settotal("ethfast", 80, 9001);
	ck_assert_int_eq(ret, 1);

	handleifselection(&p);

	/* interface with most traffic in database the is also available is selected */
	ck_assert_str_eq(p.interface, "ethslow");
}
END_TEST

START_TEST(handleifselection_selects_only_available_interfaces_and_can_ignore_database_for_traffic)
{
	int ret;
	PARAMS p;
	defaultcfg();
	initparams(&p);

	p.defaultiface = 1;
	p.query = 0;
	p.traffic = 1;
	strncpy_nt(p.interface, "default", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "ethnone", 0, 0, 0, 0);
	fake_proc_net_dev("a", "ethall", 0, 0, 0, 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethslow");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethfast");
	ck_assert_int_eq(ret, 1);

	ret = db_settotal("ethslow", 100, 200);
	ck_assert_int_eq(ret, 1);

	ret = db_settotal("ethfast", 80, 9001);
	ck_assert_int_eq(ret, 1);

	handleifselection(&p);

	/* first available interface is selected if none of the interfaces in database are available */
	ck_assert_str_eq(p.interface, "ethnone");
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
	tcase_add_exit_test(tc_cli, vnstat_handlesetalias_exits_if_no_interface_has_been_specified, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlesetalias_exits_if_given_interface_does_not_exist, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlesetalias_exits_after_setting_alias, 0);
	tcase_add_exit_test(tc_cli, vnstat_handlesetalias_exits_after_clearing_alias, 0);
	tcase_add_exit_test(tc_cli, vnstat_handletrafficmeters_exits_when_interface_is_not_available, 1);
	tcase_add_exit_test(tc_cli, vnstat_handletrafficmeters_exits_when_interface_is_not_available_with_configuration_tips, 1);
	tcase_add_exit_test(tc_cli, vnstat_handletrafficmeters_exits_when_specific_interface_is_not_available, 1);
	tcase_add_test(tc_cli, vnstat_handletrafficmeters_can_calculate_traffic);
	tcase_add_test(tc_cli, vnstat_handletrafficmeters_can_calculate_traffic_and_output_json);
	tcase_add_test(tc_cli, handleifselection_does_nothing_when_interface_has_already_been_selected);
	tcase_add_test(tc_cli, handleifselection_selects_default_interface_if_field_is_filled);
	tcase_add_exit_test(tc_cli, handleifselection_exits_when_no_suitable_interface_is_available_for_query, 1);
	tcase_add_test(tc_cli, handleifselection_selects_only_interface_from_database_for_query);
	tcase_add_test(tc_cli, handleifselection_selects_highest_traffic_interface_from_database_for_query);
	tcase_add_exit_test(tc_cli, handleifselection_exits_when_no_suitable_interface_is_available_for_traffic, 1);
	tcase_add_test(tc_cli, handleifselection_can_select_interface_without_database_for_traffic);
	tcase_add_exit_test(tc_cli, handleifselection_exits_if_only_database_shows_interfaces_for_traffic, 1);
	tcase_add_test(tc_cli, handleifselection_selects_only_available_interfaces_for_traffic);
	tcase_add_test(tc_cli, handleifselection_selects_only_available_interfaces_and_can_ignore_database_for_traffic);
	suite_add_tcase(s, tc_cli);
}
