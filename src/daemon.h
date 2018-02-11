#ifndef DAEMON_H
#define DAEMON_H

typedef struct {
	int running, updateinterval, dbcount, dodbsave, rundaemon;
	int dbsaved, showhelp, sync, saveinterval, forcesave, noadd;
	int alwaysadd;
	uint32_t dbhash;
	char cfgfile[512], dirname[512];
	char user[33], group[33];
	time_t current, prevdbupdate, prevdbsave;
	datanode *datalist;
} DSTATE;

void daemonize(void);
int addinterfaces(const char *dirname, const int running);
void debugtimestamp(void);

void initdstate(DSTATE *s);
void preparedatabases(DSTATE *s);
void setsignaltraps(void);
void filldatabaselist(DSTATE *s);
void adjustsaveinterval(DSTATE *s);
void checkdbsaveneed(DSTATE *s);
void processdatalist(DSTATE *s);
int datalist_cacheget(DSTATE *s);
void datalist_getifinfo(DSTATE *s);
int datalist_timevalidation(DSTATE *s);
int datalist_writedb(DSTATE *s);
void handleintsignals(DSTATE *s);
void preparedirs(DSTATE *s);
int waittimesync(DSTATE *s);

#endif
