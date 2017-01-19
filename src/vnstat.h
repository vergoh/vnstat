#ifndef VNSTAT_H
#define VNSTAT_H

typedef struct {
	int query, import;
	int create, active, ifcount, force, traffic;
	int livetraffic, defaultiface, delete, livemode;
	char interface[32], dirname[512], nick[32], filename[512];
	char definterface[32], cfgfile[512], *ifacelist, jsonmode, xmlmode;
} PARAMS;

void initparams(PARAMS *p);
void showhelp(PARAMS *p);
void showlonghelp(PARAMS *p);
//void handledbmerge(PARAMS *p);
//void handleimport(PARAMS *p);
void handledelete(PARAMS *p);
void handlecreate(PARAMS *p);
void handleshowdatabases(PARAMS *p);
void showoneinterface(PARAMS *p, const char *interface);
void handletrafficmeters(PARAMS *p);

#endif
