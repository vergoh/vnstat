#ifndef VNSTATI_H
#define VNSTATI_H

#include "common.h"
#include "image.h"

#define FONT_SIZE_MIN 6
#define FONT_SIZE_MAX 72

typedef struct {
	int cache, help;
	int32_t limit;
	char interface[MAXIFPARAMLEN], filename[512], cfgfile[512];
	FILE *pngout;
} IPARAMS;

void initiparams(IPARAMS *p);
void showihelp(const IPARAMS *p);
void parseargs(IPARAMS *p, IMAGECONTENT *ic, int argc, char **argv);
void validateinput(const IPARAMS *p);
void handlecaching(const IPARAMS *p, const IMAGECONTENT *ic);
void handledatabase(IPARAMS *p, IMAGECONTENT *ic);
void validateoutput(const IPARAMS *p);
void writeoutput(IPARAMS *p, IMAGECONTENT *ic);
#if HAVE_DECL_GDIMAGEFILE
void showsupportedfileextensions(void);
#endif

#endif
