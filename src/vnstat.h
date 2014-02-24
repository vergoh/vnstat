#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/vfs.h>
#include <ctype.h>

/* location of the database directory */
#define DATABASEDIR "/var/lib/vnstat"

/* on which day should months change */
#define MONTHROTATE "1"

/* date output formats for -d, -m, -t and -w */
/* see 'man date' for control codes */
#define DFORMAT "%d.%m."
#define MFORMAT "%b '%y"
#define TFORMAT "%d.%m.%y"

/* default interface */
#define DEFIFACE "eth0"

/* how many seconds should sampling take by default */
#define DEFSAMPTIME "5"

/* default query mode */
/* 0 = normal, 1 = days, 2 = months, 3 = top10 */
/* 4 = dumpdb, 5 = short, 6 = weeks */
#define DEFQMODE "0"

/* how much the boot time can variate between updates (seconds) */
#define BVAR "15"

/* database version */
/* 1 = 1.0, 2 = 1.1-1.2, 3 = 1.3*/
#define DBVERSION 3

/* version string */
#define VNSTATVERSION "1.4"

#ifdef BLIMIT
/* 64 bit limit */
#define FPOINT 18446744073709551616

#else
/* 32 bit limit */
#define FPOINT 4294967296
#endif

typedef struct {
	time_t date;
	uint64_t rx, tx;
} HOUR;

typedef struct {
	time_t date;
	uint64_t rx, tx;
	int rxk, txk;
	int used;
} DAY;

typedef struct {
	time_t month;
	uint64_t rx, tx;
	int rxk, txk;
	int used;
} MONTH;

typedef struct {
	int version;
	char interface[32];
	char nick[32];
	int active;
	uint64_t totalrx, totaltx, currx, curtx;
	int totalrxk, totaltxk;
	time_t lastupdated, created;
	DAY day[30];
	MONTH month[12];
	DAY top10[10];
	HOUR hour[24];
	uint64_t btime;
} DATA;

DATA data;
char procline[512], statline[128];
int debug;
