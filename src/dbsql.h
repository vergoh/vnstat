#ifndef DBSQL_H
#define DBSQL_H

#include <sqlite3.h>

typedef struct dbiflist {
	char interface[32];
	struct dbiflist *next;
} dbiflist;

typedef struct dbdatalist {
	time_t timestamp;
	int64_t rowid;
	uint64_t rx, tx;
	struct dbdatalist *next;
} dbdatalist;

typedef struct dbdatalistinfo {
	uint32_t count;
	time_t maxtime, mintime;
	uint64_t minrx, mintx;
	uint64_t maxrx, maxtx;
	uint64_t min, max;
	uint64_t sumrx, sumtx;
} dbdatalistinfo;

typedef struct interfaceinfo {
	char name[32], alias[32];
	int active;
	time_t created, updated;
	uint64_t rxcounter, txcounter;
	uint64_t rxtotal, txtotal;
} interfaceinfo;

int db_open_ro(void);
int db_open_rw(const int createifnotfound);
int db_open(const int createifnotfound, const int readonly);
int db_validate(const int readonly);
int db_setpragmas(void);
int db_close(void);
int db_exec(const char *sql);
int db_create(void);
int db_addinterface(const char *iface);
int db_removeinterface(const char *iface);
uint64_t db_getinterfacecount(void);
uint64_t db_getinterfacecountbyname(const char *iface);
sqlite3_int64 db_getinterfaceid(const char *iface, const int createifnotfound);
char *db_getinterfaceidin(const char *iface);
int db_setactive(const char *iface, const int active);
int db_setupdated(const char *iface, const time_t timestamp);
int db_setcounters(const char *iface, const uint64_t rxcounter, const uint64_t txcounter);
int db_getcounters(const char *iface, uint64_t *rxcounter, uint64_t *txcounter);
int db_getinterfaceinfo(const char *iface, interfaceinfo *info);
int db_setalias(const char *iface, const char *alias);
int db_setinfo(const char *name, const char *value, const int createifnotfound);
char *db_getinfo(const char *name);
int db_getiflist(dbiflist **dbifl);
char *db_get_date_generator(const int range, const short direct, const char *nowdate);
int db_addtraffic(const char *iface, const uint64_t rx, const uint64_t tx);
int db_addtraffic_dated(const char *iface, const uint64_t rx, const uint64_t tx, const uint64_t timestamp);
int db_setcreation(const char *iface, const time_t timestamp);
int db_settotal(const char *iface, const uint64_t rx, const uint64_t tx);
int db_insertdata(const char *table, const char *iface, const uint64_t rx, const uint64_t tx, const uint64_t timestamp);
int db_removeoldentries(void);
int db_removeoldentries_top(void);
int db_vacuum(void);
int db_begintransaction(void);
int db_committransaction(void);
int db_rollbacktransaction(void);
int db_iserrcodefatal(int errcode);
void db_walcheckpoint(void);

int dbiflistadd(dbiflist **dbifl, const char *iface);
void dbiflistfree(dbiflist **dbifl);

int db_getdata(dbdatalist **dbdata, dbdatalistinfo *listinfo, const char *iface, const char *table, const uint32_t resultlimit);
int db_getdata_range(dbdatalist **dbdata, dbdatalistinfo *listinfo, const char *iface, const char *table, const uint32_t resultlimit, const char *databegin, const char *dataend);
void updatelistinfo(dbdatalistinfo *listinfo, const uint64_t rx, const uint64_t tx, const time_t timestamp);
int dbdatalistadd(dbdatalist **dbdata, const uint64_t rx, const uint64_t tx, const time_t timestamp, const int64_t rowid);
void dbdatalistfree(dbdatalist **dbdata);

unsigned int getqueryinterfacecount(const char *input);
char *getifaceinquery(const char *input);

/* global db */
extern sqlite3 *db;
extern int db_errcode;
extern int db_intransaction;

#endif
