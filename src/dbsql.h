#ifndef DBSQL_H
#define DBSQL_H

#include <sqlite3.h>

int db_open(int createifnotfound);
int db_close(void);
int db_exec(const char *sql);
int db_create(void);
int db_addinterface(const char *iface);
sqlite3_int64 db_getinterfaceid(const char *iface, const int createifnotfound);
int db_setactive(const char *iface, const int active);
int db_setcounters(const char *iface, const uint64_t rxcounter, const uint64_t txcounter);
int db_getcounters(const char *iface, uint64_t *rxcounter, uint64_t *txcounter);
int db_setalias(const char *iface, const char *alias);
int db_setinfo(const char *name, const char *value, const int createifnotfound);
char *db_getinfo(const char *name);
int db_addtraffic(const char *iface, const uint64_t rx, const uint64_t tx);
int db_addtraffic_dated(const char *iface, const uint64_t rx, const uint64_t tx, const uint64_t timestamp);
int db_removeoldentries(void);
int db_vacuum(void);
int db_begintransaction(void);
int db_committransaction(void);
int db_rollbacktransaction(void);

/* global db */
sqlite3 *db;

#endif
