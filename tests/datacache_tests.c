#include "vnstat_tests.h"
#include "datacache_tests.h"
#include "common.h"
#include "datacache.h"

START_TEST(datacache_can_clear_empty_cache)
{
	datacache *dc = NULL;

	datacache_clear(&dc);
	ck_assert_ptr_eq(dc, NULL);
}
END_TEST

START_TEST(datacache_can_add_to_cache)
{
	int ret;
	datacache *dc = NULL;

	ret = datacache_add(&dc, "eth0", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(dc->interface, "eth0");
	ck_assert_int_eq(dc->active, 1);
	ck_assert_int_eq(dc->filled, 0);
	ck_assert_int_eq(dc->syncneeded, 0);
	ck_assert_int_eq(dc->currx, 0);
	ck_assert_int_eq(dc->curtx, 0);
	ck_assert_ptr_eq(dc->log, NULL);
	ck_assert_ptr_eq(dc->next, NULL);

	datacache_clear(&dc);
	ck_assert_ptr_eq(dc, NULL);
}
END_TEST

START_TEST(datacache_can_add_to_cache_consistently)
{
	int ret;
	datacache *dc = NULL;
	datacache *bookmark = NULL;

	ret = datacache_add(&dc, "eth0", 0);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(dc->interface, "eth0");
	ck_assert_int_eq(dc->active, 1);
	ck_assert_int_eq(dc->filled, 0);
	ck_assert_int_eq(dc->syncneeded, 0);
	ck_assert_int_eq(dc->currx, 0);
	ck_assert_int_eq(dc->curtx, 0);
	ck_assert_ptr_eq(dc->log, NULL);
	ck_assert_ptr_eq(dc->next, NULL);

	bookmark = dc;

	ret = datacache_add(&dc, "eth1", 0);
	ck_assert_int_eq(ret, 1);

	ck_assert_str_eq(dc->interface, "eth1");
	ck_assert_int_eq(dc->active, 1);
	ck_assert_int_eq(dc->filled, 0);
	ck_assert_int_eq(dc->syncneeded, 0);
	ck_assert_int_eq(dc->currx, 0);
	ck_assert_int_eq(dc->curtx, 0);
	ck_assert_ptr_eq(dc->log, NULL);
	ck_assert_ptr_ne(dc->next, NULL);

	ck_assert_str_eq(bookmark->interface, "eth0");

	bookmark = dc;
	ck_assert_str_eq(bookmark->interface, "eth1");

	bookmark = bookmark->next;
	ck_assert_str_eq(bookmark->interface, "eth0");

	datacache_clear(&dc);
	ck_assert_ptr_eq(dc, NULL);
}
END_TEST

START_TEST(datacache_knows_how_to_count)
{
	int ret;
	datacache *dc = NULL;

	ret = datacache_add(&dc, "eth0", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&dc, "eth1", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 2);

	ret = datacache_activecount(&dc);
	ck_assert_int_eq(ret, 2);

	dc->active = 0;

	ret = datacache_activecount(&dc);
	ck_assert_int_eq(ret, 1);

	datacache_clear(&dc);
	ck_assert_ptr_eq(dc, NULL);

	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 0);

	ret = datacache_activecount(&dc);
	ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(datacache_can_seek)
{
	int ret;
	datacache *dc = NULL;
	datacache *iter = NULL;

	ret = datacache_add(&dc, "eth0", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&dc, "eth1", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&dc, "eth2", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&dc, "eth3", 0);
	ck_assert_int_eq(ret, 1);

	iter = dc;
	ck_assert_str_eq(iter->interface, "eth3");
	ret = datacache_seek(&iter, "eth3");
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(iter->interface, "eth3");

	iter = dc;
	ck_assert_str_eq(iter->interface, "eth3");
	ret = datacache_seek(&iter, "eth1");
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(iter->interface, "eth1");

	iter = dc;
	ck_assert_str_eq(iter->interface, "eth3");
	ret = datacache_seek(&iter, "eth0");
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(iter->interface, "eth0");

	iter = dc;
	ck_assert_str_eq(iter->interface, "eth3");
	ret = datacache_seek(&iter, "eth");
	ck_assert_int_eq(ret, 0);

	datacache_clear(&dc);
	ck_assert_ptr_eq(dc, NULL);
}
END_TEST

START_TEST(datacache_can_remove)
{
	int ret;
	datacache *dc = NULL;

	ret = datacache_add(&dc, "eth0", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&dc, "eth1", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&dc, "eth2", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&dc, "eth3", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 4);

	/* invalid removal doesn't cause issues */
	ret = datacache_remove(&dc, "eth4");
	ck_assert_int_eq(ret, 0);
	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 4);
	ck_assert_str_eq(dc->interface, "eth3");

	/* head removal */
	ret = datacache_remove(&dc, "eth3");
	ck_assert_int_eq(ret, 1);
	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 3);
	ck_assert_str_eq(dc->interface, "eth2");

	/* middle removal */
	ret = datacache_remove(&dc, "eth1");
	ck_assert_int_eq(ret, 1);
	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 2);
	ck_assert_str_eq(dc->interface, "eth2");

	/* tail removal */
	ret = datacache_remove(&dc, "eth0");
	ck_assert_int_eq(ret, 1);
	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 1);
	ck_assert_str_eq(dc->interface, "eth2");

	datacache_clear(&dc);
	ck_assert_ptr_eq(dc, NULL);
}
END_TEST

