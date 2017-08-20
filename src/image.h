#ifndef IMAGE_H
#define IMAGE_H

#include <gd.h>        /* libgd2-dev libgd2 */
#include <gdfontt.h>   /* gdFontGetTiny() */
#include <gdfonts.h>   /* gdFontGetSmall() */
#include <gdfontmb.h>  /* gdFontGetMediumBold() */
#include <gdfontl.h>   /* gdFontGetLarge() */
#include <gdfontg.h>   /* gdFontGetGiant() */

/* rectangle size */
#define YBEGINOFFSET (-1)
#define YENDOFFSET 6

/* donut size */
#define DOUTRAD 49
#define DINRAD 15

typedef struct {
	gdImagePtr im;
	int cbackground, cedge, cheader, cheadertitle, cheaderdate, ctext, cline, clinel, cvnstat;
	int crx, crxd, ctx, ctxd, cbgoffset, showheader, showedge, showlegend, altdate;
	char headertext[65];
	time_t current;
} IMAGECONTENT;

void initimagecontent(IMAGECONTENT *ic);
void drawimage(IMAGECONTENT *ic);
void colorinit(IMAGECONTENT *ic);
void colorinitcheck(const char *color, int value, const char *cfgtext, const int *rgb);
void layoutinit(IMAGECONTENT *ic, char *title, int width, int height);
void drawlegend(IMAGECONTENT *ic, int x, int y);
void drawbar(IMAGECONTENT *ic, int x, int y, int len, uint64_t rx, int rxk, uint64_t tx, int txk, uint64_t max);
void drawpole(IMAGECONTENT *ic, int x, int y, int len, uint64_t rx, uint64_t tx, uint64_t max);
void drawdonut(IMAGECONTENT *ic, int x, int y, float rxp, float txp);
void drawhours(IMAGECONTENT *ic, int x, int y, int rate);
void drawsummary(IMAGECONTENT *ic, int type, int rate);
void drawoldsummary(IMAGECONTENT *ic, int type, int rate);
void drawhourly(IMAGECONTENT *ic, int rate);
void drawdaily(IMAGECONTENT *ic);
void drawmonthly(IMAGECONTENT *ic);
void drawtop(IMAGECONTENT *ic);
void hextorgb(char *input, int *rgb);
void modcolor(int *rgb, int offset, int force);
char *getimagevalue(uint64_t kb, int len, int rate);
char *getimagescale(uint64_t kb, int rate);

#endif
