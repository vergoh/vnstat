#ifndef VNSTAT_H
#define VNSTAT_H

typedef struct {
	int update, query, newdb, reset, sync, merged, savemerged, import;
	int create, active, files, force, cleartop, rebuildtotal, traffic;
	int livetraffic, defaultiface, delete, livemode;
	char interface[32], dirname[512], nick[32], filename[512];
	char definterface[32], cfgfile[512], *ifacelist;
} PARAMS;

void initparams(PARAMS *p);
int synccounters(const char *iface, const char *dirname);
void showhelp(PARAMS *p);
void showlonghelp(PARAMS *p);
void handledbmerge(PARAMS *p);
void handlecounterreset(PARAMS *p);
void handleimport(PARAMS *p);
void handlecountersync(PARAMS *p);
void handledelete(PARAMS *p);
void handlecleartop10(PARAMS *p);
void handlerebuildtotal(PARAMS *p);
void handleenabledisable(PARAMS *p);
void handlecreate(PARAMS *p);
void handleupdate(PARAMS *p);
void handleshowdatabases(PARAMS *p);
void showoneinterface(PARAMS *p, const char *interface);
void handletrafficmeters(PARAMS *p);

#endif