START_TEST(datacache_can_do_stuff)
{
	int ret, i;
	datacache *dc = NULL;
	datacache *iter = NULL;

	ret = datacache_add(&dc, "eth0", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&dc, "eth1", 0);
	ck_assert_int_eq(ret, 1);

	ret = xferlog_add(&dc->log, 2, 1, 2);
	ck_assert_int_eq(ret, 1);

	ret = xferlog_add(&dc->log, 2, 10, 15);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&dc, "eth2", 0);
	ck_assert_int_eq(ret, 1);

	ret = datacache_add(&dc, "eth3", 0);
	ck_assert_int_eq(ret, 1);

	ret = xferlog_add(&dc->log, 2, 2, 2);
	ck_assert_int_eq(ret, 1);

	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 4);

	iter = dc;
	ret = datacache_seek(&iter, "eth1");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(iter->log->timestamp, 2);
	ck_assert_int_eq(iter->log->rx, 11);
	ck_assert_int_eq(iter->log->tx, 17);
	ret = xferlog_add(&iter->log, 2, 10, 20);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(iter->log->rx, 21);
	ck_assert_int_eq(iter->log->tx, 37);

	iter = dc;
	ret = datacache_seek(&iter, "eth0");
	ck_assert_ptr_eq(iter->log, NULL);
	ret = xferlog_add(&iter->log, 2, 12, 34);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(iter->log->timestamp, 2);
	ck_assert_int_eq(iter->log->rx, 12);
	ck_assert_int_eq(iter->log->tx, 34);

	iter = dc;
	ret = datacache_seek(&iter, "eth1");
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(iter->log->timestamp, 2);
	ck_assert_int_eq(iter->log->rx, 21);
	ck_assert_int_eq(iter->log->tx, 37);
	ret = xferlog_add(&iter->log, 10, 12, 34);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(iter->log->timestamp, 10);
	ck_assert_int_eq(iter->log->rx, 12);
	ck_assert_int_eq(iter->log->tx, 34);

	iter = dc;
	ret = datacache_seek(&iter, "eth3");
	for (i=1; i<=10; i++) {
		ret = xferlog_add(&iter->log, i, i*10, i*20);
		ck_assert_int_eq(ret, 1);
	}
	ck_assert_int_eq(iter->log->timestamp, 10);
	ck_assert_int_eq(iter->log->rx, 100);
	ck_assert_int_eq(iter->log->tx, 200);

	/* suppress output to validate that debug function doesn't cause a crash */
	suppress_output();
	datacache_debug(&dc);
	printf("\n");

	ret = datacache_remove(&dc, "eth1");
	ck_assert_int_eq(ret, 1);
	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 3);

	datacache_debug(&dc);
	printf("\n");

	datacache_clear(&dc);
	ck_assert_ptr_eq(dc, NULL);
	datacache_debug(&dc);
	ret = datacache_count(&dc);
	ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(xferlog_can_clear_empty_log)
{
	xferlog *log = NULL;

	xferlog_clear(&log);
	ck_assert_ptr_eq(log, NULL);
}
END_TEST

START_TEST(xferlog_can_log)
{
	int ret;
	xferlog *log = NULL;

	ret = xferlog_add(&log, 1, 1, 1);
	ck_assert_int_eq(ret, 1);

	ck_assert_ptr_ne(log, NULL);

	xferlog_clear(&log);
	ck_assert_ptr_eq(log, NULL);
}
END_TEST

START_TEST(xferlog_can_handle_multiple_entries)
{
	int ret;
	xferlog *log = NULL;
	xferlog *bookmark = NULL;

	ret = xferlog_add(&log, 1, 1, 1);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(log->timestamp, 1);
	ck_assert_int_eq(log->rx, 1);
	ck_assert_int_eq(log->tx, 1);

	bookmark = log;

	ret = xferlog_add(&log, 1, 1, 2);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(log->timestamp, 1);
	ck_assert_int_eq(log->rx, 2);
	ck_assert_int_eq(log->tx, 3);

	ret = xferlog_add(&log, 4, 5, 5);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(log->timestamp, 4);
	ck_assert_int_eq(log->rx, 5);
	ck_assert_int_eq(log->tx, 5);

	ret = xferlog_add(&log, 7, 4, 4);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(log->timestamp, 7);
	ck_assert_int_eq(log->rx, 4);
	ck_assert_int_eq(log->tx, 4);

	ret = xferlog_add(&log, 1, 1, 1);
	ck_assert_int_eq(ret, 1);
	ck_assert_int_eq(log->timestamp, 1);
	ck_assert_int_eq(log->rx, 1);
	ck_assert_int_eq(log->tx, 1);

	/* check that new addition with same timestamp doesn't modify old record */
	ck_assert_int_eq(bookmark->timestamp, 1);
	ck_assert_int_eq(bookmark->rx, 2);
	ck_assert_int_eq(bookmark->tx, 3);

	xferlog_clear(&log);
	ck_assert_ptr_eq(log, NULL);
}
END_TEST

void add_datacache_tests(Suite *s)
{
	TCase *tc_datacache = tcase_create("Datacache");
	tcase_add_test(tc_datacache, datacache_can_clear_empty_cache);
	tcase_add_test(tc_datacache, datacache_can_add_to_cache);
	tcase_add_test(tc_datacache, datacache_can_add_to_cache_consistently);
	tcase_add_test(tc_datacache, datacache_knows_how_to_count);
	tcase_add_test(tc_datacache, datacache_can_seek);
	tcase_add_test(tc_datacache, datacache_can_remove);
	tcase_add_test(tc_datacache, datacache_can_do_stuff);
	tcase_add_test(tc_datacache, xferlog_can_clear_empty_log);
	tcase_add_test(tc_datacache, xferlog_can_log);
	tcase_add_test(tc_datacache, xferlog_can_handle_multiple_entries);
	suite_add_tcase(s, tc_datacache);
}
