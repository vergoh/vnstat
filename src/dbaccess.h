#ifndef DBACCESS_H
#define DBACCESS_H

int readdb(const char *iface, const char *dirname);
void initdb(void);
int writedb(const char *iface, const char *dirname, int newdb);
int backupdb(const char *current, const char *backup);
int convertdb(FILE *db);
int lockdb(int fd, int dbwrite);
int checkdb(const char *iface, const char *dirname);
int removedb(const char *iface, const char *dirname);
void cleanhours(void);
void rotatedays(void);
void rotatemonths(void);


/* version 1.0 database format aka db v1 */
typedef struct {
	char date[11];
	uint64_t rx, tx;
} DAY10;

typedef struct {
	char month[4];
	uint64_t rx, tx;
} MONTH10;

typedef struct {
	int version;
	char interface[32];
	uint64_t totalrx, totaltx, currx, curtx;
	int totalrxk, totaltxk;
	time_t lastupdated, created;
	DAY10 day[30];
	MONTH10 month[12];
	DAY10 top10[10];
	uint64_t btime;
} DATA10;


/* version 1.1-1.2 database format aka db v2 */
typedef struct {
	time_t date;
	uint64_t rx, tx;
	int used;
} DAY12;

typedef struct {
	time_t month;
	uint64_t rx, tx;
	int used;
} MONTH12;

typedef struct {
	int version;
	char interface[32];
	char nick[32];
	int active;
	uint64_t totalrx, totaltx, currx, curtx;
	int totalrxk, totaltxk;
	time_t lastupdated, created;
	DAY12 day[30];
	MONTH12 month[12];
	DAY12 top10[10];
	uint64_t btime;
} DATA12;

#endif
