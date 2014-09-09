#ifndef VNSTATI_H
#define VNSTATI_H

#define YBEGINOFFSET -1
#define YENDOFFSET 6

#define DOUTRAD 49
#define DINRAD 15

typedef struct {
	int cache, help;
	char interface[32], dirname[512], filename[512], cfgfile[512];
} IPARAMS;

void initiparams(IPARAMS *p);
void showihelp(IPARAMS *p);

#endif
