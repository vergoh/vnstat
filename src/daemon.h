#ifndef DAEMON_H
#define DAEMON_H

typedef struct {
	int running, updateinterval, dbcount, dodbsave, rundaemon;
	int dbsaved, showhelp, sync, saveinterval, forcesave, noadd;
	uint32_t dbhash;
	char cfgfile[512], dirname[512];
	char user[33], group[33];
	time_t current, prevdbupdate, prevdbsave;
	datanode *datalist;
} DSTATE;

void daemonize(void);
int addinterfaces(const char *dirname);
void debugtimestamp(void);
uid_t getuser(const char *user);
gid_t getgroup(const char *group);
void setuser(const char *user);
void setgroup(const char *group);
int direxists(const char *dir);
int mkpath(const char *dir, const mode_t mode);

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
void preparedbdir(DSTATE *s);
void updatedbowner(const char *dir, const char *user, const char *group);

#endif
