#ifndef VNSTAT_FUNC_H
#define VNSTAT_FUNC_H

typedef struct {
	int query, setalias;
	int addiface, force, traffic;
	int livetraffic, defaultiface, removeiface, renameiface, livemode;
	int32_t limit;
	uint64_t dbifcount;
	char interface[MAXIFPARAMLEN], alias[32], newifname[MAXIFLEN], filename[512];
	char definterface[MAXIFPARAMLEN], cfgfile[512], *ifacelist, jsonmode, xmlmode;
	char databegin[18], dataend[18];
	unsigned int alert, alertoutput, alertexit, alerttype, alertcondition;
	int alertrateunit, alertrateunitmode;
	uint64_t alertlimit;
	unsigned int merge;
	char mergesrc[512], mergedst[512];
} PARAMS;

void initparams(PARAMS *p);
void showhelp(const PARAMS *p);
void showlonghelp(const PARAMS *p);
void parseargs(PARAMS *p, const int argc, char **argv);
int parsealertargs(PARAMS *p, char **argv);
void showalerthelp(void);
void showstylehelp(void);
void handlemerge(PARAMS *p);
int parsedatabaseinterface(const char *input, char *database, char *interface);
void handleshowalert(PARAMS *p);
void handleremoveinterface(const PARAMS *p);
void handlerenameinterface(const PARAMS *p);
void handleaddinterface(PARAMS *p);
void handlesetalias(const PARAMS *p);
void handleshowdata(PARAMS *p);
void showoneinterface(PARAMS *p);
void handletrafficmeters(PARAMS *p);
void handleifselection(PARAMS *p);
void showiflist(const int mode);
void validateinterface(PARAMS *p);

#endif
