#ifndef DBSQL_H
#define DBSQL_H

#include <sqlite3.h>

int db_open(int createifnotfound);
int db_close(void);
int db_exec(char *sql);
int db_create(void);
int db_addinterface(char *iface);
sqlite3_int64 db_getinterfaceid(char *iface, int createifnotfound);
int db_setactive(char *iface, int active);
int db_setalias(char *iface, char *alias);
int db_setinfo(char *name, char *value, int createifnotfound);
char *db_getinfo(char *name);
int db_addtraffic(char *iface, uint64_t rx, uint64_t tx);

/* global db */
sqlite3 *db;

#endif
