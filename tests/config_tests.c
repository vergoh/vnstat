#include "common.h"
#include "vnstat_tests.h"
#include "config_tests.h"
#include "cfg.h"
#include "cfgoutput.h"
#include "ibw.h"

START_TEST(validatecfg_default_all)
{
	defaultcfg();
	validatecfg(CT_All);
}
END_TEST

START_TEST(validatecfg_default_cli)
{
	defaultcfg();
	validatecfg(CT_CLI);
}
END_TEST

START_TEST(validatecfg_default_daemon)
{
	defaultcfg();
	validatecfg(CT_Daemon);
}
END_TEST

START_TEST(validatecfg_default_image)
{
	defaultcfg();
	validatecfg(CT_Image);
}
END_TEST

START_TEST(validatecfg_does_not_modify_valid_changes)
{
	defaultcfg();
	ck_assert_int_eq(cfg.listhours, LISTHOURS);
	cfg.listhours = 1;
	ck_assert_int_ne(cfg.listhours, LISTHOURS);
	validatecfg(CT_All);
	ck_assert_int_eq(cfg.listhours, 1);
}
END_TEST

START_TEST(validatecfg_restores_invalid_values_back_to_default)
{
	defaultcfg();
	cfg.unitmode = 3;
	cfg.savestatus = 2;
	cfg.listhours = -1;
	suppress_output();
	validatecfg(CT_All);
	ck_assert_int_eq(cfg.unitmode, UNITMODE);
	ck_assert_int_eq(cfg.savestatus, SAVESTATUS);
	ck_assert_int_eq(cfg.listhours, LISTHOURS);
}
END_TEST

START_TEST(validatecfg_can_tune_updateinterval_to_avoid_rollover_issues)
{
	noexit = 1;
	defaultcfg();
	cfg.updateinterval = 60;
	cfg.maxbw = 1000;
	cfg.bwdetection = 1;
	suppress_output();
	validatecfg(CT_Daemon);
	ck_assert_int_ne(cfg.updateinterval, 60);
	ck_assert_int_eq(cfg.updateinterval, UPDATEINTERVAL);
}
END_TEST

START_TEST(validatecfg_has_fallback_for_updateinterval_for_very_fast_interfaces)
{
	noexit = 1;
	defaultcfg();
	cfg.updateinterval = 60;
	cfg.maxbw = 2000;
	cfg.bwdetection = 1;
	suppress_output();
	validatecfg(CT_Daemon);
	ck_assert_int_ne(cfg.updateinterval, 60);
	ck_assert_int_ne(cfg.updateinterval, UPDATEINTERVAL);
	ck_assert_int_eq(cfg.updateinterval, (UPDATEINTERVAL / 2));
}
END_TEST

START_TEST(validatecfg_can_change_estimatestyle_for_images_depending_on_settings)
{
	noexit = 1;
	debug = 1;
	defaultcfg();
	cfg.barshowsrate = 0;
	cfg.estimatebarvisible = 0;
	cfg.estimatestyle = 1;
	suppress_output();

	validatecfg(CT_Image);
	ck_assert_int_eq(cfg.barshowsrate, 0);
	ck_assert_int_eq(cfg.estimatebarvisible, 0);
	ck_assert_int_eq(cfg.estimatestyle, 1);

	cfg.barshowsrate = 1;
	cfg.estimatebarvisible = 0;
	validatecfg(CT_Image);
	ck_assert_int_eq(cfg.barshowsrate, 1);
	ck_assert_int_eq(cfg.estimatebarvisible, 0);
	ck_assert_int_eq(cfg.estimatestyle, 1);

	cfg.barshowsrate = 0;
	cfg.estimatebarvisible = 1;
	validatecfg(CT_Image);
	ck_assert_int_eq(cfg.barshowsrate, 0);
	ck_assert_int_eq(cfg.estimatebarvisible, 1);
	ck_assert_int_eq(cfg.estimatestyle, 1);

	cfg.barshowsrate = 1;
	cfg.estimatebarvisible = 1;
	validatecfg(CT_Image);
	ck_assert_int_eq(cfg.barshowsrate, 1);
	ck_assert_int_eq(cfg.estimatebarvisible, 1);
	ck_assert_int_eq(cfg.estimatestyle, 0);
}
END_TEST

