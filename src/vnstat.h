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

/* location of the database directory */
#define DATABASEDIR "/var/spool/vnstat"

/* on which day should months change */
#define MONTHROTATE "1"

/* date output formats for -d, -m and -t */
/* see 'man date' for control codes */
#define DFORMAT "%d.%m."
#define MFORMAT "%b '%y"
#define TFORMAT "%d.%m.%y"

/* default interface */
#define DEFIFACE "eth0"

/* default query mode */
/* 0 = normal, 1 = days, 2 = months, 3 = top10 */
/* 4 = dumpdb, 5 = short */
#define DEFQMODE "0"

/* database version */
/* 1 = 1.0, 2 = 1.1 */
#define DBVERSION 2

/* version string */
#define VNSTATVERSION "1.1"

/* 32 bit limit */
#define FPOINT 4294967296

typedef struct {
	time_t date;
	uint64_t rx, tx;
	int used;
} DAY;

typedef struct {
	time_t month;
	uint64_t rx, tx;
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
	uint64_t btime;
} DATA;

DATA data;
char procline[512], statline[128];
int debug;
