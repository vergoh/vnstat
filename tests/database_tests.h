#ifndef DATABASE_TESTS_H
#define DATABASE_TESTS_H

#include "dbaccess.h"

void add_database_tests(Suite *s);
int writedb(DATA *data, const char *iface, const char *dirname, int newdb);
int backupdb(const char *current, const char *backup);

#endif
