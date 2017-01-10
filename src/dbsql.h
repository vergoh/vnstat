#ifndef DBSQL_H
#define DBSQL_H

#include <sqlite3.h>

typedef struct dbiflist {
	char interface[32];
	struct dbiflist *next;
} dbiflist;

typedef struct interfaceinfo {
	char name[32], alias[32];
	int active;
	time_t created, updated;
	uint64_t rxcounter, txcounter;
	uint64_t rxtotal, txtotal;
} interfaceinfo;

int db_open(int createifnotfound);
int db_close(void);
int db_exec(const char *sql);
int db_create(void);
int db_addinterface(const char *iface);
uint64_t db_getinterfacecount(void);
uint64_t db_getinterfacecountbyname(const char *iface);
sqlite3_int64 db_getinterfaceid(const char *iface, const int createifnotfound);
int db_setactive(const char *iface, const int active);
int db_setcounters(const char *iface, const uint64_t rxcounter, const uint64_t txcounter);
int db_getcounters(const char *iface, uint64_t *rxcounter, uint64_t *txcounter);
int db_getinterfaceinfo(const char *iface, interfaceinfo *info);
int db_setalias(const char *iface, const char *alias);
int db_setinfo(const char *name, const char *value, const int createifnotfound);
char *db_getinfo(const char *name);
int db_getiflist(dbiflist **dbifl);
int db_addtraffic(const char *iface, const uint64_t rx, const uint64_t tx);
int db_addtraffic_dated(const char *iface, const uint64_t rx, const uint64_t tx, const uint64_t timestamp);
int db_setcreation(const char *iface, const uint64_t timestamp);
int db_settotal(const char *iface, const uint64_t rx, const uint64_t tx);
int db_insertdata(const char *table, const char *iface, const uint64_t rx, const uint64_t tx, const uint64_t timestamp);
int db_removeoldentries(void);
int db_vacuum(void);
int db_begintransaction(void);
int db_committransaction(void);
int db_rollbacktransaction(void);

int dbiflistadd(dbiflist **dbifl, const char *iface);
void dbiflistfree(dbiflist **dbifl);

/* global db */
sqlite3 *db;

#endif
