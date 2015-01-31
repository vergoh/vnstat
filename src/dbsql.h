#ifndef DBSQL_H
#define DBSQL_H

#include <sqlite3.h>

int db_open();
int db_exec(char *sql);
int db_create();
void db_addinterface(char *iface);
void db_addtraffic(char *iface, int rx, int tx);

/* global db */
sqlite3 *db;

#endif
