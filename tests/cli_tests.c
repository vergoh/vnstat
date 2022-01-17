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

START_TEST(vnstat_handleremoveinterface_removes_interface)
{
	int ret;
	PARAMS p;

	initparams(&p);
	p.removeiface = 1;
	p.defaultiface = 0;
	p.force = 1;
	strncpy_nt(p.interface, "known", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("known");
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecountbyname("known");
	ck_assert_int_eq(ret, 1);

	suppress_output();
	handleremoveinterface(&p);

	ret = (int)db_getinterfacecountbyname("known");
	ck_assert_int_eq(ret, 0);
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

START_TEST(vnstat_handlerenameinterface_renames_interface)
{
	int ret;
	PARAMS p;

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

	ret = (int)db_getinterfacecountbyname("oldname");
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecountbyname("newname");
	ck_assert_int_eq(ret, 0);

	suppress_output();
	handlerenameinterface(&p);

	ret = (int)db_getinterfacecountbyname("oldname");
	ck_assert_int_eq(ret, 0);

	ret = (int)db_getinterfacecountbyname("newname");
	ck_assert_int_eq(ret, 1);
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

START_TEST(vnstat_handleaddinterface_adds_interface)
{
	int ret;
	PARAMS p;

	initparams(&p);
	p.addiface = 1;
	p.defaultiface = 0;
	cfg.spacecheck = 0;
	strncpy_nt(p.interface, "newiface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "newiface", 0, 0, 0, 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecountbyname("newiface");
	ck_assert_int_eq(ret, 0);

	suppress_output();
	handleaddinterface(&p);

	ret = (int)db_getinterfacecountbyname("newiface");
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(vnstat_handleaddinterface_can_be_forced_to_add_interface_that_does_not_exist)
{
	int ret;
	PARAMS p;

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

	ret = (int)db_getinterfacecountbyname("newiface");
	ck_assert_int_eq(ret, 0);

	suppress_output();
	handleaddinterface(&p);

	ret = (int)db_getinterfacecountbyname("newiface");
	ck_assert_int_eq(ret, 1);
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

START_TEST(vnstat_handlesetalias_sets_alias)
{
	int ret;
	PARAMS p;
	interfaceinfo info;
	initparams(&p);
	p.setalias = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "ethiface", 32);
	strncpy_nt(p.alias, "The Internet", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethiface");
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfaceinfo("ethiface", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(info.alias, "");

	suppress_output();
	handlesetalias(&p);

	ret = db_getinterfaceinfo("ethiface", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(info.alias, "The Internet");
}
END_TEST

START_TEST(vnstat_handlesetalias_clears_alias)
{
	int ret;
	PARAMS p;
	interfaceinfo info;
	initparams(&p);
	p.setalias = 1;
	p.defaultiface = 0;
	strncpy_nt(p.interface, "ethiface", 32);
	strncpy_nt(p.alias, "", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("ethiface");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("ethiface", "Local network");
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfaceinfo("ethiface", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(info.alias, "Local network");

	suppress_output();
	handlesetalias(&p);

	ret = db_getinterfaceinfo("ethiface", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(info.alias, "");
}
END_TEST

START_TEST(vnstat_handleaddinterface_can_also_set_alias_after_adding_interface)
{
	int ret;
	PARAMS p;
	interfaceinfo info;

	initparams(&p);
	p.addiface = 1;
	p.setalias = 1;
	p.defaultiface = 0;
	cfg.spacecheck = 0;
	strncpy_nt(p.interface, "newiface", 32);
	strncpy_nt(p.alias, "The Interface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "newiface", 0, 0, 0, 0);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = (int)db_getinterfacecountbyname("newiface");
	ck_assert_int_eq(ret, 0);

	suppress_output();
	handleaddinterface(&p);

	ret = (int)db_getinterfacecountbyname("newiface");
	ck_assert_int_eq(ret, 1);

	ret = db_getinterfaceinfo("newiface", &info);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(info.alias, "The Interface");
}
END_TEST

START_TEST(vnstat_handletrafficmeters_exits_when_interface_is_not_available)
{
	PARAMS p;
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

START_TEST(vnstat_handletrafficmeters_can_handle_interface_merge_using_first_interface)
{
	PARAMS p;
	initparams(&p);
	p.traffic = 1;
	p.defaultiface = 0;
	cfg.qmode = 1;
	cfg.sampletime = 0;
	strncpy_nt(p.interface, "someiface+anotherinterface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "someiface", 0, 0, 0, 0);

	suppress_output();
	handletrafficmeters(&p);
}
END_TEST

START_TEST(vnstat_handletrafficmeters_can_calculate_traffic_and_output_json)
{
	PARAMS p;
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

START_TEST(vnstat_handletrafficmeters_livetraffic_does_not_crash)
{
	PARAMS p;
	initparams(&p);
	p.livetraffic = 1;
	p.defaultiface = 0;
	cfg.qmode = 1;
	cfg.ostyle = 0;
	cfg.sampletime = 0;
	strncpy_nt(p.interface, "someiface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "someiface", 0, 0, 0, 0);

	suppress_output();
	handletrafficmeters(&p);
}
END_TEST

START_TEST(vnstat_handletrafficmeters_livetraffic_does_not_crash_with_interface_merge)
{
	PARAMS p;
	initparams(&p);
	p.livetraffic = 1;
	p.defaultiface = 0;
	cfg.qmode = 1;
	cfg.ostyle = 0;
	cfg.sampletime = 0;
	strncpy_nt(p.interface, "someiface+anotherinterface", 32);

	ck_assert_int_eq(remove_directory(TESTDIR), 1);
	fake_proc_net_dev("w", "someiface", 0, 0, 0, 0);

	suppress_output();
	handletrafficmeters(&p);
}
END_TEST

START_TEST(vnstat_handletrafficmeters_livetraffic_does_not_crash_with_json)
{
	PARAMS p;
	initparams(&p);
	p.livetraffic = 1;
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

START_TEST(showalerthelp_does_not_crash)
{
	suppress_output();
	showalerthelp();
}
END_TEST

START_TEST(parsealertargs_knows_which_parameters_belong_to_it)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "2", "--days", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 0);
	ck_assert_int_eq(p.alertexit, 0);
	ck_assert_int_eq(p.alerttype, 0);
	ck_assert_int_eq(p.alertcondition, 0);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_helps_when_asked)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "2", "--help", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 0);
	ck_assert_int_eq(p.alertexit, 0);
	ck_assert_int_eq(p.alerttype, 0);
	ck_assert_int_eq(p.alertcondition, 0);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_can_set_parameters)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "2", "y", "total", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	debug = 1;
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(p.alertoutput, 1);
	ck_assert_int_eq(p.alertexit, 2);
	ck_assert_int_eq(p.alerttype, 4);
	ck_assert_int_eq(p.alertcondition, 3);
	ck_assert_int_eq(p.alertlimit, 5120);
}
END_TEST

START_TEST(parsealertargs_can_validate_output)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "a", "2", "y", "total", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 0);
	ck_assert_int_eq(p.alertexit, 0);
	ck_assert_int_eq(p.alerttype, 0);
	ck_assert_int_eq(p.alertcondition, 0);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_can_validate_output_range)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "4", "2", "y", "total", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 4);
	ck_assert_int_eq(p.alertexit, 0);
	ck_assert_int_eq(p.alerttype, 0);
	ck_assert_int_eq(p.alertcondition, 0);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_can_validate_exit)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "b", "y", "total", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 1);
	ck_assert_int_eq(p.alertexit, 0);
	ck_assert_int_eq(p.alerttype, 0);
	ck_assert_int_eq(p.alertcondition, 0);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_can_validate_exit_range)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "4", "y", "total", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 1);
	ck_assert_int_eq(p.alertexit, 4);
	ck_assert_int_eq(p.alerttype, 0);
	ck_assert_int_eq(p.alertcondition, 0);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_knows_first_useless_parameter_combination)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "0", "0", "y", "total", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 0);
	ck_assert_int_eq(p.alertexit, 0);
	ck_assert_int_eq(p.alerttype, 0);
	ck_assert_int_eq(p.alertcondition, 0);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_knows_second_useless_parameter_combination)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "0", "1", "y", "total", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 0);
	ck_assert_int_eq(p.alertexit, 1);
	ck_assert_int_eq(p.alerttype, 0);
	ck_assert_int_eq(p.alertcondition, 0);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_can_validate_type)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "2", "a", "total", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 1);
	ck_assert_int_eq(p.alertexit, 2);
	ck_assert_int_eq(p.alerttype, 0);
	ck_assert_int_eq(p.alertcondition, 0);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_can_validate_condition)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "2", "y", "total_recall", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 1);
	ck_assert_int_eq(p.alertexit, 2);
	ck_assert_int_eq(p.alerttype, 4);
	ck_assert_int_eq(p.alertcondition, 0);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_knows_first_invalid_condition_combination)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "2", "3", "y", "rx_estimate", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 2);
	ck_assert_int_eq(p.alertexit, 3);
	ck_assert_int_eq(p.alerttype, 4);
	ck_assert_int_eq(p.alertcondition, 4);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_knows_second_invalid_condition_combination)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "3", "2", "y", "tx_estimate", "5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 3);
	ck_assert_int_eq(p.alertexit, 2);
	ck_assert_int_eq(p.alerttype, 4);
	ck_assert_int_eq(p.alertcondition, 5);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_can_validate_limit_as_integer)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "2", "y", "total", "5.5", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 1);
	ck_assert_int_eq(p.alertexit, 2);
	ck_assert_int_eq(p.alerttype, 4);
	ck_assert_int_eq(p.alertcondition, 3);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_can_validate_limit_as_non_zero)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "2", "y", "total", "0", "KiB", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 1);
	ck_assert_int_eq(p.alertexit, 2);
	ck_assert_int_eq(p.alerttype, 4);
	ck_assert_int_eq(p.alertcondition, 3);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_can_validate_limit_unit)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "2", "y", "total", "5", "KeK", NULL};

	defaultcfg();
	initparams(&p);
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 1);
	ck_assert_int_eq(p.alertexit, 2);
	ck_assert_int_eq(p.alerttype, 4);
	ck_assert_int_eq(p.alertcondition, 3);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(parsealertargs_knows_the_64_bit_limit_regardless_of_used_unit)
{
	int ret;
	PARAMS p;
	char *argv[] = {"--alert", "1", "2", "y", "total", "16", "EiB", NULL};

	defaultcfg();
	initparams(&p);
	debug = 1;
	suppress_output();

	ret = parsealertargs(&p, argv);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(p.alertoutput, 1);
	ck_assert_int_eq(p.alertexit, 2);
	ck_assert_int_eq(p.alerttype, 4);
	ck_assert_int_eq(p.alertcondition, 3);
	ck_assert_int_eq(p.alertlimit, 0);
}
END_TEST

