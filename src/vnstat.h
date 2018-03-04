#ifndef VNSTAT_H
#define VNSTAT_H

typedef struct {
	int query, setalias;
	int addiface, ifcount, force, traffic;
	int livetraffic, defaultiface, removeiface, livemode;
	char interface[32], dirname[512], alias[32], filename[512];
	char definterface[32], cfgfile[512], *ifacelist, jsonmode, xmlmode;
} PARAMS;

void initparams(PARAMS *p);
void showhelp(PARAMS *p);
void showlonghelp(PARAMS *p);
void handleremoveinterface(PARAMS *p);
void handleaddinterface(PARAMS *p);
void handlesetalias(PARAMS *p);
void handleshowdatabases(PARAMS *p);
void showoneinterface(PARAMS *p, const char *interface);
void handletrafficmeters(PARAMS *p);

#endif
