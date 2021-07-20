#ifndef VNSTAT_FUNC__H
#define VNSTAT_FUNC__H

typedef struct {
	int query, setalias;
	int addiface, force, traffic;
	int livetraffic, defaultiface, removeiface, renameiface, livemode;
	int32_t limit;
	uint64_t dbifcount;
	char interface[32], alias[32], newifname[32], filename[512];
	char definterface[32], cfgfile[512], *ifacelist, jsonmode, xmlmode;
	char databegin[18], dataend[18];
} PARAMS;

void initparams(PARAMS *p);
void showhelp(PARAMS *p);
void showlonghelp(PARAMS *p);
void parseargs(PARAMS *p, int argc, char **argv);
void showstylehelp(void);
void handleremoveinterface(PARAMS *p);
void handlerenameinterface(PARAMS *p);
void handleaddinterface(PARAMS *p);
void handlesetalias(PARAMS *p);
void handleshowdata(PARAMS *p);
void showoneinterface(PARAMS *p);
void handletrafficmeters(PARAMS *p);
void handleifselection(PARAMS *p);
void showiflist(const int parseable);
void showdbiflist(const int parseable);

#endif
