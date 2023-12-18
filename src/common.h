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
#include <sys/time.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#define BSD_VNSTAT
#endif

/* OpenBSD and NetBSD don't support the ' character (decimal conversion) in printf formatting */
#if !defined(__OpenBSD__) && !defined(__NetBSD__)
#define DECCONV "'"
#else
#define DECCONV
#endif

/* used in debug to get function name */
#if __STDC_VERSION__ < 199901L
#if __GNUC__ >= 2
#define __func__ __FUNCTION__
#else
#define __func__ "function"
#endif
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

/* database file name */
#define DATABASEFILE "vnstat.db"

/* modifier used for sqlite date and time functions */
#define DATABASELOCALTIMEMODIFIER ", 'localtime'"

/* on which day should months change */
#define MONTHROTATE 1
#define MONTHROTATEYEARS 0

/* date output formats for -d, -m, -t and image header*/
/* see 'man date' for control codes      1.x values     <1.8 values */
#define DFORMAT "%Y-%m-%d"		 /* "%x"         "%d.%m." */
#define MFORMAT "%Y-%m"			 /* "%b '%y"     "%b '%y" */
#define TFORMAT "%Y-%m-%d"		 /* "%x"         "%d.%m.%y" */
#define HFORMAT "%Y-%m-%d %H:%M" /* "%x %H:%M"   "%d.%m.%Y %H:%M" */

#ifndef DATETIMEFORMAT
#define DATETIMEFORMAT "%Y-%m-%d %H:%M:%S"
#endif

#ifndef DATETIMEFORMATWITHOUTSECS
#define DATETIMEFORMATWITHOUTSECS "%Y-%m-%d %H:%M"
#endif

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
/* 2 = same as 1 + rate in summary */
/* 3 = rate everywhere */
#define OSTYLE 3
#define ESTIMATEBARVISIBLE 1

/* interface order */
/* 0 = alphabetical by name, 1 = alphabetical by alias */
#define INTERFACEORDER 0

/* rate in vnstati summary output */
#define SUMMARYRATE 1

/* rate in vnstati hourly output */
#define HOURLYRATE 1

/* rate unit */
/* 0 = bytes, 1 = bits */
#define RATEUNIT 1

/* number of decimals */
#define DEFAULTDECIMALS 2
#define HOURLYDECIMALS 1

/* hourly section style */
#define HOURLYSTYLE 2

/* default interface */
#ifndef DEFIFACE
#define DEFIFACE ""
#endif

/* default locale */
#define LOCALE "-"

/* bandwidth detection, 0 = feature disabled */
#define BWDETECT 1
#define BWDETECTINTERVAL 5

/* default maximum bandwidth (Mbit) for all interfaces */
/* 0 = feature disabled */
#define DEFMAXBW 1000

/* animation visibility in -l / --live */
#define LIVESPINNER 1

/* maximum allowed config value for bandwidth */
#define BWMAX 50000

/* how many seconds should sampling take by default */
#define DEFSAMPTIME 5

/* default query mode */
/* 0 = normal, 1 = days, 2 = months, 3 = top, 5 = short */
/* 7 = hours, 8 = xml, 9 = one line, 10 = json */
#define DEFQMODE 0

/* interface match method */
/* 0 = interface name exact case sensitive, 1 = 0 + case sensitive exact alias */
/* 2 = 1 + case insensitive exact alias, 3 = 2 + case insensitive beginning of alias */
#define IFACEMATCHMETHOD 3

/* estimate line visibility and text */
#define ESTIMATEVISIBLE 1
#define ESTIMATETEXT "estimated"

/* how much the boot time can variate between updates (seconds) */
#define BVAR 15

/* check disk space by default */
#define USESPACECHECK 1

/* create trafficless entries by default */
#define TRAFFICLESSENTRIES 1

/* list outputs */
#define LISTFIVEMINS 24
#define LISTHOURS 24
#define LISTDAYS 30
#define LISTMONTHS 12
#define LISTYEARS 0
#define LISTTOP 10
#define LISTJSONXML 0

/* data retention defaults */
#define FIVEMINUTEHOURS 48
#define HOURLYDAYS 4
#define DAILYDAYS 62
#define MONTHLYMONTHS 25
#define YEARLYYEARS -1
#define TOPDAYENTRIES 20

/* assume that locale can be UTF-n when enabled */
#define UTFLOCALE 1

/* 1 = 2.0 */
#define SQLDBVERSION "1"

/* xml format version */
/* 1 = 1.7-1.16, 2 = 2.0 */
#define XMLVERSION 2

/* json format version */
/* 1 = 1.13-1.16, 2 = 2.0 */
#define JSONVERSION "2"

/* json format version, -tr */
/* 1 = 1.18- */
#define JSONVERSION_TR 1

/* json format version, --live */
/* 1 = 1.18- */
#define JSONVERSION_LIVE 1

/*json format version, --alert */
/* 1 = 2.12- */
#define JSONALERT "1"

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
#define UPDATEINTERVAL 20
#define TIMESYNCWAIT 5
#define POLLINTERVAL 5
#define SAVEINTERVAL 5
#define OFFSAVEINTERVAL 30
#define RESCANONSAVE 1
#define ALWAYSADD 0
#define SAVESTATUS 1
#define USELOGGING 2
#define CREATEDIRS 1
#define UPDATEFILEOWNER 1
#define LOGFILE "/var/log/vnstat/vnstat.log"
#define PIDFILE "/var/run/vnstat/vnstat.pid"
#define IS64BIT -2
#define WALDB 0
#define WALDBCHECKPOINTINTERVALMINS 240
#define SLOWDBWARNLIMIT 4.0 // needs to be less than DBREADTIMEOUTSECS
#define DBSYNCHRONOUS -1
#define USEUTC 0
#define VACUUMONSTARTUP 1
#define VACUUMONHUPSIGNAL 1

