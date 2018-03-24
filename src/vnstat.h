#ifndef VNSTAT_H
#define VNSTAT_H

typedef struct {
	int query, setalias;
	int addiface, force, traffic;
	int livetraffic, defaultiface, removeiface, livemode;
	uint64_t ifcount;
	char interface[32], dirname[512], alias[32], filename[512];
	char definterface[32], cfgfile[512], *ifacelist, jsonmode, xmlmode;
	char databegin[18], dataend[18];
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