START_TEST(validatecfg_limits_5_minute_result_count_to_available_data_amount)
{
	noexit = 1;
	defaultcfg();
	cfg.fiveminutehours = 10;
	cfg.fivegresultcount = 9001;
	suppress_output();
	validatecfg(CT_Image);
	ck_assert_int_eq(cfg.fiveminutehours, 10);
	ck_assert_int_eq(cfg.fivegresultcount, 120);
}
END_TEST

START_TEST(validatecfg_limits_5_minute_result_count_to_not_be_too_much)
{
	noexit = 1;
	defaultcfg();
	cfg.fiveminutehours = 9001;
	cfg.fivegresultcount = 12345;
	suppress_output();
	validatecfg(CT_Image);
	ck_assert_int_eq(cfg.fiveminutehours, 9001);
	ck_assert_int_eq(cfg.fivegresultcount, FIVEGRESULTCOUNT);
}
END_TEST

START_TEST(validatecfg_does_not_touch_5_minute_result_count_if_data_is_not_being_created)
{
	noexit = 1;
	defaultcfg();
	cfg.fiveminutehours = 0;
	cfg.fivegresultcount = 1234;
	suppress_output();
	validatecfg(CT_Image);
	ck_assert_int_eq(cfg.fiveminutehours, 0);
	ck_assert_int_eq(cfg.fivegresultcount, 1234);
}
END_TEST

START_TEST(validatecfg_is_not_stupid_with_5_minute_result_count_if_there_is_no_data_limit)
{
	noexit = 1;
	defaultcfg();
	cfg.fiveminutehours = -1;
	cfg.fivegresultcount = 1242;
	suppress_output();
	validatecfg(CT_Image);
	ck_assert_int_eq(cfg.fiveminutehours, -1);
	ck_assert_int_eq(cfg.fivegresultcount, 1242);
}
END_TEST

START_TEST(printcfgfile_default)
{
	defaultcfg();
	ck_assert_int_eq(ibwadd("name1", 1), 1);
	ck_assert_int_eq(ibwadd("name2", 2), 1);
	suppress_output();
	printcfgfile();
}
END_TEST

START_TEST(printcfgfile_experimental)
{
	defaultcfg();
	cfg.experimental = 1;
	ck_assert_int_eq(ibwadd("name1", 1), 1);
	ck_assert_int_eq(ibwadd("name2", 2), 1);
	suppress_output();
	printcfgfile();
}
END_TEST

START_TEST(loadcfg_included_default)
{
	ck_assert_int_eq(loadcfg(CFGFILE, CT_All), 1);
}
END_TEST

START_TEST(loadcfg_no_file)
{
	ck_assert_int_eq(loadcfg("", CT_All), 1);
}
END_TEST

START_TEST(loadcfg_nonexistent_file)
{
	suppress_output();
	ck_assert_int_eq(loadcfg("_nosuchfile_", CT_All), 0);
}
END_TEST

START_TEST(loadcfg_not_a_cfgfile)
{
	ck_assert_int_eq(loadcfg("Makefile", CT_All), 1);
}
END_TEST

START_TEST(ibwloadcfg_included_default)
{
	ck_assert_int_eq(ibwloadcfg(CFGFILE), 1);
}
END_TEST

START_TEST(ibwloadcfg_no_file)
{
	ck_assert_int_eq(ibwloadcfg(""), 1);
}
END_TEST

START_TEST(ibwloadcfg_nonexistent_file)
{
	suppress_output();
	ck_assert_int_eq(ibwloadcfg("_nosuchfile_"), 0);
}
END_TEST

START_TEST(ibwloadcfg_not_a_cfgfile)
{
	ck_assert_int_eq(ibwloadcfg("Makefile"), 1);
}
END_TEST

