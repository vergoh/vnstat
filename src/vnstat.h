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
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <sys/file.h>

/*

Note! These are only the default values for settings
and most can be changed later from the config file.

*/

/* location of the database directory */
#define DATABASEDIR "/var/lib/vnstat"

/* on which day should months change */
#define MONTHROTATE 1

/* date output formats for -d, -m, -t and -w */
/* see 'man date' for control codes */
#define DFORMAT "%d.%m."
#define MFORMAT "%b '%y"
#define TFORMAT "%d.%m.%y"

/* characters used for visuals */
#define RXCHAR "%"
#define TXCHAR ":"
#define RXHOURCHAR "r"
#define TXHOURCHAR "t"

/* default interface */
#define DEFIFACE "eth0"

/* default locale */
#define LOCALE "en_US"

/* default maximum bandwidth (Mbit) for all interfaces */
/* 0 = feature disabled */
#define DEFMAXBW 100

/* how many seconds should sampling take by default */
#define DEFSAMPTIME 5

/* maximum time (minutes) between two updates before traffic */
/* for that period will be discarded */
/* set to a little over one hour so that it doesn't break using */
/* cron.hourly like Gentoo seems to do */
#define MAXUPDATEINTERVAL 62

/* default query mode */
/* 0 = normal, 1 = days, 2 = months, 3 = top10 */
/* 4 = dumpdb, 5 = short, 6 = weeks, 7 = hours */
#define DEFQMODE 0

/* how much the boot time can variate between updates (seconds) */
#define BVAR 15

/* use file locking by default */
#define USEFLOCK 1

/* how many times try file locking before giving up */
/* each try takes about a second */
#define LOCKTRYLIMIT 5

/* database version */
/* 1 = 1.0, 2 = 1.1-1.2, 3 = 1.3- */
#define DBVERSION 3

/* version string */
#define VNSTATVERSION "1.6"

/* integer limits */
#define FP32 4294967295ULL
#define FP64 18446744073709551615ULL

/* sampletime in seconds for live traffic */
/* don't use values below 2 */
#define LIVETIME 2

/* internal config structure */
typedef struct {
	int bvar;
	int qmode;
	int sampletime;
	int monthrotate;
	int maxbw;
	int flock;
	char dformat[64], mformat[64], tformat[64];
	char iface[32];
	char locale[32];
	char dbdir[512];
	char rxchar[2], txchar[2], rxhourchar[2], txhourchar[2];
} CFG;

/* internal interface information structure */
typedef struct {
	char name[32];
	int filled;
	uint64_t rx;
	uint64_t tx;
	uint64_t rxp;
	uint64_t txp;
} IFINFO;

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

/* db structure */
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

typedef struct ibw {
	char interface[32];
	int limit;
	struct ibw *next;
} ibwnode;

/* global variables */
DATA data;
CFG cfg;
IFINFO ifinfo;
ibwnode *ifacebw;
int debug;
int intsignal;
