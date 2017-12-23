#ifndef COMMON_H
#define COMMON_H

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <sys/file.h>
#include <inttypes.h>
#include <syslog.h>
#include <sys/statvfs.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <fcntl.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <ifaddrs.h>
#endif

/* OpenBSD and NetBSD don't support the ' character (decimal conversion) in printf formatting */
#if !defined(__OpenBSD__) && !defined(__NetBSD__)
#define DECCONV "'"
#else
#define DECCONV
#endif

/*

Note! These are only the default values for settings
and most can be changed later from the config file.

*/

/* location of the database directory */
#ifndef DATABASEDIR
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
#define DATABASEDIR "/var/db/vnstat"
#else
#define DATABASEDIR "/var/lib/vnstat"
#endif
#endif
#define DATABASEFILE "vnstat.db"

/* on which day should months change */
#define MONTHROTATE 1

/* date output formats for -d, -m, -t and image header*/
/* see 'man date' for control codes       <1.8 values */
#define DFORMAT "%x"                    /* "%d.%m." */
#define MFORMAT "%b '%y"                /* "%b '%y" */
#define TFORMAT "%x"                    /* "%d.%m.%y" */
#define HFORMAT "%x %H:%M"              /* "%d.%m.%Y %H:%M" */

/* characters used for visuals */
#define RXCHAR "%"
#define TXCHAR ":"
#define RXHOURCHAR "r"
#define TXHOURCHAR "t"

/* unit mode */
/* 0 = KiB/MiB/GiB/TiB, 1 = KB/MB/GB/TB */
#define UNITMODE 0

/* rate unit mode */
/* 0 = Kibit/s..., 1 = kbit/s... */
#define RATEUNITMODE 1

/* output style */
/* 0 = minimal/narrow, 1 = bars everywhere */
/* 2 = same as 1 + rate in summary and weekly */
/* 3 = rate everywhere */
#define OSTYLE 3

/* rate in vnstati summary output */
#define SUMMARYRATE 1

/* layout of summary output in vnstati */
#define SUMMARYLAYOUT 1

/* rate in vnstati hourly output */
#define HOURLYRATE 1

/* rate unit */
/* 0 = bytes, 1 = bits */
#define RATEUNIT 1

/* default interface */
#ifndef DEFIFACE
#define DEFIFACE "eth0"
#endif

/* default locale */
#define LOCALE "-"

/* bandwidth detection, 0 = feature disabled */
#define BWDETECT 1
#define BWDETECTINTERVAL 5

/* default maximum bandwidth (Mbit) for all interfaces */
/* 0 = feature disabled */
#define DEFMAXBW 1000

/* maximum allowed config value for bandwidth */
#define BWMAX 50000

/* how many seconds should sampling take by default */
#define DEFSAMPTIME 5

/* maximum time (minutes) between two updates before traffic */
/* for that period will be discarded */
/* set to a little over one hour so that it doesn't break using */
/* cron.hourly like Gentoo seems to do */
#define MAXUPDATEINTERVAL 62

/* default query mode */
/* 0 = normal, 1 = days, 2 = months, 3 = top10 */
/* 4 = exportdb, 5 = short, 6 = weeks, 7 = hours */
/* 8 = xml */
#define DEFQMODE 0

/* how much the boot time can variate between updates (seconds) */
#define BVAR 15

/* check disk space by default */
#define USESPACECHECK 1

/* use file locking by default */
#define USEFLOCK 1

/* log trafficless days by default */
#define TRAFLESSDAY 1

/* assume that locale can be UTF-n when enabled */
#define UTFLOCALE 1

/* how many times try file locking before giving up */
/* each try takes about a second */
#define LOCKTRYLIMIT 5

/* database version */
/* 1 = 1.0, 2 = 1.1-1.2, 3 = 1.3- */
#define DBVERSION 3

/* xml format version */
/* 1 = 1.7- */
#define XMLVERSION 1

/* json format version */
/* 1 = 1.13- */
#define JSONVERSION 1

/* --oneline format version */
#define ONELINEVERSION 1

/* integer limits */
#define MAX32 4294967295ULL
#define MAX64 18446744073709551615ULL

/* sampletime in seconds for live traffic */
/* don't use values below 2 */
#define LIVETIME 2

/* /proc/net/dev */
#ifndef PROCNETDEV
#define PROCNETDEV "/proc/net/dev"
#endif

/* /sys/class/net */
#ifndef SYSCLASSNET
#define SYSCLASSNET "/sys/class/net"
#endif

/* daemon defaults */
#define UPDATEINTERVAL 30
#define POLLINTERVAL 5
#define SAVEINTERVAL 5
#define OFFSAVEINTERVAL 30
#define SAVESTATUS 1
#define USELOGGING 2
#define CREATEDIRS 1
#define UPDATEFILEOWNER 1
#define LOGFILE "/var/log/vnstat/vnstat.log"
#define PIDFILE "/var/run/vnstat/vnstat.pid"

/* no transparency by default */
#define TRANSBG 0

/* default colors */
#define CBACKGROUND "FFFFFF"
#define CEDGE "AEAEAE"
#define CHEADER "606060"
#define CHEADERTITLE "FFFFFF"
#define CHEADERDATE "FFFFFF"
#define CTEXT "000000"
#define CLINE "B0B0B0"
#define CLINEL "-"
#define CRX "92CF00"
#define CRXD "-"
#define CTX "606060"
#define CTXD "-"

/* internal config structure */
typedef struct {
	char dformat[64], mformat[64], tformat[64], hformat[64];
	char iface[32];
	char locale[32];
	char dbdir[512];
	char rxchar[2], txchar[2], rxhourchar[2], txhourchar[2];
	char cbg[8], cedge[8], cheader[8], cheadertitle[8], cheaderdate[8], ctext[8];
	char cline[8], clinel[8], cvnstat[8], crx[8], crxd[8], ctx[8], ctxd[8];
	int32_t unitmode, rateunitmode, rateunit, bvar, qmode, sampletime, hourlyrate, summaryrate;
	int32_t monthrotate, maxbw, flock, spacecheck, traflessday, transbg, slayout, ostyle;
	char cfgfile[512], logfile[512], pidfile[512];
	char daemonuser[33], daemongroup[33];
	int32_t updateinterval, pollinterval, saveinterval, offsaveinterval, savestatus, uselogging;
	int32_t createdirs, updatefileowner, bwdetection, bwdetectioninterval, utflocale;
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

typedef struct ibwnode {
	char interface[32];
	uint32_t limit;
	uint32_t fallback;
	short retries;
	time_t detected;
	struct ibwnode *next;
} ibwnode;

typedef enum PrintType {
	PT_Info = 0,
	PT_Error,
	PT_Config,
	PT_Multiline,
	PT_ShortMultiline
} PrintType;

/* common functions */
int printe(PrintType type);
int logprint(PrintType type);
int verifylogaccess(void);
int dmonth(int month);
uint32_t mosecs(void);
uint64_t countercalc(const uint64_t *a, const uint64_t *b);
void addtraffic(uint64_t *destmb, int *destkb, const uint64_t srcmb, const int srckb);
uint64_t mbkbtokb(uint64_t mb, uint64_t kb);
char *strncpy_nt(char *dest, const char *src, size_t n);
int isnumeric(const char *s);
void panicexit(const char *sourcefile, const int sourceline) __attribute__((noreturn));
char *getversion(void);

/* global variables */
DATA data;
CFG cfg;
IFINFO ifinfo;
char errorstring[512];
ibwnode *ifacebw;
int debug;
int noexit;      /* = running as daemon if 2 */
int intsignal;
int pidfile;
int disableprints;

#endif