/* database read timeout */
#define DBREADTIMEOUTSECS 5

/* no transparency by default */
#define TRANSBG 0

/* small fonts by default */
#define LARGEFONTS 0

/* no extra space between lines by default */
#define LINESPACEADJUST 0

/* no image scaling by default */
#define IMAGESCALE 100

/* image output estimate bar style */
/* 0 = not shown, 1 = continuation of existing bar, 2 = separate bar */
#define ESTIMATESTYLE 1

/* bar column in list outputs shows rate when rate column is visible */
#define BARSHOWSRATE 0

/* 5 minute graph size */
#define FIVEGRESULTCOUNT 576
#define FIVEGHEIGHT 300
#define FIVEGMINRESULTCOUNT 288
#define FIVEGMINHEIGHT 150

/* hourly graph mode (0 = 24 hour sliding window, 1 = begins from midnight) */
#define HOURLYGMODE 0

/* graph used in extended summary, 0 = hours, 1 = 5 minutes*/
#define SUMMARYGRAPH 0

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

/* interface name length limit */
#define MAXIFLEN 32
#define MAXIFPARAMLEN 256

/* number of retries after non-fatal database errors, */
/* will result in given number + 1 tries in total before exit, */
/* a full disk (as reported by sqlite) will no cause retries or exit */
#define DBRETRYLIMIT 5

/* buffer sizes */
#define DATEBUFFLEN 64

/* internal config structure */
typedef struct {
	char dformat[64], mformat[64], tformat[64], hformat[64];
	char iface[MAXIFPARAMLEN];
	char locale[32];
	char dbdir[512], dbtzmodifier[14];
	char rxchar[2], txchar[2], rxhourchar[2], txhourchar[2], estimatetext[10];
	char cbg[8], cedge[8], cheader[8], cheadertitle[8], cheaderdate[8], ctext[8];
	char cline[8], clinel[8], cvnstat[8], crx[8], crxd[8], ctx[8], ctxd[8];
	int32_t unitmode, rateunitmode, rateunit, bvar, qmode, ifacematchmethod, sampletime, hourlyrate, summaryrate;
	int32_t monthrotate, monthrotateyears, maxbw, spacecheck, trafficlessentries, transbg, ostyle;
	int32_t defaultdecimals, hourlydecimals, hourlystyle, is64bit, waldb, dbsynchronous, useutc, imagescale;
	int32_t largefonts, linespaceadjust, estimatebarvisible, estimatestyle, estimatevisible, barshowsrate, fivegresultcount;
	int32_t fivegheight, summarygraph, hourlygmode, alwaysadd, livespinner;
	char cfgfile[512], logfile[512], pidfile[512];
	char daemonuser[33], daemongroup[33];
	int32_t timesyncwait, updateinterval, pollinterval, saveinterval, offsaveinterval, rescanonsave, savestatus;
	int32_t uselogging, createdirs, updatefileowner, bwdetection, bwdetectioninterval, utflocale;
	int32_t fiveminutehours, hourlydays, dailydays, monthlymonths, yearlyyears, topdayentries;
	int32_t listfivemins, listhours, listdays, listmonths, listyears, listtop, listjsonxml;
	int32_t timestampprints, interfaceorder, vacuumonstartup, vacuumonhupsignal, experimental;
} CFG;

/* internal interface information structure */
typedef struct {
	char name[MAXIFLEN];
	short filled;
	short is64bit;
	uint64_t rx;
	uint64_t tx;
	uint64_t rxp;
	uint64_t txp;
	time_t timestamp;
} IFINFO;

typedef struct ibwnode {
	char interface[MAXIFLEN];
	uint32_t limit;
	uint32_t fallback;
	short retries;
	time_t detected;
	struct ibwnode *next;
} ibwnode;

typedef enum PrintType {
	PT_Info = 0,
	PT_Infoless,
	PT_Warning,
	PT_Error,
	PT_Config,
	PT_Multiline,
	PT_ShortMultiline
} PrintType;

/* common functions */
int printe(const PrintType type);
int logprint(const PrintType type);
int verifylogaccess(void);
int dmonth(const int month);
int isleapyear(const int year);
time_t mosecs(time_t month, time_t updated);
uint64_t countercalc(const uint64_t *a, const uint64_t *b, const short is64bit);
char *strncpy_nt(char *dest, const char *src, size_t n);
int isnumeric(const char *s);
void panicexit(const char *sourcefile, const int sourceline) __attribute__((noreturn));
char *getversion(void);
double timeused(const char *func, const int reset);
void timeused_debug(const char *func, const int reset);

/* global variables */
extern CFG cfg;
extern IFINFO ifinfo;
extern char errorstring[1024];
extern ibwnode *ifacebw;
extern int debug;
extern int noexit; /* = running as daemon if 2 */
extern int intsignal;
extern int pidfile;
extern int disableprinte;
extern int stderrprinte;

#endif
