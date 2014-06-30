#ifndef VNSTATD_H
#define VNSTATD_H

typedef struct {
	int running, updateinterval, dbcount, dodbsave, rundaemon;
	int dbsaved, showhelp, sync, saveinterval, forcesave, noadd;
	uint32_t dbhash;
	char cfgfile[512], dirname[512];
	char user[33], group[33];
	time_t current, prevdbupdate, prevdbsave;
	datanode *datalist;
} DSTATE;

void initdstate(DSTATE *s);
void showhelp(void);
void preparedatabases(DSTATE *s);
void setsignaltraps(void);
void filldatabaselist(DSTATE *s);
void adjustsaveinterval(DSTATE *s);
void checkdbsaveneed(DSTATE *s);
void processdatalist(DSTATE *s);
void handleintsignals(DSTATE *s);

#endif
