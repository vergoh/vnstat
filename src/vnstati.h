#ifndef VNSTATI_H
#define VNSTATI_H

#include "image.h"

typedef struct {
	int cache, help;
	char interface[32], dirname[512], filename[512], cfgfile[512];
	FILE *pngout;
} IPARAMS;

void initiparams(IPARAMS *p);
void showihelp(IPARAMS *p);
void validateinput(IPARAMS *p);
void handlecaching(IPARAMS *p, IMAGECONTENT *ic);
void handledatabase(IPARAMS *p);
void openoutput(IPARAMS *p);
void writeoutput(IPARAMS *p, IMAGECONTENT *ic);

#endif
