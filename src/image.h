#ifndef IMAGE_H
#define IMAGE_H

#include <gd.h>		  /* libgd2-dev libgd2 */
#include <gdfontt.h>  /* gdFontGetTiny() */
#include <gdfonts.h>  /* gdFontGetSmall() */
#include <gdfontmb.h> /* gdFontGetMediumBold() */
#include <gdfontl.h>  /* gdFontGetLarge() */
#include <gdfontg.h>  /* gdFontGetGiant() */
#include "dbsql.h"

/* rectangle size */
#define YBEGINOFFSET (-1)

#define FIVEMINHEIGHTOFFSET 4
#define FIVEMINWIDTHFULLPADDING 10
#define FIVEMINWIDTHPADDING 2
#define FIVEMINEXTRASPACE 78

#define PERCENTILEMINWIDTHFULLPADDING 10

#define SCALEMINPIXELS 25

/* Graph y-axis left chrome: builtin keeps a 36px advance; TTF sizes from 4 digits. */
#define GRAPH_AXIS_BASE 36
#define GRAPH_AXIS_LABEL_GAP 4
#define GRAPH_AXIS_PLOT_PAD 5
#define GRAPH_EXTRA_RIGHT 29 /* FIVEMINEXTRASPACE - 8 - GRAPH_AXIS_BASE - GRAPH_AXIS_PLOT_PAD */

typedef enum {
	FONT_BUILTIN = 0,
	FONT_TTF
} fontmode_t;

typedef enum {
	FONT_ROLE_BODY = 0, /* Small / Large */
	FONT_ROLE_AXIS,     /* Tiny / Small — graph labels + header date */
	FONT_ROLE_TITLE,    /* Large / Giant — summary section titles */
	FONT_ROLE_HEADER,   /* always Giant — layoutinit title bar */
	FONT_ROLE_FOOTER    /* always Tiny — credit line */
} fontrole_t;

typedef struct {
	fontmode_t mode;
	gdFontPtr body, axis, title, header, footer;
	int cw, ch; /* body metrics for layout */
	int ascent; /* body: shared TTF baseline offset from y */
	int header_ascent; /* title/header role baseline offset */
	int axis_ascent; /* axis role baseline offset */
	int header_ch; /* measured header/title glyph height */
	int axis_ch; /* measured axis/date glyph height */
	int axis_num4_w; /* width of "9999" at axis size (TTF gutter) */
	int header_h; /* title bar height in pixels */
	char ttfpath[512];
	double ptsize; /* effective body point size after LargeFonts */
	double scale; /* cw / 6.0 relative to small builtin */
	double title_scale; /* title/header size ratio vs body */
	double axis_scale; /* axis label size ratio vs body */
} IMAGEFONT;

typedef struct {
	gdImagePtr im;
	IMAGEFONT fontctx;
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
int imagefontinit(IMAGECONTENT *ic, const int largefonts);
void imagefontcleanup(void);
void imagestring(IMAGECONTENT *ic, const fontrole_t role, const int x, const int y, const char *text, const int color);
void imagestringup(IMAGECONTENT *ic, const fontrole_t role, const int x, const int y, const char *text, const int color);
int imagetextwidth(IMAGECONTENT *ic, const fontrole_t role, const char *text);
int imagefontwidth(IMAGECONTENT *ic, const fontrole_t role);
int imagefontheight(IMAGECONTENT *ic, const fontrole_t role);
int imageextrapx(const IMAGECONTENT *ic, const int extra);
int imageuipx(const IMAGECONTENT *ic, const int base);
void imagedrawhline(IMAGECONTENT *ic, const int x1, const int x2, const int y, const int color);
void imagedrawvline(IMAGECONTENT *ic, const int x, const int y1, const int y2, const int color);
void imagedrawrect(IMAGECONTENT *ic, const int x1, const int y1, const int x2, const int y2, const int color);
void imagedrawdashedhline(IMAGECONTENT *ic, const int x1, const int x2, const int y, const int color);
int graph_axis_left(const IMAGECONTENT *ic);
int graph_xpos_margin(const IMAGECONTENT *ic);
int graph_extra_space(const IMAGECONTENT *ic);
int graph_axis_left_delta(const IMAGECONTENT *ic);
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
int drawpercentile(IMAGECONTENT *ic, const int mode, const int xpos, const int ypos, const int height, uint64_t *percentile);

#endif
