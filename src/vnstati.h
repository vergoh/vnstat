#ifndef VNSTATI_H
#define VNSTATI_H

typedef struct {
	int cache, help;
	char interface[32], filename[512], cfgfile[512];
	FILE *pngout;
} IPARAMS;

void initiparams(IPARAMS *p);
void showihelp(IPARAMS *p);
void parseargs(IPARAMS *p, IMAGECONTENT *ic, int argc, char **argv);
void validateinput(IPARAMS *p);
void handlecaching(IPARAMS *p, IMAGECONTENT *ic);
void handledatabase(IPARAMS *p, IMAGECONTENT *ic);
void openoutput(IPARAMS *p);
void writeoutput(IPARAMS *p, IMAGECONTENT *ic);

#endif