START_TEST(handleshowalert_requires_interface_to_be_specified)
{
	PARAMS p;

	defaultcfg();
	initparams(&p);
	suppress_output();
	p.alert = 1;
	p.defaultiface = 1;

	handleshowalert(&p);
}
END_TEST

START_TEST(validateinterface_does_not_use_alias_if_interface_names_matches)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	strncpy_nt(p.interface, "eth0", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth0", "LAN");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth1", "eth0");
	ck_assert_int_eq(ret, 1);

	validateinterface(&p);

	ck_assert_str_eq(p.interface, "eth0");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(validateinterface_supports_interface_merges)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	strncpy_nt(p.interface, "eth0+eth1", 32);

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth0", "LAN");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth1", "Internet");
	ck_assert_int_eq(ret, 1);

	validateinterface(&p);

	ck_assert_str_eq(p.interface, "eth0+eth1");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(validateinterface_detects_if_not_all_interfaces_are_available_for_merge)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	strncpy_nt(p.interface, "eth0+eth2", 32);
	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth0", "LAN");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth1", "Internet");
	ck_assert_int_eq(ret, 1);

	validateinterface(&p);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(validateinterface_detects_if_not_all_interfaces_are_unique_for_merge)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	strncpy_nt(p.interface, "eth0+eth0", 32);
	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth0", "LAN");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth1", "Internet");
	ck_assert_int_eq(ret, 1);

	validateinterface(&p);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(validateinterface_uses_all_matching_methods_if_no_match_for_exact_name_is_found)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	debug = 1;
	cfg.ifacematchmethod = 3;
	strncpy_nt(p.interface, "inter", 32);
	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth0", "LAN");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth1", "Internet");
	ck_assert_int_eq(ret, 1);

	validateinterface(&p);

	ck_assert_str_eq(p.interface, "eth1");

	ret = db_close();
	ck_assert_int_eq(ret, 1);
}
END_TEST

