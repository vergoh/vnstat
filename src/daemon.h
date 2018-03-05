#ifndef DAEMON_H
#define DAEMON_H

typedef struct {
	int running, updateinterval, dodbsave, rundaemon;
	int dbsaved, showhelp, sync, saveinterval, forcesave, noadd;
	int alwaysadd, bootdetected, cleanuphour, dbretrycount;
	uint32_t iflisthash;
	uint64_t dbcount;
	char cfgfile[512], dirname[512];
	char user[33], group[33];
	time_t current, prevdbupdate, prevdbsave;
	datacache *dcache;
} DSTATE;

void daemonize(void);
void debugtimestamp(void);

unsigned int addinterfaces(DSTATE *s);
void initdstate(DSTATE *s);
void preparedatabases(DSTATE *s);
unsigned int importlegacydbs(DSTATE *s);
void setsignaltraps(void);
void filldatabaselist(DSTATE *s);
void adjustsaveinterval(DSTATE *s);
void checkdbsaveneed(DSTATE *s);
void processdatacache(DSTATE *s);
void processdatalist(DSTATE *s);
void handleintsignals(DSTATE *s);
void preparedirs(DSTATE *s);
void detectboot(DSTATE *s);

int initcachevalues(DSTATE *s, datacache **dc);
int processifinfo(DSTATE *s, datacache **dc);
void flushcachetodisk(DSTATE *s);
void handledatabaseerror(DSTATE *s);
void cleanremovedinterfaces(DSTATE *s);

void datacache_status(datacache **dc);

void interfacechangecheck(DSTATE *s);
uint32_t simplehash(const char *data, int len);

void errorexitdaemon(DSTATE *s, const int fataldberror) __attribute__((noreturn));

int getcurrenthour(void);
int waittimesync(DSTATE *s);

#endif