START_TEST(ibwget_with_empty_list_and_no_maxbw)
{
	uint32_t limit;
	cfg.maxbw = 0;
	ibwflush();
	ck_assert_int_eq(ibwget("does_not_exist", &limit), 0);
}
END_TEST

START_TEST(ibwget_with_empty_list_and_maxbw)
{
	int ret;
	uint32_t limit;
	cfg.maxbw = 10;
	ibwflush();
	ret = ibwget("does_not_exist", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 10);
}
END_TEST

START_TEST(ibwget_from_config)
{
	int ret;
	uint32_t limit;
	ck_assert_int_eq(loadcfg(CFGFILE, CT_All), 1);
	ck_assert_int_eq(ibwloadcfg(CFGFILE), 1);
	cfg.maxbw = 10;
	ret = ibwget("ethnone", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 8);
}
END_TEST

START_TEST(ibwadd_single_success)
{
	int ret;
	uint32_t limit;
	cfg.maxbw = 0;
	ck_assert_int_eq(ibwadd("newinterface", 1), 1);
	ret = ibwget("does_not_exist", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);
	ret = ibwget("newinterface", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 1);
}
END_TEST

START_TEST(ibwadd_multi_success)
{
	int ret;
	uint32_t limit;
	cfg.maxbw = 0;
	ck_assert_int_eq(ibwadd("name1", 1), 1);
	ck_assert_int_eq(ibwadd("name2", 2), 1);
	ck_assert_int_eq(ibwadd("name3", 3), 1);
	ck_assert_int_eq(ibwadd("name4", 2), 1);
	ck_assert_int_eq(ibwadd("name5", 1), 1);
	ck_assert_int_eq(ibwadd("name6", 10), 1);

	ret = ibwget("does_not_exist", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);
	ret = ibwget("name1", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 1);
	ret = ibwget("name3", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 3);
	ret = ibwget("name4", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 2);
	ret = ibwget("name6", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 10);
	ret = ibwget("name2", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 2);
	ret = ibwget("name5", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 1);
	ret = ibwget("name1", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 1);
	ret = ibwget("does_not_exist", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);
}
END_TEST

START_TEST(ibwadd_update_success)
{
	int ret;
	uint32_t limit;
	cfg.maxbw = 0;
	ck_assert_int_eq(ibwadd("name1", 1), 1);
	ck_assert_int_eq(ibwadd("name2", 2), 1);
	ret = ibwget("does_not_exist", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);

	ret = ibwget("name1", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 1);
	ret = ibwget("name2", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 2);
	ret = ibwget("does_not_exist", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);

	ck_assert_int_eq(ibwadd("name2", 5), 1);
	ck_assert_int_eq(ibwadd("name1", 4), 1);

	ret = ibwget("name1", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 4);
	ret = ibwget("name2", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 5);
	ret = ibwget("does_not_exist", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);
}
END_TEST

START_TEST(ibwflush_success)
{
	int ret;
	uint32_t limit;
	cfg.maxbw = 0;
	ck_assert_int_eq(ibwadd("name1", 1), 1);
	ck_assert_int_eq(ibwadd("name2", 2), 1);

	ret = ibwget("name1", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 1);
	ret = ibwget("name2", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 2);
	ret = ibwget("does_not_exist", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);

	ibwflush();

	ret = ibwget("name1", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);
	ret = ibwget("name2", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);
	ret = ibwget("does_not_exist", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);

	ck_assert_int_eq(ibwadd("name1", 1), 1);
	ck_assert_int_eq(ibwadd("name2", 2), 1);

	ret = ibwget("name1", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 1);
	ret = ibwget("name2", &limit);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(limit, 2);
	ret = ibwget("does_not_exist", &limit);
	ck_assert_int_eq(ret, 0);
	ck_assert_int_eq(limit, 0);
}
END_TEST

START_TEST(ibwlist_empty)
{
	suppress_output();
	ibwlist();
}
END_TEST

START_TEST(ibwlist_filled)
{
	cfg.maxbw = 0;
	ck_assert_int_eq(ibwadd("name1", 1), 1);
	ck_assert_int_eq(ibwadd("name2", 2), 1);
	suppress_output();
	ibwlist();
}
END_TEST

