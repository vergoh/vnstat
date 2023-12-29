#ifndef IMAGE_H
#define IMAGE_H

#include <gd.h>		  /* libgd2-dev libgd2 */
#include <gdfontt.h>  /* gdFontGetTiny() */
#include <gdfonts.h>  /* gdFontGetSmall() */
#include <gdfontmb.h> /* gdFontGetMediumBold() */
#include <gdfontl.h>  /* gdFontGetLarge() */
#include <gdfontg.h>  /* gdFontGetGiant() */

/* rectangle size */
#define YBEGINOFFSET (-1)

#define FIVEMINHEIGHTOFFSET 4
#define FIVEMINWIDTHFULLPADDING 10
#define FIVEMINWIDTHPADDING 2
#define FIVEMINEXTRASPACE 78

#define PERCENTILEMINWIDTHFULLPADDING 10

#define SCALEMINPIXELS 25

typedef struct {
	gdImagePtr im;
	gdFontPtr font;
	interfaceinfo interface;
	int cbackground, cedge, cheader, cheadertitle, cheaderdate, ctext, cline, clinel, cpercentileline, cvnstat;
	int crx, crxd, ctx, ctxd, ctotal, cbgoffset, cbgoffsetmore, showheader, showedge, showlegend, altdate;
	int lineheight, large, invert;
	char headertext[65], databegin[18], dataend[18];
	time_t current;
} IMAGECONTENT;

typedef struct {
	time_t date;
	uint64_t rx, tx;
} HOURDATA;

void initimagecontent(IMAGECONTENT *ic);
void drawimage(IMAGECONTENT *ic);
#if HAVE_DECL_GD_NEAREST_NEIGHBOUR
void scaleimage(IMAGECONTENT *ic);
#endif
int drawhours(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte);
void drawhourly(IMAGECONTENT *ic, const int israte);
void drawlist(IMAGECONTENT *ic, const char *listname);
void drawsummary(IMAGECONTENT *ic, const int layout, const int israte);
void drawsummary_alltime(IMAGECONTENT *ic, const int x, const int y);
void drawsummary_digest(IMAGECONTENT *ic, const int x, const int y, const char *mode);
void drawfivegraph(IMAGECONTENT *ic, const int israte, const int resultcount, const int height);
int drawfiveminutes(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte, const int resultcount, const int height);
void draw95thpercentilegraph(IMAGECONTENT *ic, const int mode);
void drawpercentile(IMAGECONTENT *ic, const int mode, const int xpos, const int ypos, const int height);

#endif
