#ifndef DBSQL_TESTS_H
#define DBSQL_TESTS_H

uint64_t get_timestamp(const int year, const int month, const int day, const int hour, const int minute);
void range_test_month_setup(void);
void range_test_hour_setup(void);
void add_dbsql_tests(Suite *s);

#endif