START_TEST(extractcfgvalue_can_extract)
{
	int ret;
	char value[32], cfgline[32];

	snprintf(cfgline, 32, "one 1");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(value, "1");
}
END_TEST

START_TEST(extractcfgvalue_can_really_extract)
{
	int ret;
	char value[32], cfgline[32];

	snprintf(cfgline, 32, "one\t1");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(value, "1");

	snprintf(cfgline, 32, "one\t\t1");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(value, "1");

	snprintf(cfgline, 32, "one \t 1");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(value, "1");

	snprintf(cfgline, 32, "one \t 1 \t2");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 4);
	ck_assert_str_eq(value, "1 \t2");

	snprintf(cfgline, 32, "one \"1\"");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(value, "1");

	snprintf(cfgline, 32, "one\t\"1\"");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(value, "1");

	snprintf(cfgline, 32, "one \t \"1\"");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(value, "1");

	snprintf(cfgline, 32, "one \t \"1\" \t");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(value, "1");

	snprintf(cfgline, 32, "one \t \"1\" \t2");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(value, "1");

	snprintf(cfgline, 32, "one \t == \t \"1\" \t == well doh");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(value, "1");
}
END_TEST

START_TEST(extractcfgvalue_knows_when_not_to_extract)
{
	int ret;
	char value[32], cfgline[32];

	snprintf(cfgline, 32, "one");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 0);
	ck_assert_str_eq(value, "");
}
END_TEST

START_TEST(extractcfgvalue_really_knows_when_not_to_extract)
{
	int ret;
	char value[32], cfgline[32];

	snprintf(cfgline, 32, "one   \t   ");
	ret = extractcfgvalue(value, 32, cfgline, 3);
	ck_assert_int_eq(ret, 0);
	ck_assert_str_eq(value, "");
}
END_TEST