START_TEST(validateinterface_knows_when_to_give_up_searching)
{
	int ret;
	PARAMS p;

	defaultcfg();
	initparams(&p);
	debug = 1;
	cfg.ifacematchmethod = 3;
	strncpy_nt(p.interface, "outer", 32);
	suppress_output();

	ret = db_open_rw(1);
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth0");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth0", "LAN");
	ck_assert_int_eq(ret, 1);

	ret = db_addinterface("eth1");
	ck_assert_int_eq(ret, 1);

	ret = db_setalias("eth1", "Internet");
	ck_assert_int_eq(ret, 1);

	validateinterface(&p);

	ret = db_close();
	ck_assert_int_eq(ret, 1);
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
	tcase_add_test(tc_cli, vnstat_handleremoveinterface_removes_interface);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_if_no_interface_has_been_specified, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_if_new_interface_name_is_not_given, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_if_given_interface_does_not_exist, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_if_new_interface_name_already_exist, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlerenameinterface_exits_if_force_is_not_used, 1);
	tcase_add_test(tc_cli, vnstat_handlerenameinterface_renames_interface);
	tcase_add_exit_test(tc_cli, vnstat_handleaddinterface_exits_if_no_interface_has_been_specified, 1);
	tcase_add_exit_test(tc_cli, vnstat_handleaddinterface_exits_if_interface_already_exist_in_database, 1);
	tcase_add_exit_test(tc_cli, vnstat_handleaddinterface_exits_if_interface_does_not_exist, 1);
	tcase_add_test(tc_cli, vnstat_handleaddinterface_adds_interface);
	tcase_add_test(tc_cli, vnstat_handleaddinterface_can_be_forced_to_add_interface_that_does_not_exist);
	tcase_add_exit_test(tc_cli, vnstat_handlesetalias_exits_if_no_interface_has_been_specified, 1);
	tcase_add_exit_test(tc_cli, vnstat_handlesetalias_exits_if_given_interface_does_not_exist, 1);
	tcase_add_test(tc_cli, vnstat_handlesetalias_sets_alias);
	tcase_add_test(tc_cli, vnstat_handlesetalias_clears_alias);
	tcase_add_test(tc_cli, vnstat_handleaddinterface_can_also_set_alias_after_adding_interface);
	tcase_add_exit_test(tc_cli, vnstat_handletrafficmeters_exits_when_interface_is_not_available, 1);
	tcase_add_exit_test(tc_cli, vnstat_handletrafficmeters_exits_when_interface_is_not_available_with_configuration_tips, 1);
	tcase_add_exit_test(tc_cli, vnstat_handletrafficmeters_exits_when_specific_interface_is_not_available, 1);
	tcase_add_test(tc_cli, vnstat_handletrafficmeters_can_calculate_traffic);
	tcase_add_test(tc_cli, vnstat_handletrafficmeters_can_handle_interface_merge_using_first_interface);
	tcase_add_test(tc_cli, vnstat_handletrafficmeters_can_calculate_traffic_and_output_json);
	tcase_add_test(tc_cli, vnstat_handletrafficmeters_livetraffic_does_not_crash);
	tcase_add_test(tc_cli, vnstat_handletrafficmeters_livetraffic_does_not_crash_with_interface_merge);
	tcase_add_test(tc_cli, vnstat_handletrafficmeters_livetraffic_does_not_crash_with_json);
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
	tcase_add_test(tc_cli, showalerthelp_does_not_crash);
	tcase_add_test(tc_cli, parsealertargs_knows_which_parameters_belong_to_it);
	tcase_add_test(tc_cli, parsealertargs_helps_when_asked);
	tcase_add_test(tc_cli, parsealertargs_can_set_parameters);
	tcase_add_test(tc_cli, parsealertargs_can_validate_output);
	tcase_add_test(tc_cli, parsealertargs_can_validate_output_range);
	tcase_add_test(tc_cli, parsealertargs_can_validate_exit);
	tcase_add_test(tc_cli, parsealertargs_can_validate_exit_range);
	tcase_add_test(tc_cli, parsealertargs_knows_first_useless_parameter_combination);
	tcase_add_test(tc_cli, parsealertargs_knows_second_useless_parameter_combination);
	tcase_add_test(tc_cli, parsealertargs_can_validate_type);
	tcase_add_test(tc_cli, parsealertargs_can_validate_condition);
	tcase_add_test(tc_cli, parsealertargs_knows_first_invalid_condition_combination);
	tcase_add_test(tc_cli, parsealertargs_knows_second_invalid_condition_combination);
	tcase_add_test(tc_cli, parsealertargs_can_validate_limit_as_integer);
	tcase_add_test(tc_cli, parsealertargs_can_validate_limit_as_non_zero);
	tcase_add_test(tc_cli, parsealertargs_can_validate_limit_unit);
	tcase_add_test(tc_cli, parsealertargs_knows_the_64_bit_limit_regardless_of_used_unit);
	tcase_add_exit_test(tc_cli, handleshowalert_requires_interface_to_be_specified, 1);
	tcase_add_test(tc_cli, validateinterface_does_not_use_alias_if_interface_names_matches);
	tcase_add_test(tc_cli, validateinterface_supports_interface_merges);
	tcase_add_exit_test(tc_cli, validateinterface_detects_if_not_all_interfaces_are_available_for_merge, 1);
	tcase_add_exit_test(tc_cli, validateinterface_detects_if_not_all_interfaces_are_unique_for_merge, 1);
	tcase_add_test(tc_cli, validateinterface_uses_all_matching_methods_if_no_match_for_exact_name_is_found);
	tcase_add_exit_test(tc_cli, validateinterface_knows_when_to_give_up_searching, 1);
	suite_add_tcase(s, tc_cli);
}