START_TEST(setcfgvalue_can_set_chars)
{
	int ret;
	char target[32];
	struct cfgsetting cset[] = {{"unused", target, 0, 32, 0}};

	ret = setcfgvalue(&cset[0], "one", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(target, "one");

	ret = setcfgvalue(&cset[0], "1", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(target, "1");

	ret = setcfgvalue(&cset[0], "-", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(target, "-");

	ret = setcfgvalue(&cset[0], "qwe rty uio  ads", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(target, "qwe rty uio  ads");
}
END_TEST

START_TEST(setcfgvalue_can_set_ints)
{
	int ret, target;
	struct cfgsetting cset[] = {{"unused", 0, &target, 0, 0}};

	ret = setcfgvalue(&cset[0], "1", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(target, 1);

	ret = setcfgvalue(&cset[0], "123", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(target, 123);

	ret = setcfgvalue(&cset[0], "-1", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(target, -1);

	ret = setcfgvalue(&cset[0], "-321", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(target, -321);

	ret = setcfgvalue(&cset[0], "0", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(target, 0);
}
END_TEST

START_TEST(setcfgvalue_does_not_exceed_char_limit)
{
	int ret;
	char target[10];
	struct cfgsetting cset[] = {{"unused", target, 0, 5, 0}};

	ret = setcfgvalue(&cset[0], "one", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(target, "one");

	ret = setcfgvalue(&cset[0], "12345", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(target, "1234");

	ret = setcfgvalue(&cset[0], "12  5", "unused");
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(target, "12  ");
}
END_TEST

START_TEST(setcfgvalue_can_do_nothing)
{
	int ret;
	struct cfgsetting cset[] = {{"unused", 0, 0, 0, 0}};

	ret = setcfgvalue(&cset[0], "nothing", "unused");
	ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(configlocale_does_not_crash)
{
	defaultcfg();

	unsetenv("LC_ALL");
	snprintf(cfg.locale, 32, "en_US");
	configlocale();
	snprintf(cfg.locale, 32, "-");
	configlocale();

	setenv("LC_ALL", "en_US", 1);
	snprintf(cfg.locale, 32, "en_US");
	configlocale();
	ck_assert_int_eq(cfg.utflocale, 0);
	snprintf(cfg.locale, 32, "-");
	configlocale();
	ck_assert_int_eq(cfg.utflocale, 0);

	setenv("LC_ALL", "en_US.UTF-8", 1);
	snprintf(cfg.locale, 32, "en_US");
	configlocale();
	ck_assert_int_eq(cfg.utflocale, 1);
	snprintf(cfg.locale, 32, "-");
	configlocale();
	ck_assert_int_eq(cfg.utflocale, 1);
}
END_TEST

void add_config_tests(Suite *s)
{
	TCase *tc_config = tcase_create("Config");
	tcase_add_checked_fixture(tc_config, setup, teardown);
	tcase_add_unchecked_fixture(tc_config, setup, teardown);
	tcase_add_test(tc_config, validatecfg_default_all);
	tcase_add_test(tc_config, validatecfg_default_cli);
	tcase_add_test(tc_config, validatecfg_default_daemon);
	tcase_add_test(tc_config, validatecfg_default_image);
	tcase_add_test(tc_config, validatecfg_does_not_modify_valid_changes);
	tcase_add_test(tc_config, validatecfg_restores_invalid_values_back_to_default);
	tcase_add_test(tc_config, validatecfg_can_tune_updateinterval_to_avoid_rollover_issues);
	tcase_add_test(tc_config, validatecfg_has_fallback_for_updateinterval_for_very_fast_interfaces);
	tcase_add_test(tc_config, validatecfg_can_change_estimatestyle_for_images_depending_on_settings);
	tcase_add_test(tc_config, validatecfg_limits_5_minute_result_count_to_available_data_amount);
	tcase_add_test(tc_config, validatecfg_limits_5_minute_result_count_to_not_be_too_much);
	tcase_add_test(tc_config, validatecfg_does_not_touch_5_minute_result_count_if_data_is_not_being_created);
	tcase_add_test(tc_config, validatecfg_is_not_stupid_with_5_minute_result_count_if_there_is_no_data_limit);
	tcase_add_test(tc_config, printcfgfile_default);
	tcase_add_test(tc_config, printcfgfile_experimental);
	tcase_add_test(tc_config, loadcfg_included_default);
	tcase_add_test(tc_config, loadcfg_no_file);
	tcase_add_test(tc_config, loadcfg_nonexistent_file);
	tcase_add_test(tc_config, loadcfg_not_a_cfgfile);
	tcase_add_test(tc_config, ibwloadcfg_included_default);
	tcase_add_test(tc_config, ibwloadcfg_no_file);
	tcase_add_test(tc_config, ibwloadcfg_nonexistent_file);
	tcase_add_test(tc_config, ibwloadcfg_not_a_cfgfile);
	tcase_add_test(tc_config, ibwget_with_empty_list_and_no_maxbw);
	tcase_add_test(tc_config, ibwget_with_empty_list_and_maxbw);
	tcase_add_test(tc_config, ibwget_from_config);
	tcase_add_test(tc_config, ibwadd_single_success);
	tcase_add_test(tc_config, ibwadd_multi_success);
	tcase_add_test(tc_config, ibwadd_update_success);
	tcase_add_test(tc_config, ibwflush_success);
	tcase_add_test(tc_config, ibwlist_empty);
	tcase_add_test(tc_config, ibwlist_filled);
	tcase_add_test(tc_config, extractcfgvalue_can_extract);
	tcase_add_test(tc_config, extractcfgvalue_can_really_extract);
	tcase_add_test(tc_config, extractcfgvalue_knows_when_not_to_extract);
	tcase_add_test(tc_config, extractcfgvalue_really_knows_when_not_to_extract);
	tcase_add_test(tc_config, setcfgvalue_can_set_chars);
	tcase_add_test(tc_config, setcfgvalue_can_set_ints);
	tcase_add_test(tc_config, setcfgvalue_does_not_exceed_char_limit);
	tcase_add_test(tc_config, setcfgvalue_can_do_nothing);
	tcase_add_test(tc_config, configlocale_does_not_crash);
	suite_add_tcase(s, tc_config);
}
